# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

@AGENTS.md

## What this is

A Windows C++20 console app that simulates electron-beam charging of dielectric targets. Each iteration solves the electrostatic potential on a tetrahedral FEM mesh. It then traces electron tracks through the resulting field and deposits the charge of the tracks that hit the target back into the mesh. Results go to `.mat` files and are visualized with MATLAB scripts.

## Build and run

- The only build system is Visual Studio: `space_traverse.sln` → `space_traverse/space_traverse.vcxproj`. It uses platform toolset `v145` (VS 2026), C++20 and OpenMP. Use the `x64` configurations. `TinyMAT_Debug.lib`/`TinyMAT_Release.lib` are prebuilt for x64 in `space_traverse/lib/TinyMAT/`.
- Command-line build from a VS Developer shell:
  ```
  msbuild space_traverse.sln /p:Configuration=Release /p:Platform=x64
  ```
- The output goes to `x64/<Config>/space_traverse.exe`.
- The executable takes no arguments. It reads `config.yaml` from the **current working directory**, and relative paths in the config (`msh/...`, `out/...`) resolve from there. Run it with `space_traverse/` as the working directory; this is the VS debugger default.
- To switch scenarios, copy one of `space_traverse/configs/*.yaml` over `space_traverse/config.yaml`. Each config names a `.msh` file in `space_traverse/msh/`.
- There is no test suite and no linter.

## Pipeline (`src/main.cpp` → `SimuControl`)

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
- `debug/`: spdlog setup (`LOG_*` macros, `simulog.log`) and a Chrome-trace profiler that writes `profile.json`. The profiler only runs when `#define PROFILER` in `debug/Define.h` is uncommented.

## Mesh and config conventions

- Geometry is authored in `space_traverse/msh/scripts/*.geo` and meshed with Gmsh into `msh/*.msh`.
- Physical group names in the mesh must match the names in the config: the `Target`, `World` and optional `Mask` volumes and their `... Boundary` surfaces. The code also looks for a volume named `Detector`.
- Units are SI (m, A, s) except beam energy, which is in eV.
- `Solver.smooth` is parsed, but `Prepare` hardcodes `SetFieldSmoothing(false)`.

## Third-party code

Everything under `space_traverse/include/` (Eigen, fkYAML, spdlog, TinyMAT) and `space_traverse/lib/` is vendored. Don't edit it.

## Post-processing

`space_traverse/scripts/*.m` are MATLAB scripts that load `out/<run>/surfacegeo.mat` and `solution_<iter>.mat`. They contain hardcoded run directories and iteration numbers.
