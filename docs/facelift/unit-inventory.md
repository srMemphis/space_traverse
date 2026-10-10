# Unit inventory for facelift unit tests

Input for the wayfinder ticket "Which units get tests in the facelift, and in what order?" (map: "Facelift: unit tests, premake, code style, docs"). Read at commit `18e3263`. This inventory decides nothing; it lists facts.

**Refactor risk** = how likely the upcoming architecture refactor rewrites the unit, so a test of it would be thrown away. **Seam** = the minimal change a test needs, within the map's rule (no responsibility moves between classes).

## Units

| # | Unit | File | Depends on | Testable as-is? | Minimal seam | Refactor risk |
|---|---|---|---|---|---|---|
| 1 | `Geometry`: `ThreePointPlane`, `BarycentricGradient`, `TriangleArea`, `TetrahedronVolume` | `utils/Geometry.h` | Eigen | Yes (pure static) | none | Low |
| 2 | `Vec3` | `utils/Vec3.h` | — | Yes | none | Medium: duplicates Eigen, used only in `FindElementByCoords`; likely removed |
| 3 | `Box`: `Contains`, `Split` | `mesh/Box.h` | — | Yes | none | Low |
| 4 | `GJK::Check` (box vs. tetrahedron intersection) | `mesh/GJK.h` | Eigen, `Box`, `ETetrahedron`, `Node` | Yes (public `Check`) | none | Low |
| 5 | `UnsignedRange` + `RangeIterator` | `utils/UnsignedRange.*` | — | Yes | none | Medium: the i/j index split may be redesigned, but the class is subtle and bug-prone |
| 6 | `TopologyTree`: `GenerateData`, `Find`, `IsInBounds` | `mesh/TopologyTree.*` | `Box`, `GJK`, element types, Log | Yes, with hand-built `Volume` map + element/node vectors | logger (see cross-cutting) | High: `// TODO: rewrite this SHIT` |
| 7 | Mesh parsing: `Mesh::FromFile` + private `Read*` sections (Gmsh 4.1 text) | `mesh/Mesh.cpp` | `std::ifstream`, Log, Profiler | Only through a file on disk | `Read*` take `std::istream&` instead of `std::ifstream&`; add `FromStream(std::istream&)` that `FromFile` delegates to. Tiny `.msh` text inline in tests | Medium |
| 8 | Mesh queries: `NameToTag`, `TagToName`, `GetPhisicalTagOfTetrahedron`, `GetVolumeOfTetrahedron`, `GetVolumeOfVolume`, `GetBoundingBox`, `MaxTetraAtNode`, `FindElementByCoords`, `IsInNarrowBounds` | `mesh/Mesh.cpp` | loaded mesh | Via seam 7 | seam 7 | Medium |
| 9 | `Mesh::GenerateGradientVectors` (private; read by `Solver` via `friend`) | `mesh/Mesh.cpp` | `Geometry` | Indirectly: `Solver::GetField` on a linear potential | none | Medium |
| 10 | `Solver`: `SetMesh`, `SetBoundaryValues`, `AssembleLeft`/`AssembleRight`, `Solve`, `GetField`, `AddElementCharge` | `solver/Solver.cpp` | `Mesh` (+ privates), `UnsignedRange`, Eigen CG, Log | Via seam 7 | seam 7. Natural analytic case: a cube of a few tets with Dirichlet values on two opposite faces; linear FEM reproduces a linear potential and a uniform field exactly | Medium-high: the refactor will touch the Kii/Kij split and the `friend` |
| 11 | `Track`: `MakeStep`, `CalcNewVelocity`, `SetAcceleration`, `CalcDt` | `particles/Track.cpp` | Eigen | Yes | none | Low-medium |
| 12a | `Beam` start positions: `GeneratePrimaryPosition` (isocell disk) + `GetTracks` | `particles/Beam.cpp` | Eigen | Yes | none | Medium |
| 12b | `Beam::Run` (initial speed from energy, integration loop) | `particles/Beam.cpp` | callbacks, OpenMP, `std::random_device` | Yes when divergence = 0 (deterministic): stub field (uniform) + stub stop (plane) | optional: injectable seed for divergence > 0 | Medium |
| 13 | `Beam::SaveTracksToFile` downsampling | `particles/Beam.cpp` | `ToMatlab`/TinyMAT | No (writes `.mat`, no reader) | would need extracting the downsampling | Medium; skip |
| 14 | `GetSecondaryCoefficient` | `SimuControl.cpp` (free function, no header) | Log | No | declare in a header | High: placeholder physics |
| 15 | YAML helpers: `yaml::FromFile`, `NodeToString`/`Int`/`Double`/`Bool` | `utils/yaml.h` | fkYAML, Log | Yes (`fkyaml::node::deserialize` from a string) | none | Medium (GUI later) |
| 16 | `SimuControl::ConfigFromFile` | `SimuControl.cpp` | file, private `m_params` | No | extract `ParseConfig(const node&, SimParameters&)` | High |
| 17 | `SimuControl::Prepare` / `Run` / `BeamShouldStop` / `RegisterHits` / `GenerateSurfaceToVolumeIndexing` | `SimuControl.cpp` | everything, filesystem, `.mat` output | No | beyond facelift rules | Very high; skip |
| 18 | `DetectorData`, `ToMatlab` | `DetectorData.h`, `utils/ToMatlab.*` | TinyMAT | No reader for `.mat` | — | skip |
| 19 | `Log`, `Profiler` | `debug/` | spdlog | infrastructure | — | skip |

