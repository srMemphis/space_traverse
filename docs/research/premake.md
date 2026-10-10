# Research: what premake5 supports for this project

Ticket: #5 (child of map #3). Researched 2026-10-10 against premake-core **v5.0.0**. Sources are the tagged source tree, the docs that ship in that tree (`website/docs`, published at premake.github.io), and GitHub release notes. Links are pinned to the `v5.0.0` tag unless noted otherwise.

## TL;DR

- **Latest release: Premake 5.0 (`v5.0.0`, 2026-10-05), the first stable 5.x.** The previous release was 5.0-beta8 (2026-01-24). ([releases](https://github.com/premake/premake-core/releases/tag/v5.0.0))
- **`vs2026` is a built-in action, and its default toolset is `msc-v145`.** No workaround is needed. It was added in beta8 by PR #2504, "Added support for Visual Studio 2026". ([vs2026.lua](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2026.lua), [beta8 notes](https://github.com/premake/premake-core/releases/tag/v5.0.0-beta8))
- **The `vs2026` action writes a `.slnx` solution, not `.sln`.** The build command in `CLAUDE.md` changes accordingly.
- **The `flags` API was removed in 5.0.0.** Most premake tutorials and examples online still use `flags { "MultiProcessorCompile" }` and similar, and those no longer work. Use the dedicated APIs.
- Everything else the current vcxproj needs has a first-class API: `openmp "On"`, `cppdialect "C++20"`, `debugdir`, `externalincludedirs` + `externalwarnings "Off"`, per-config `links`/`libdirs` via `filter`, and multi-project workspaces with `links`/`dependson`/`uses`.
- **CMake output is not built in.** Built-in exporters are codelite, compilecommands, gmake, gmakelegacy, ninja, vstudio and xcode. CMake comes from the community module [Jarod42/premake-cmake](https://github.com/Jarod42/premake-cmake) (Apache-2.0, updated for the post-`flags` API in 2026).

## 1. Release and Visual Studio 2026 / v145

- The `v5.0.0` release ships `premake-5.0.0-windows.zip` (about 640 KB). Its notes include PR #2812, "Update Windows builds to use VS 2026", and PR #2760, "implement defaultconfiguration to sln2026". ([release](https://github.com/premake/premake-core/releases/tag/v5.0.0))
- `modules/vstudio/vs2026.lua` registers the action as `trigger = "vs2026"` and `toolset = "msc-v145"`. Its `vstudio` block sets `versionName = "Version 18"` and `toolsVersion = "15.0"`. ([source](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2026.lua))
- `PlatformToolset` comes from the configured `toolset` when one is set; otherwise it falls back to the action's default (`m.platformToolset`, [vs2010_vcxproj.lua](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua)). With `premake5 vs2026`, every config gets `<PlatformToolset>v145</PlatformToolset>` without a `toolset` line.
- **Fallback (not needed on 5.0.0):** run `premake5 vs2022` with `toolset "msc-v145"`. The `toolset` doc allows a version suffix such as `"msc-v110"` or `"v100"`. ([toolset.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/toolset.md)) This produces a `.sln` and VS2022-era project headers with toolset v145.
- **`.slnx` solution.** `vs2026.generateSolution` calls `p.generate(wks, ".slnx", vstudio.sln2026.generate)` ([vs2026.lua](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2026.lua)). MSBuild builds `.slnx` from 17.12/17.13 on ([MSBuild CLI reference](https://learn.microsoft.com/en-us/visualstudio/msbuild/msbuild-command-line-reference), [VS blog](https://devblogs.microsoft.com/visualstudio/new-simpler-solution-file-format/)), so `msbuild space_traverse.slnx /p:Configuration=Release /p:Platform=x64` works with VS 2026. MSBuild requires an explicit file argument when both a `.sln` and a `.slnx` sit in the same folder.

## 2. Settings in the current vcxproj, mapped to premake 5.0.0

| vcxproj today | premake 5.0.0 | Source |
|---|---|---|
| `OpenMPSupport=true` | `openmp "On"` writes `<OpenMPSupport>true</OpenMPSupport>`; with the clang toolset it adds `/openmp` instead. The field is registered with **`scope = "project"`**, so set it once per project, not under a config filter. | [openmp.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/openmp.md), `m.supportOpenMP` in [vs2010_vcxproj.lua](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua), [_premake_init.lua](https://github.com/premake/premake-core/blob/v5.0.0/src/_premake_init.lua) |
| `LanguageStandard=stdcpp20` | `cppdialect "C++20"` writes `stdcpp20` for actions newer than vs2017. C++23 and C++26 are also accepted. | `m.languageStandard` in vs2010_vcxproj.lua; allowed values in _premake_init.lua |
| `ConformanceMode=true` | `conformancemode "On"` (boolean field, registered by the vstudio module) | [vstudio/_preload.lua](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/_preload.lua) |
| `MultiProcessorCompilation=true` | `multiprocessorcompile "On"`. The old `flags { "MultiProcessorCompile" }` is removed. | [flags.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/flags.md), _premake_init.lua |
| `FunctionLevelLinking`, `IntrinsicFunctions` | `functionlevellinking "On"`, `intrinsics "On"`. When unset, both are emitted as `true` for optimized configs. | _preload.lua, vs2010_vcxproj.lua |
| `WholeProgramOptimization=true` (Release) | `linktimeoptimization "On"`. The old `flags { "LinkTimeOptimization" }` is removed. | flags.md |
| `GenerateDebugInformation=true` in **Release** | `symbols "On"` in Release too. Release gets no PDB unless this is set explicitly. | `m.generateDebugInformation` |
| `FavorSizeOrSpeed=Speed` | `optimize "Speed"` (Release) | optimize.md |
| `SDLCheck=true` | **No dedicated API in the vstudio exporter** (no `SDLCheck` emitter exists). Use `buildoptions { "/sdl" }`. | grep of vs2010_vcxproj.lua @v5.0.0 |
| `/utf-8` in AdditionalOptions | `buildoptions { "/utf-8" }` | buildoptions.md |
| `CharacterSet=Unicode` | `characterset "Unicode"` | _premake_init.lua |
| `WindowsTargetPlatformVersion=10.0` | `systemversion "latest"` | `m.windowsTargetPlatformVersion` |
| `_DEBUG/NDEBUG;_CONSOLE` | `defines` under `filter "configurations:Debug"` and `"configurations:Release"`. For `kind "ConsoleApp"`, premake adds `_CONSOLE` itself. | defines.md |

### Prebuilt TinyMAT per configuration

```lua
libdirs { "space_traverse/lib/TinyMAT" }
filter "configurations:Debug"
    links { "TinyMAT_Debug" }      -- msc.getlinks appends ".lib"
filter "configurations:Release"
    links { "TinyMAT_Release" }
filter {}
```

- `links` and `libdirs` are config-scoped. Write system libraries without their extension; `msc.getlinks` appends `.lib` when the name has no known library extension. ([Linking.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/Linking.md), [msc.lua](https://github.com/premake/premake-core/blob/v5.0.0/src/tools/msc.lua))
- A link name that matches a sibling project's name becomes a `<ProjectReference>`; any other name goes to `AdditionalDependencies`.

### Debugger working directory

- `debugdir "space_traverse"` (relative to `premake5.lua`) writes `<LocalDebuggerWorkingDirectory>` to the **`.vcxproj.user`** file, not the `.vcxproj`. ([debugdir.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/debugdir.md), [vs2010_vcxproj_user.lua](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj_user.lua))
- Premake only writes the `.user` file when it has something to put in it. Regenerating replaces a hand-edited `.user` file.
- Today the vcxproj sets no working directory; it relies on VS's default of `$(ProjectDir)`. That stops being `space_traverse/` as soon as `location` moves the generated project, which makes `debugdir` mandatory after the move.
- `.gitignore` already ignores `*.user`, which is fine because premake regenerates the file.

### Vendored headers as external

```lua
includedirs         { "space_traverse/src" }
externalincludedirs { "space_traverse/include" }
externalwarnings    "Off"       -- /external:W0, ExternalWarningLevel=TurnOffAllWarnings
```

- For vs2019 and newer actions, `externalincludedirs` goes into the `<ExternalIncludePath>` property (VC++ Directories). ([externalincludedirs.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/externalincludedirs.md); `m.includePath`)
- For vs2019 and newer, `m.externalWarningLevel` **always** writes `<ExternalWarningLevel>`, defaulting to `Level3` when `externalwarnings` is unset. Set `"Off"` explicitly to silence Eigen, spdlog and fkYAML once the warning level goes to `/W4`.
- The MSC command-line path adds `/external:W3` automatically when external dirs exist and no level is set (comment D9007 in msc.lua). That path does not apply to vstudio, but it matters for ninja or compilecommands.
- Our code includes vendored headers with angle brackets (`<Eigen/Dense>`, `<spdlog/spdlog.h>`). `externalanglebrackets "On"` (`TreatAngleIncludeAsExternal`) is an optional extra and not required.
- `sysincludedirs` was removed in beta8 (PR #2573). Use `externalincludedirs` only.

## 3. Multiple projects in one workspace (static lib + exe + test exe)

```lua
workspace "space_traverse"
    configurations { "Debug", "Release" }
    platforms { "x64" }               -- drop Win32; the TinyMAT libs are x64-only
    location "build"                  -- optional; keeps generated files out of the root
    startproject "space_traverse"

project "core"   kind "StaticLib"   -- every src/**.cpp except main.cpp
project "space_traverse" kind "ConsoleApp"  links { "core" }   -- main.cpp
project "tests"  kind "ConsoleApp"  links { "core" }   -- test sources + framework
```

- Linking a sibling by project name is documented in [Linking.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/Linking.md). In VS this becomes a `<ProjectReference>`, which gives both build order and implicit linking (`m.linkLibraryDependencies`).
- `dependson { ... }` is for build-order-only dependencies that are not linked. ([dependson.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/dependson.md)) It is not needed for the lib/exe/test triangle.
- **Sharing settings.** 5.0 adds `usage` and `uses` blocks (`PUBLIC`/`INTERFACE`/`PRIVATE`). `core` can publish its include dirs, defines, `openmp` and the TinyMAT `links` to both consumers. ([Usages-and-Uses.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/Usages-and-Uses.md)) Usages do not inherit from parent scopes and do not propagate recursively unless re-exported. A plain Lua function called in each project also works.
- **Gotcha: `links` on a StaticLib.** For `kind "StaticLib"`, premake writes `links` into `<Lib><AdditionalDependencies>` (`m.elements.lib`). The librarian would then merge `TinyMAT_*.lib` into `core.lib`. Put TinyMAT on the consumers instead, either directly or through an `INTERFACE` usage.
- **Gotcha: OpenMP and the test exe.** `openmp` is project-scoped, so the `tests` project must turn it on too if it calls code that uses `#pragma omp`. Otherwise the pragmas are silently ignored and tests run single-threaded, which hides races.

## 4. Distributing premake

- The docs describe premake as "a small command line executable, delivered as a single file", to be placed on the PATH "or anywhere else convenient". ([Using-Premake.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/Using-Premake.md)) The Windows zip is about 640 KB.
- **Package managers lag behind.** Issue [#2411](https://github.com/premake/premake-core/issues/2411) (open) records winget at `5.0-beta2` and choco at `premake.portable 4.3.0`. The maintainer deferred package-manager publishing until there was a stable channel, and 5.0.0 is the first one. Until winget or choco catch up, the reproducible options are vendoring the exe or downloading the pinned release asset.
- **License: BSD-3-Clause** ([LICENSE.txt](https://github.com/premake/premake-core/blob/v5.0.0/LICENSE.txt)). Committing `premake5.exe` is allowed as long as the copyright notice and conditions travel with it. Keep `LICENSE.txt` next to it, the same way `space_traverse/lib/TinyMAT/License` sits next to the TinyMAT libs.
- **Gotcha: `.gitignore`.** It ignores `[Bb]in/`, `[Oo]bj/`, `x64/` and `[Oo]ut/`. A conventional `vendor/premake/bin/premake5.exe` path is silently ignored by the `[Bb]in/` rule. Choose a path without a `bin` segment (for example `tools/premake/premake5.exe`), or add a `!` exception.

## 5. Generating CMake from premake

- No CMake action is built in. `modules/` at v5.0.0 contains codelite, compilecommands, gmake, gmakelegacy, ninja, raw, self-test, vstudio and xcode. ([tree](https://github.com/premake/premake-core/tree/v5.0.0/modules))
- The official community module list points to [Jarod42/premake-cmake](https://github.com/Jarod42/premake-cmake) ("CMakeLists exporter for premake"). ([community/modules.md](https://github.com/premake/premake-core/blob/v5.0.0/website/community/modules.md))
  - Apache-2.0, not archived; last push 2026-05-07.
  - Recent commits include "Replace removed `flags` by the dedicated API" (2026-03-24) and "Updated to support the full premake5 API surface" (2026-04-15).
  - Usage: put the module in a `cmake/` folder on premake's search path, `require "cmake"`, then run `premake5 cmake`.
  - It is third-party, and its coverage of `openmp`, `externalincludedirs` and `uses` is unverified. Treat it as a cross-platform lead-in for later, not something to rely on now.
- Also new and built in: `premake5 compilecommands` (PR #2636) and a `ninja` exporter (PR #2554, moved into core in beta8). `compile_commands.json` is what clang-tidy needs, which is relevant to the warnings and static-analysis ticket on map #3.

## 6. Migration gotchas for this vcxproj (checklist)

1. **Drop Win32.** The vcxproj declares `Debug|Win32` and `Release|Win32`, but the TinyMAT libs are x64-only. Declare `platforms { "x64" }` (or `architecture "x86_64"`) only.
2. **Use `.slnx`.** Update `CLAUDE.md` to `msbuild space_traverse.slnx ...`. Delete the old `space_traverse.sln` so MSBuild never has to choose between two solution files.
3. **Use no `flags`.** Use `multiprocessorcompile`, `linktimeoptimization`, `symbols`, `fatalwarnings` and the other dedicated APIs. Copy-pasted 4.x/alpha examples will fail on 5.0.0.
4. **Use `buildoptions` for `/sdl` and `/utf-8`.** Neither has a dedicated vstudio API.
5. **Set `symbols "On"` in Release** to keep today's Release PDBs.
6. **Set `debugdir`** to `space_traverse/` as soon as `location` moves generated files. It lives in the `.vcxproj.user` file.
7. **Pin `targetdir` and `objdir`.** The `targetdir` doc says output defaults to "the same directory as the script". `CLAUDE.md` documents `x64/<Config>/space_traverse.exe`, so set something like `targetdir "x64/%{cfg.buildcfg}"` to keep that path, and `objdir` to a path under an ignored folder.
8. **Set `externalwarnings "Off"`** explicitly. When unset, the vs2019+ exporter writes `Level3`.
9. **Turn on `openmp`** in every project that compiles OpenMP code, test exe included.
10. **Carry `.filters` over with `vpaths`.** The existing `.vcxproj.filters` is replaced by premake's generated one; folder grouping follows the directory tree unless `vpaths` is set.
11. **Expect a new ProjectGuid.** Premake generates GUIDs deterministically from the project name unless `uuid` is set. That is harmless here, since nothing references the old GUID.

## Sources

- premake-core v5.0.0 release: https://github.com/premake/premake-core/releases/tag/v5.0.0
- premake-core v5.0.0-beta8 release: https://github.com/premake/premake-core/releases/tag/v5.0.0-beta8
- Source @v5.0.0: `modules/vstudio/{vs2026.lua, vs2026_solution.lua, vs2010_vcxproj.lua, vs2010_vcxproj_user.lua, _preload.lua}`, `src/_premake_init.lua`, `src/tools/msc.lua`, `LICENSE.txt`
- Docs @v5.0.0 (`website/docs`, published at https://premake.github.io/docs/): Using-Premake, toolset, flags, openmp, cppdialect, debugdir, externalincludedirs, externalwarnings, Linking, dependson, Usages-and-Uses, Workspaces-and-Projects, targetdir
- Community modules list: https://github.com/premake/premake-core/blob/v5.0.0/website/community/modules.md
- premake-cmake: https://github.com/Jarod42/premake-cmake
- Package-manager status: https://github.com/premake/premake-core/issues/2411
- MSBuild `.slnx` support: https://learn.microsoft.com/en-us/visualstudio/msbuild/msbuild-command-line-reference, https://devblogs.microsoft.com/visualstudio/new-simpler-solution-file-format/
