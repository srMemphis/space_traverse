# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

@AGENTS.md

## What this is

A Windows C++20 console app that simulates electron-beam charging of dielectric targets. Each iteration solves the electrostatic potential on a tetrahedral FEM mesh. It then traces electron tracks through the resulting field and deposits the charge of the tracks that hit the target back into the mesh. Results go to `.mat` files and are visualized with MATLAB scripts.

## Build and run

- The build is defined in `premake5.lua` and generated with premake 5.0.0, vendored as `tools/premake/premake5.exe`. Visual Studio files are not checked in; everything generated goes to `build/` (ignored).
- Generate, then build from a VS Developer shell:
  ```
  generate_vs2026.bat
  msbuild build/space_traverse.slnx /p:Configuration=Release /p:Platform=x64
  ```
  Re-run `generate_vs2026.bat` after editing `premake5.lua` or adding/removing source files. It writes `build/space_traverse.slnx` and `build/projects/*.vcxproj`. Always pass the `.slnx` explicitly to MSBuild.
- Toolset `v145` (VS 2026), C++20, OpenMP, x64 only. Projects:
  - `space_traverse_core` (StaticLib): everything in `space_traverse/src/`.
  - `space_traverse` (ConsoleApp): `space_traverse/app/main.cpp`, links `space_traverse_core` and TinyMAT. `TinyMAT_Debug.lib`/`TinyMAT_Release.lib` are prebuilt for x64 in `space_traverse/lib/TinyMAT/`; `lib/TinyMAT/License/SOURCE.txt` records how to rebuild them.
- Configurations: `Debug`, `Release` (optimized, `/GL`), and `Profile` (Release plus `PROFILER`, which turns on the Chrome-trace profiler). All three have debug symbols.
- The exe goes to `build/<Config>/space_traverse.exe`; intermediates go to `build/obj/<Config>/<project>/`.
- The executable takes no arguments. It reads `config.yaml` from the **current working directory**, and relative paths in the config (`msh/...`, `out/...`) resolve from there. Run it with `space_traverse/` as the working directory; `debugdir` in `premake5.lua` makes that the VS debugger default.
- To switch scenarios, copy one of `space_traverse/configs/*.yaml` over `space_traverse/config.yaml`. Each config names a `.msh` file in `space_traverse/msh/`.
- There is no test suite and no linter. The smoke check below is the regression gate.

## Smoke check

Use it to prove that a change keeps the simulation output bit-identical. It runs `space_traverse/configs/smoke.yaml` (sphere_mask, beam aimed at the Target so charge deposits, `divergence: 0`, about 1.5 min in Release) and compares the `.mat` files.

1. Build Release before the change and save the exe outside the repository, e.g. `build/Release/space_traverse.exe` → a temp folder.
2. Make the change and build Release again.
3. From the repository root, run both and compare:
   ```
   py tools/smoke/run.py <saved>/space_traverse.exe before
   py tools/smoke/run.py build/Release/space_traverse.exe after
   py tools/smoke/compare.py space_traverse/out/smoke_before space_traverse/out/smoke_after
   ```
   `run.py` swaps `config.yaml` for the smoke config, runs the exe from `space_traverse/`, moves the output to `space_traverse/out/smoke_<label>` and restores `config.yaml`. `compare.py` exits 0 on a match; on a mismatch it lists the differing files and the first differing byte offset.

Two runs of the same Release exe match bit for bit with OpenMP at its default thread count, so any mismatch comes from the change. `compare.py` skips the first 116 bytes of each file: that MAT header holds TinyMAT's creation timestamp.

## Pipeline (`app/main.cpp` → `SimuControl`)

1. `ConfigFromFile` parses `config.yaml` (fkYAML, helpers in `utils/yaml.h`) into `SimParameters`.
2. `Prepare`:
   - **Deletes** `outputDir` and recreates it, then copies `config.yaml` into it.
   - Loads the mesh and writes `surfacegeo.mat`.
   - Registers materials, sets Dirichlet 0 V on the World and Mask boundaries, and assembles the stiffness matrix (`AssembleLeft`) once.
   - Configures the `Beam`: start positions, step size derived from the mesh bounding box and `numsteps`, a stopping callback (`SimuControl::BeamShouldStop`) and a field callback (`Solver::GetField`).
3. `Run`, repeated for `iterations`:
   - `Solver::AssembleRight`, then `Solve`. `Solve` uses Eigen's conjugate gradient, warm-started from the previous solution.
   - `Beam::Reset` + `Run`. Tracks are traced in parallel with OpenMP, so the stopping and field callbacks must be thread-safe.
   - `RegisterHits` classifies each track's end element by physical tag. Detector hits are recorded in `DetectorData`. Target hits deposit `current·dt/numTracks`, reduced by a secondary-emission coefficient, as element charge.
   - Every `saveInterval` iterations, and on the last one, it writes `out/.../solution_<iter>.mat` (potential, tracks, charge, detector data).

## Modules (`space_traverse/src`)

- `mesh/`: `Mesh` parses the Gmsh 4 `.msh` format (sections, physical names, entities, nodes, elements). It precomputes the per-tetrahedron gradient vectors used by the solver; `Solver` is a `friend` of `Mesh`. `TopologyTree` is a bounding-box BVH used by `FindElementByCoords`. `GJK.h` and `Box.h` provide intersection tests.
- `solver/`: linear-tetrahedron FEM for Poisson's equation. Matrices are split into the interior (`Kii`) and boundary-coupled (`Kij`) blocks; `UnsignedRange` maps global node indices to i/j indices. Material = relative permittivity (`epsilon`).
- `particles/`: `Beam` generates `3·n²` starting points on a disk with divergence. `Track` integrates relativistic electron motion with a Verlet-style scheme using a **fixed spatial step size**, deriving `dt` from it.
- `utils/ToMatlab`: wraps TinyMAT for writing `.mat` files.
- `debug/`: spdlog setup (`LOG_*` macros, `simulog.log`) and a Chrome-trace profiler that writes `profile.json`. The profiler only runs in the `Profile` configuration, which defines `PROFILER`.

## Mesh and config conventions

- Geometry is authored in `space_traverse/msh/scripts/*.geo` and meshed with Gmsh into `msh/*.msh`.
- Physical group names in the mesh must match the names in the config: the `Target`, `World` and optional `Mask` volumes and their `... Boundary` surfaces. The code also looks for a volume named `Detector`.
- Units are SI (m, A, s) except beam energy, which is in eV.
- `Solver.smooth` is parsed, but `Prepare` hardcodes `SetFieldSmoothing(false)`.

## Third-party code

Everything under `space_traverse/include/` (Eigen, fkYAML, spdlog, TinyMAT), `space_traverse/lib/` and `tools/premake/` (premake 5.0.0, BSD-3) is vendored. Don't edit it.

## Post-processing

`space_traverse/scripts/*.m` are MATLAB scripts that load `out/<run>/surfacegeo.mat` and `solution_<iter>.mat`. They contain hardcoded run directories and iteration numbers.