## Cross-cutting seams

- **Logger:** `LOG_*` macros dereference `Log::GetLogger()`, which is null until `Log::Init` runs. Any tested code path that logs crashes unless the test runner initializes logging (e.g. `Log::Init` in the test `main`), or `Log` falls back to a null logger.
- **Linking:** all of `src/` except `main.cpp` must be linkable by the test exe (the static-lib question in the build-layout ticket). `GetSecondaryCoefficient` and `ConfigFromFile` live in `SimuControl.cpp`, which pulls in TinyMAT and the filesystem.
- **Profiler:** `PROFILE_*` macros compile out unless `PROFILER` is defined. No seam needed.

## Suspected bugs found while reading (unconfirmed; tests should confirm)

Classified per the map's policy. Programming bugs get fixed in the facelift; physics and numerics bugs get filed separately and are not fixed. Items marked ⚠ would change simulation results if fixed.

**Programming**

- P1 `Mesh::TagToName` returns `""` through a `const std::string&`, which dangles when the tag is not found (UB).
- P2 `Mesh::GetPhisicalTagOfTetrahedron` has no `return` when the index is in no volume (UB). It is reachable from `RegisterHits` when `FindElementByCoords` returns -1/-2 for a track end point.
- P3 `Solver::GetFieldCasual`/`GetFieldSmooth` call `.at(elemIndex)` with -1/-2, which throws `std::out_of_range` inside the OpenMP region and leads to `std::terminate`. `BeamShouldStop`'s narrow-bounds check probably prevents most cases.
- P4 `Mesh::GetVolumeOfVolume` and `Solver::AssembleLeft` (materials) dereference `map::find()` without checking for `end()`.
- P5 `RangeIterator` has three problems:
  - The loops in `Solver::Solve` (`for (i = Get(); i < GetMax(); Advance())`) never write the node when the range holds a single element.
  - An empty range dereferences `end()`.
  - A copied iterator keeps `m_iter` pointing into the source's list.
- P6 `Beam` leaves `m_divergence`, `m_energy`, `m_stepSize`, `m_direction` and `m_centerPosition` uninitialized until their setters run.
- P7 `Mesh::Clear` does not clear `m_PhisicalNames` or `m_GradientVectors`, so loading a second mesh leaves stale data behind.
- P8 ⚠ `TopologyTree::GenerateData`, `Mesh::GetBoundingBox` and `GenerateTightBox` start from `Box()`, which is all zeros, so the box always contains the origin. `GenerateTightBox` scales by 0.999 *about the origin*, which grows the box on sides that do not straddle zero. The bounding box sets the beam step size, so a fix changes results.
- P9 `GJK::Check`: when the box and tetrahedron centers coincide, it normalizes a zero vector (NaN). On `MAX_ITER` it returns `false` (`FIXME`), which can drop elements from the BVH, and then `FindElementByCoords` fails.

**Physics / numerics (defer, file separately)**

- N1 The speed comes from a relativistic formula, but the acceleration is `F / m_e`, with no γ.
- N2 π is approximated as `3.1415` in `Beam::SetDivergence` and in the azimuth distribution.
- N3 `GetSecondaryCoefficient` is a step function: 0 above 1700 eV, 1 below.
- N4 `FindElementByCoords` uses strict `> 0` tests, so points on faces or edges are not found. It also assumes positively oriented tetrahedra.
- N5 The Jacobian determinant is used without `abs` (`/-6` in `AssembleLeft`, `/6` in `AssembleRight`), so the signs depend on node ordering.
- N6 `Track::GetVelocity`/`GetAcceleration` return `.second` after the swap, which is the *previous* step's value. These feed the detector and impact energies.
- N7 `RegisterHits` hardcodes `1.602e-19` instead of `phys::qe`.
