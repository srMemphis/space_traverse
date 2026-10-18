# Research: compiler warnings and static analysis (MSVC v145, premake 5.0.0, clang-tidy)

Ticket: #15 (child of map #3). Researched 2026-10-18. Research only: no recommendations; the decision belongs to the ticket that picks the warning level and checks. Each claim links to its source. Section 8 is a measurement on the current code (`dev` at `a177203`) with the current `.vcxproj`.

Source conventions:
- **PM** = premake-core at tag `v5.0.0` (commit `4f6dab3`): `https://github.com/premake/premake-core/blob/v5.0.0/...`
- **LLVM** = llvm-project at tag `llvmorg-22.1.3`: `https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/...`
- **[local VS]** = MSBuild files installed on this machine by Visual Studio 18 (2026) Community, under `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\...`, with MSVC tools 14.51.36231. These are primary product files, but they aren't published online, so they're cited by path. They reflect this one install.
- **UNVERIFIED** marks a claim that couldn't be traced to a primary source or measured.

## TL;DR

**Measured on the current code (§8):**
- **`/W3` (today) → `/W4` costs 2 new warnings in own code** (`C4100`, `C4189`) and 5 in Eigen (`C5054`). `/external:W0` on `space_traverse/include` removes the Eigen ones. The baseline is not clean: `/W3` already reports 16 warnings in `src/` (13 narrowing conversions, 2 non-UTF-8 files, 1 `IsRowMajor()` called as a function), and Debug adds 2 more that are known bugs from #6.
- **`/Wall`: 954 unique warnings, 791 of them outside `src/` (778 vendored, 13 STL/SDK).** With `/external:W0`, 162 remain in own code, mostly `C4365`/`C4388` signed/unsigned (98) and `C4514` (26).
- **MSVC `/analyze`** (the default `NativeRecommendedRules`) takes about 3× the Debug build time and finds 4 issues in own code. It also reports 25 in vendored headers, because `/external:W0` doesn't cover analysis and that needs `/analyze:external-`. It skips OpenMP regions (`C6993`).
- **clang-tidy 22.1.3, bundled with VS, runs on a hand-made cl-mode `compile_commands.json` with the project's MSVC flags.** But every TU hits a compile error in the bundled fmt 11.0.2 (spdlog 1.15.0), so **`clang-analyzer-*` silently finds nothing**. AST checks still work: 962 own-code diagnostics with a broad check set, mostly style.
- **Release doesn't link with the current toolset.** `TinyMAT_Release.lib` has `/GL` objects from another compiler version (`C1047`). This is independent of warnings and concerns #11 and #9.

**From the sources:**
- `/W4` gives level 1–4 warnings except the off-by-default ones. `/Wall` adds every off-by-default warning. Microsoft says the CRT and C++ standard library are meant to be warning-free only at `/W4`. ([MS: off-by-default](https://learn.microsoft.com/en-us/cpp/preprocessor/compiler-warnings-that-are-off-by-default))
- `/external:W0` doesn't affect back-end (mostly C47xx) warnings such as C4710 and C4711, and it doesn't affect code analysis. A library's own `#pragma warning(push, n)` still overrides `/external:Wn`. ([MS: /external](https://learn.microsoft.com/en-us/cpp/build/reference/external-external-headers-diagnostics))
- **Default `/external` template behavior:** a warning raised inside an external template that's instantiated from our code follows the external level, so `/external:W0` suppresses it. `/external:templates-` brings those warnings back when the instantiation happens in non-external code. ([MS: /external](https://learn.microsoft.com/en-us/cpp/build/reference/external-external-headers-diagnostics))
- **premake 5.0.0 warning APIs:**
  - The supported APIs are `warnings`, `externalwarnings`, `externalanglebrackets`, `externalincludedirs`, `disablewarnings`, `enablewarnings`, `fatalwarnings`, `linkerfatalwarnings`, `clangtidy` and `runcodeanalysis`.
  - `flags { "FatalWarnings" }` was removed. The replacement is `fatalwarnings { "All" }`. ([PM: flags.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/flags.md), [PM: fatalwarnings.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/fatalwarnings.md))
  - `warnings "Extra"` and `warnings "High"` both map to `Level4`. `warnings "Everything"` maps to `EnableAllWarnings` (`/Wall`).
- **No premake API for these settings:**
  - `/external:templates-` (`ExternalTemplatesDiagnostics`)
  - `/analyze:external-`
  - `ClangTidyChecks`
  - `EnableMicrosoftCodeAnalysis`
  - `CodeAnalysisRuleSet`

  The generic escape hatches are `vsprops` (MSBuild properties), `buildoptions` (raw switches) and `premake.override` of the vcxproj call arrays.
- **premake `compilecommands` action:** it's built in. It only accepts the clang, gcc, emcc and cosmocc toolsets, defaults to clang, and writes GNU-style `clang++` command lines (`-isystem`, `-std=c++20`, `-fopenmp`). It skips any project whose toolset resolves to `msc`. ([PM: compilecommands/_preload.lua](https://github.com/premake/premake-core/blob/v5.0.0/modules/compilecommands/_preload.lua#L46-L60))
- **VS clang-tidy integration:** VS builds its own cl.exe-style compile commands and adds `--target=amd64-pc-windows-msvc`. It strips `OpenMPSupport`, `SDLCheck` and `DisableSpecificWarnings` from those commands. `ClangTidyChecks` defaults to `clang-analyzer-*` when it's empty. On this machine VS 18 ships clang-tidy **LLVM 22.1.3**. [local VS]
- **clang-tidy 22 changes:** warnings from non-system headers now show by default (`--header-filter` defaults to `.*`), and system headers are no longer analyzed by default. In clang-cl mode `/external:I` becomes `-isystem`, so vendored headers given as external dirs count as system headers for clang-tidy. ([LLVM 22 release notes](https://releases.llvm.org/22.1.0/tools/clang/tools/extra/docs/ReleaseNotes.html), [LLVM Options.td L9128](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/clang/include/clang/Options/Options.td#L9128))
- **fmt 11.0.2 (bundled in spdlog 1.15.0):** it fails under clang ≥ 20 in C++20 mode, because clang implemented CWG2765 string-literal identity in constant evaluation. fmt 11.1.0 fixed it (#4177), and spdlog 1.15.1 bundles fmt 11.1.3. The Clang Static Analyzer doesn't run on a TU that has errors. (See section 7.)

---

## 1. MSVC warning levels and controls

### 1.1 Levels and switches
Source for this table: [MS: /w, /W0–/W4, /Wall, /wd, /we, /wo, /Wv, /WX](https://learn.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level).

| Switch | Semantics |
|---|---|
| `/W0` = `/w` | Suppresses all warnings. Warning pragmas in the source aren't affected by `/w`. |
| `/W1` | Command-line default. |
| `/W3` | IDE default. |
| `/W4` | Levels 1–4, but not warnings that are off by default. Microsoft recommends it for new projects as "lint-like". |
| `/Wall` | Everything in `/W4` plus every off-by-default warning. |
| `/WX` | Treats all compiler warnings as errors. The linker has its own `/WX`. |
| `/w1nnnn`…`/w4nnnn` | Sets the level for warning *nnnn*. Example: `/w34326` makes C4326 a level-3 warning, so it isn't reported under `/W2`. This is also how an off-by-default warning is enabled at a level ([MS: off-by-default](https://learn.microsoft.com/en-us/cpp/preprocessor/compiler-warnings-that-are-off-by-default)). |
| `/wdnnnn` | Suppresses the warning. |
| `/wennnn` | Treats one warning as an error. |
| `/wonnnn` | Reports the warning only once. |
| `/Wv:nn` | Shows only warnings introduced up to compiler version *nn*. |

Two more rules from the same page:
- **Mutual exclusion:** `/w1–4nnnn`, `/wd`, `/we` and `/wo` override each other, and the last one on the command line wins.
- **Precompiled headers:** the warning options used to build a PCH come back into effect when the PCH is used. Another option on the command line overrides them.

### 1.2 `#pragma warning`
Source: [MS: warning pragma](https://learn.microsoft.com/en-us/cpp/preprocessor/warning).
- **Specifiers:** `1`–`4` (assign a level), `default`, `disable`, `error`, `once` and `suppress` (next line only). VS 2022 17.14 added an optional `justification:` on `disable`/`suppress`, which appears in SARIF output.
- **Turning on an off-by-default warning:** both `default:` and a level specifier do this.
- **`push[, n]` / `pop`:** `push` saves the state of every warning. `push, n` also sets the global level to *n*.
- **Code-generation warnings (4700–4999):** the state in effect when the compiler reaches a function definition applies to the whole function body. A pragma inside the body takes effect only after the function ends.

### 1.3 `/sdl` (already used by this project)
Source: [MS: /sdl](https://learn.microsoft.com/en-us/cpp/build/reference/sdl-enable-additional-security-checks).
- **Warnings it makes errors:** C4146, C4308, C4532, C4533, C4700, C4703, C4789, C4995 and C4996. Each is the equivalent of `/we<n>`.
- **Other effects:** it adds runtime checks (strict `/GS`, pointer sanitization after `delete`, member-pointer initialization).

### 1.4 What `/Wall` adds, and how noisy it is
Source for the warning tables: [MS: Compiler warnings that are off by default](https://learn.microsoft.com/en-us/cpp/preprocessor/compiler-warnings-that-are-off-by-default) (updated 2026-04-15).

Microsoft's own framing on that page:
- These warnings are off because most developers don't find them useful. Some flag style choices or Microsoft extensions, and some flag real pitfalls.
- If all of them are enabled, some "may appear many times in library headers".
- The CRT and C++ standard library are intended to emit no warnings **only at `/W4`**.

Known-noisy warnings that `/Wall` turns on:

| Warning | Level | Message (abridged) | Notes |
|---|---|---|---|
| C4514 | 4 | unreferenced inline function has been removed | Fires for header inline functions that aren't used. |
| C4710 | 4 | function not inlined | C47xx code-generation warning, so `/external:W0` doesn't suppress it (§3.3). |
| C4711 | 1 | function selected for inline expansion | Informational. C47xx code-generation warning (§3.3). |
| C4820 | 4 | bytes padding added after member | Fires on most structs. |
| C5045 | (no level given) | compiler will insert Spectre mitigation if `/Qspectre` specified | Since VS 2017 15.7. |
| C4625 / C4626 | 4 | copy constructor / assignment operator implicitly deleted (base inaccessible) | |
| C5026 / C5027 | 1 and 4 | move constructor / move assignment implicitly deleted | |
| C4668 | 4 | symbol not defined as a macro, replaced with '0' in `#if` | Common in Windows SDK headers. |
| C4464 | 4 | relative include path contains '..' | |
| C4571 | 4 | catch(...) semantics changed since VC 7.1 | |
| C4774 | 4 | format string argument isn't a string literal | |

Off-by-default warnings that can be enabled one at a time on top of `/W4` (with `/w<L>nnnn` or `#pragma warning(<L>: nnnn)`). Levels are from the same page:

| Warning | Level | What it catches |
|---|---|---|
| C4061 | 4 | enumerator not explicitly handled by a case label (even when there's a `default`) |
| C4062 | 4 | enumerator not handled in a switch that has no `default` |
| C4242 / C4254 | 4 | narrowing conversion, possible loss of data |
| C4263 / C4264 / C4266 | 4 / 1 / 4 | virtual override or hiding problems |
| C4265 | 3 | class has virtual functions but its destructor isn't virtual |
| C4287 | 3 | unsigned/negative constant mismatch |
| C4296 | 4 | expression is always false |
| C4365 | 4 | signed/unsigned mismatch in a conversion |
| C4388 | 4 | signed/unsigned mismatch in a comparison |
| C4555 | 1 | expression has no effect |
| C4619 | 3 | `#pragma warning` names a warning number that doesn't exist |
| C4640 | 3 | construction of a local static isn't thread-safe |
| C4800 | 4 | implicit conversion to bool (re-added in 16.0) |
| C4826 | 2 | sign-extended conversion |
| C4868 | 4 | left-to-right evaluation order not guaranteed in a braced initializer list |
| C5031 / C5032 | 4 | `#pragma warning` push/pop mismatch |
| C5038 | 4 | data member initialized out of declaration order |
| C5039 | 4 | potentially throwing function passed to an `extern "C"` function under `-EHc` |
| C5204 | 3 | class with virtual functions has a non-virtual trivial destructor |
| C5219 | 2 | implicit conversion, possible loss of data |
| C5233 | 4 | explicit lambda capture isn't used |
| C5245 | 4 | unreferenced function with internal linkage removed |
| C5246 | 1 | subobject initialization should be wrapped in braces |
| C5262 | 1, error | implicit fall-through |
| C5263 | 4 | `std::move` on a temporary prevents copy elision |
| C5264 | 4 | const variable isn't used |
| C5266 | 4 | `const` on a return type has no effect |
| C5267 | 4 | implicit copy ctor/assign is deprecated because the class has a user-provided one |

Some warnings in the table are marked ^Perm^, for example C4471 and C4596. They're off unless `/permissive-` is set. This project has `ConformanceMode` on, which is `/permissive-`. ([same page](https://learn.microsoft.com/en-us/cpp/preprocessor/compiler-warnings-that-are-off-by-default))

**Trade-offs:**

| Approach | Pros | Cons |
|---|---|---|
| `/W4` + selected `/w<L>nnnn` | Targeted. Matches the level the STL is kept clean at. | The enabled list has to be maintained by hand. |
| `/Wall` + `/wd` list | Picks up new off-by-default warnings automatically. | Microsoft documents heavy header noise. C4710 and C4711 aren't silenced by `/external:W0` (§3.3). |

---

## 2. How premake 5.0.0 expresses warnings

### 2.1 API registrations
- **`warnings`:** a string, one of `Off`, `Default`, `High`, `Extra`, `Everything`. ([PM: src/_premake_init.lua L1009-L1020](https://github.com/premake/premake-core/blob/v5.0.0/src/_premake_init.lua#L1009-L1020))
- **`externalwarnings`:** the same five values. ([PM: L1090-L1101](https://github.com/premake/premake-core/blob/v5.0.0/src/_premake_init.lua#L1090-L1101))
- **`externalincludedirs`:** `list:directory`. ([PM: L1083-L1088](https://github.com/premake/premake-core/blob/v5.0.0/src/_premake_init.lua#L1083-L1088))
- **`disablewarnings` and `enablewarnings`:** `list:string`. ([PM: L264-L269](https://github.com/premake/premake-core/blob/v5.0.0/src/_premake_init.lua#L264-L269), [L301-L306](https://github.com/premake/premake-core/blob/v5.0.0/src/_premake_init.lua#L301-L306))
- **`fatalwarnings`:** `list:string` with the reserved value `"All"`. ([PM: L314-L321](https://github.com/premake/premake-core/blob/v5.0.0/src/_premake_init.lua#L314-L321))
  - `premake.hasFatalCompileWarnings` checks whether the list contains `"All"`.
  - `premake.filterFatalWarnings` removes `"All"` and leaves the warning numbers. ([PM: L1122-L1145](https://github.com/premake/premake-core/blob/v5.0.0/src/_premake_init.lua#L1122-L1145))
  - There's no `fatalcompilewarnings` API.
  - `linkerfatalwarnings` is a separate API that also accepts `"All"`. ([PM: linkerfatalwarnings.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/linkerfatalwarnings.md))
- **The old `flags` values:** `FatalWarnings`, `FatalCompileWarnings` and `FatalLinkWarnings` were deprecated in beta4 and removed in beta8, and the `flags` API itself is "removed as of 5.0.0". ([PM: flags.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/flags.md))
- **`externalanglebrackets`:** `On` or `Off`, registered by the vstudio module. ([PM: modules/vstudio/_preload.lua L178-L186](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/_preload.lua#L178-L186))
- **`clangtidy`, `runcodeanalysis`, `vsprops`:** also registered by the vstudio module. ([PM: L508-L525](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/_preload.lua#L508-L525))
- **`warnings` docs:** `High` and `Everything` are still marked "Needs documentation". ([PM: warnings.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/warnings.md))

### 2.2 vcxproj XML emitted (`modules/vstudio/vs2010_vcxproj.lua`)

The `vs2026` action defaults the toolset to `msc-v145` ([PM: vs2026.lua L35](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2026.lua#L35)). The oven copies the action toolset into `cfg.toolset` unless the script or `--cc` sets one ([PM: src/base/oven.lua L694](https://github.com/premake/premake-core/blob/v5.0.0/src/base/oven.lua#L694)).

| premake input | Emitted XML | Source |
|---|---|---|
| `warnings` unset or `Default` | `<WarningLevel>Level3</WarningLevel>`, always emitted | [L3715-L3718](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3715-L3718) |
| `warnings "Off"` | `<WarningLevel>TurnOffAllWarnings</WarningLevel>` | same |
| `warnings "High"` / `"Extra"` | `<WarningLevel>Level4</WarningLevel>` (`/W4`) | same |
| `warnings "Everything"` | `<WarningLevel>EnableAllWarnings</WarningLevel>` (`/Wall`) | same |
| `fatalwarnings { "All" }` | `<TreatWarningAsError>true</TreatWarningAsError>`, but only if `warnings ~= "Off"` | [L3657-L3661](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3657-L3661); test [L559-L568](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/tests/vc2010/test_compile_settings.lua#L559-L568) |
| `fatalwarnings { "4996", ... }` | `<TreatSpecificWarningsAsErrors>4996;...;%(TreatSpecificWarningsAsErrors)</TreatSpecificWarningsAsErrors>` (`/we`) | [L3673-L3681](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3673-L3681) |
| `disablewarnings { "4710", ... }` | `<DisableSpecificWarnings>4710;...;%(DisableSpecificWarnings)</DisableSpecificWarnings>` (`/wd`) | [L3664-L3670](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3664-L3670) |
| `enablewarnings { "4265" }` | No dedicated element. Appends `/w14265` to `<AdditionalOptions>` when the toolset starts with `msc`, which always uses **level 1**. | [L2177-L2179](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L2162-L2200) |
| `externalwarnings` unset | `<ExternalWarningLevel>Level3</ExternalWarningLevel>`, always emitted for vs2019+ (even with no external dirs) | [L3729-L3734](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3729-L3734); test [L1336](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/tests/vc2010/test_compile_settings.lua#L1325-L1339) |
| `externalwarnings "Off"` | `<ExternalWarningLevel>TurnOffAllWarnings</ExternalWarningLevel>` (`/external:W0`) | same |
| `externalwarnings "High"/"Extra"/"Everything"` | `Level4`; there's no `/external:Wall` | same |
| `externalanglebrackets "On"/"Off"` | `<TreatAngleIncludeAsExternal>true/false</TreatAngleIncludeAsExternal>` | [L3765-L3773](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3765-L3773) |
| `externalincludedirs { ... }` | `<ExternalIncludePath>dirs;$(ExternalIncludePath)</ExternalIncludePath>` in the per-config output PropertyGroup (VC++ Directories), not in `<ClCompile>`. `includedirsafter` goes there too. | [L2906-L2916](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L2906-L2916) |
| `clangtidy "On"` | `<EnableClangTidyCodeAnalysis>true</EnableClangTidyCodeAnalysis>` in the output PropertyGroup | [L3532-L3536](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3532-L3536), list [L372-L397](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L372-L397) |
| `runcodeanalysis "On"` | `<RunCodeAnalysis>true</RunCodeAnalysis>` in the same group | [L3538-L3542](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3538-L3542) |
| (nothing) | No element is emitted for `ExternalTemplatesDiagnostics`, `DisableAnalyzeExternal`, `ClangTidyChecks`, `EnableMicrosoftCodeAnalysis` or `CodeAnalysisRuleSet`. A grep of the vcxproj generator, `msc.lua` and the vstudio `_preload.lua` at v5.0.0 finds none of these names. | — |

**MSVC command-line mapping** (used by non-VS exporters such as gmake/ninja with `msc`) ([PM: src/tools/msc.lua L79-L152](https://github.com/premake/premake-core/blob/v5.0.0/src/tools/msc.lua#L79-L152), [L585-L601](https://github.com/premake/premake-core/blob/v5.0.0/src/tools/msc.lua#L585-L601)):
- **Warning level:** `Off`→`/W0`, `High`/`Extra`→`/W4`, `Everything`→`/Wall`.
- **Fatal warnings:** `fatalwarnings All`→`/WX`.
- **External warnings:** `Off`→`/external:W0`, `Default`→`/external:W3`, `High`/`Extra`/`Everything`→`/external:W4`.
- **Angle brackets:** `On`→`/external:anglebrackets`.
- **Per-warning lists:** `enablewarnings`→`/w1"n"`, `disablewarnings`→`/wd"n"`, specific `fatalwarnings`→`/we"n"`.
- **External include dirs:** emitted as `/external:I` for toolset `msc-v142` and later. ([L341-L365](https://github.com/premake/premake-core/blob/v5.0.0/src/tools/msc.lua#L341-L365))
- **Automatic `/external:W3`:** added when external dirs exist but `externalwarnings` is unset, to avoid warning D9007. ([L197-L208](https://github.com/premake/premake-core/blob/v5.0.0/src/tools/msc.lua#L197-L208))

**Escape hatches:**
- **`vsprops { Name = "value" }`** emits arbitrary elements into the per-configuration conditioned `Globals` PropertyGroup ([PM: L188-L193](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L188-L193), [L3600-L3615](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3600-L3615), [vsprops.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/vsprops.md)).
  - These are MSBuild properties only. `<ClCompile>` item metadata can't be set this way.
  - The group is written before `Microsoft.Cpp.Default.props`. The VS defaults for `RunCodeAnalysis`, `EnableMicrosoftCodeAnalysis`, `EnableClangTidyCodeAnalysis` and `ClangTidyChecks` are all conditional on `'$(X)' == ''`, so values set earlier are kept. [local VS: `VC\v180\Microsoft.Cpp.Analysis.props` L17-L20, `Microsoft.Cpp.ClangTidy.props`]
- **`buildoptions { "/external:templates-" }`** adds raw switches to `<AdditionalOptions>`.
- **`premake.override(premake.vstudio.vc2010.elements, "clCompile", ...)`** adds new elements. This is the documented override and call-array mechanism. ([PM: Overrides-and-Call-Arrays.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/Overrides-and-Call-Arrays.md))

---

## 3. Isolating vendored code in MSVC (`/external:*`)

Primary source for this section: [MS: /external (External headers diagnostics)](https://learn.microsoft.com/en-us/cpp/build/reference/external-external-headers-diagnostics). The page's date metadata is 2022-06-29.

### 3.1 Switches

| Switch | Semantics |
|---|---|
| `/external:I path` | `path` and all its subdirectories are external. Only `path` itself is added to the include search. |
| `/external:env:VAR` | Every directory in the semicolon-separated env var is external, for example `/external:env:INCLUDE`. |
| `/external:anglebrackets` | Every `#include <...>` is external. |
| `/external:W0`…`W4` | Warning level for external headers. Without it, the other `/external` options are ignored and the compiler issues D9007. The effect is "similar to" `#pragma warning(push, 0)` / `#include` / `pop`. |
| `/external:templates-` | Re-allows warnings from external headers when they occur in a template instantiated in your code. |

**Version:**
- Available since VS 2017 15.6.
- Before VS 2019 16.10 it also needed `/experimental:external`; it doesn't any more.
- v145 (VS 2026) is well past that.

**IDE / MSBuild mapping** (switch names from [local VS: `VC\v180\1049\cl.xml`]):

| IDE property | MSBuild name | Switch |
|---|---|---|
| External Include Directories | `ExternalIncludePath` | `/external:I` |
| Treat Files Included with Angle Brackets as External | `TreatAngleIncludeAsExternal` | `/external:anglebrackets` |
| External Header Warning Level | `ExternalWarningLevel` | `TurnOffAllWarnings`/`Level1`…`Level4`/`InheritWarningLevel` = `/external:W0`…`W4`/none |
| Template Diagnostics in External Headers | `ExternalTemplatesDiagnostics` | `/external:templates-` |
| Disable Code Analysis for External Headers | `DisableAnalyzeExternal` | `/analyze:external-` |
| Analysis Ruleset for External Headers | `AnalyzeExternalRuleset` | `/analyze:external:ruleset` |

The MS page warns that if External Header Warning Level is "Inherit Project Warning Level" or default, the other `/external` options are ignored. In the installed props, `InheritWarningLevel` is the default value of `ExternalWarningLevel`. [local VS: `Microsoft.Cl.Common.props` L130-L135]

### 3.2 The template rule: are Eigen warnings in our instantiations suppressed?
From the "Warnings across the internal and external boundary" section of the MS page:

- **Default (no `/external:templates-`):** the external warning level applies to warnings raised inside templates defined in external headers, even when the instantiation is in our code. The doc says a low external level "can turn off warnings emitted on template instantiations in user code". So with `/external:W0`, a C4244 raised inside an Eigen template instantiated from `src/` **is suppressed**.
- **With `/external:templates-`:** "The compiler considers both the effective warning level in the file that defines the template, and the warning level where template instantiation occurs. Warnings emitted inside an external template appear if the template is instantiated within non-external code." So Eigen-internal warnings come back whenever our (non-external) code triggers the instantiation.
- Warnings that reappear because of `/external:templates-` can be silenced at the instantiation site with `#pragma warning(suppress: n)`.

### 3.3 Interactions

| Interaction | Behavior (source: MS /external page unless noted) |
|---|---|
| `#pragma warning` inside external headers | Still applies. A header's own `push, n` / `error: n` overrides `/external:Wn`, and "the `/external` options don't override that deliberate choice." TinyMAT does `#pragma warning(push, 0)` around its includes (`include/TinyMAT/tinymatwriter.h` L45-L47). Eigen 3.4.0 pushes and disables `4100 4101 4181 4211 4244 4273 4324 4503 4512 4522 4700 4714 4717 4800` (`include/Eigen/src/Core/util/DisableStupidWarnings.h`). spdlog pushes and disables 4702 and 4127 locally. |
| `/Wall` | `/Wall` sets the level for non-external files. External headers use `/external:Wn` (`W0`…`W4`; there's no external `/Wall`). |
| Back-end warnings | "Some warnings emitted by the compiler's back-end code generation aren't affected by the `/external` options. These warnings usually start with C47XX". They still need `/wd47XX`. That covers C4710 and C4711 from `/Wall`. Which other C47xx warnings fire for Eigen with this project's code is **UNVERIFIED**. |
| `/WX` | The docs don't describe the interaction directly. Inference: `/WX` promotes warnings that are emitted, and warnings suppressed by `/external:W0` aren't emitted. Back-end C47xx warnings and warnings forced on by header pragmas are still emitted, so `/WX` would turn them into errors. **Inference, not stated verbatim.** |
| Code analysis | "Code analysis warnings are also unaffected, since they don't have warning levels." From [MS: /analyze](https://learn.microsoft.com/en-us/cpp/build/reference/analyze-code-analysis): `/external:Wn` doesn't affect code analysis. `/analyze:external-` skips external files except templates treated as non-external via `/external:templates-`. `/analyze:external:ruleset` applies a different ruleset. The `CAExcludePath` env var excludes directories, and wins when a directory is in both. |
| `/analyze:WX-` | Code analysis warnings aren't treated as errors even under `/WX` ([MS: /analyze](https://learn.microsoft.com/en-us/cpp/build/reference/analyze-code-analysis)). |
| MSBuild fixup | If `EnablePREfast` isn't true, the targets clear `DisableAnalyzeExternal` and `AnalyzeExternalRuleset`. [local VS: `Microsoft.CppCommon.targets` L503-L512] |

### 3.4 How this project includes vendored code
- Includes use angle brackets: `<Eigen/Dense>`, `<spdlog/spdlog.h>`, and `<spdlog\spdlog.h>` with a backslash in `src/debug/Log.h:8`.
- Both `/external:I space_traverse/include` and `/external:anglebrackets` would therefore cover them.
- `/external:anglebrackets` would also mark the Windows SDK and STL as external. That's observed from the repo's sources; the consequence is from the MS doc's definition.

---

## 4. Visual Studio Code Analysis (MSVC `/analyze` + clang-tidy)

### 4.1 MSBuild properties (from installed VS 18 targets)
[local VS] `VC\v180\Microsoft.Cpp.Analysis.props` L17-L20, `Microsoft.Cpp.Analysis.targets`, `Microsoft\VisualStudio\v18.0\CodeAnalysis\Microsoft.CodeAnalysis.Targets` L39-L70:

| Property | Default | Effect |
|---|---|---|
| `RunCodeAnalysis` | `false` | When `true`, `RunCppAnalysis=true` for C++ projects. Analysis then runs as part of the build. |
| `EnableMicrosoftCodeAnalysis` | `true` | `RunMsvcAnalysis = RunCppAnalysis`, unless this is `false` or the toolset is clang-cl (`/analyze`, PREfast). |
| `EnableClangTidyCodeAnalysis` | `false` | When `true` together with `RunCppAnalysis`, the `ClangTidy` target runs after `ClCompile`. |
| `CodeAnalysisRuleSet` | `NativeRecommendedRules.ruleset` for C++ (`NativeMinimumRules` on Express SKUs) | Microsoft analysis ruleset. ([MS: Core Guidelines checkers](https://learn.microsoft.com/en-us/cpp/code-quality/using-the-cpp-core-guidelines-checkers): Native Recommended includes a subset of the Core Check rules.) |
| `EnableCppCoreCheck` (and `EnableEnumIndex`, `EnableHResultCheck`, `EnableVariantClear`, `EnableConcurrencyCheck`) | on unless `false` | EspXEngine extensions added when `RunMsvcAnalysis` is on. [local VS: `Microsoft.CodeAnalysis.Extensions.props`] |

Other behavior from the same files:
- **Both tools disabled:** if `RunCodeAnalysis` is on but both tools are disabled, the build warns MSB8061.
- **clang-cl toolset:** Microsoft analysis with the clang-cl toolset is error MSB8060.

**Command line:** Microsoft documents `msbuild /p:EnableCppCoreCheck=true /p:RunCodeAnalysis=true /p:CodeAnalysisRuleSet=CppCoreCheckRules.ruleset`. It also says properties set in the project must come before the `Microsoft.Cpp.targets` import. ([MS: Core Guidelines checkers, "MSBuild"](https://learn.microsoft.com/en-us/cpp/code-quality/using-the-cpp-core-guidelines-checkers))
- Shipped Core Check rule sets include `CppCoreCheckRules` and per-area sets (Arithmetic, Bounds, Class, Const, Type, …). ([same](https://learn.microsoft.com/en-us/cpp/code-quality/using-the-cpp-core-guidelines-checkers))
- That `/p:RunCodeAnalysis=true /p:EnableClangTidyCodeAnalysis=true` from command-line msbuild also runs clang-tidy follows from the targets above, which aren't IDE-gated: `ClangTidyAfterTargets` gets `ClCompile` whenever `RunClangCppAnalysis` is true. **Not executed in this research.**

### 4.2 clang-tidy inside VS
Sources: [MS: Using Clang-Tidy in Visual Studio](https://learn.microsoft.com/en-us/cpp/code-quality/clang-tidy) (updated 2025-06-20) and [local VS: `Microsoft.Cpp.ClangTidy.props` / `.targets`].

- **Toolsets and install:**
  - It's supported since VS 2019 16.4 for MSBuild and CMake, with both the MSVC and clang toolsets.
  - It's installed automatically with a C++ workload.
  - It runs "after successful compilation".
- **Checks:** the "Clang-Tidy Checks" property is passed as `--checks`. Further configuration can live in custom `.clang-tidy` files ([MS](https://learn.microsoft.com/en-us/cpp/code-quality/clang-tidy)). The installed props set `ClangTidyChecks` to `clang-analyzer-*` when it's empty. [local VS]
- **Additional properties:** Tool Directory, Additional Options (`--extra-args`), Prepend Additional Options (`--extra-args-before`) and Max Number of Processes. The last three were added in VS 2026 18.0. ([MS](https://learn.microsoft.com/en-us/cpp/code-quality/clang-tidy), [C++ blog 18.0, 2025-11-11](https://devblogs.microsoft.com/cppblog/whats-new-for-cpp-developers-in-visual-studio-2026-version-18-0/))
- **Binary:** `ClangTidyToolPath` defaults to `$(VsInstallRoot)\VC\Tools\Llvm\x64\bin` (or `$(LLVMInstallDir)\bin` when that's set), and the exe defaults to `clang-tidy.exe`.
  - On this machine that binary reports **LLVM 22.1.3**, and `VC\Tools\Llvm\x64\bin` contains only `clang-format.exe` and `clang-tidy.exe`.
  - **No Microsoft document states the bundled LLVM version for VS 2026.** The 18.0 blog post doesn't mention it.
  - MS docs only say the bundled LLVM follows major LLVM releases and is shipped unmodified from the LLVM release page. ([MS: Clang/LLVM support](https://learn.microsoft.com/en-us/cpp/build/clang-support-msbuild))
- **How VS feeds MSVC flags to clang-tidy:** the `GetCompileCommands` target runs the `CLCommandLine` task over `ClCompile` items to produce **cl.exe-style** command lines. It prepends `ClangTidyAdditionalOptions`, which is `--target=amd64-pc-windows-msvc` for x64. Before that, it **removes** this metadata from the commands: `OpenMPSupport`, `SDLCheck`, `DisableSpecificWarnings`, `TreatSpecificWarningsAsErrors`, `PrecompiledHeader*`, `SpectreMitigation`, `MultiProcessorCompilation`, `EnablePREfast`/`PREfast*`, `TreatWChar_tAsBuiltInType`, `UseFullPaths` and others.
  - It **keeps** `WarningLevel`, `TreatWarningAsError`, `AdditionalOptions`, `LanguageStandard` and `ConformanceMode`. [local VS: `Microsoft.Cpp.ClangTidy.targets` L58-L232]
  - Inference: OpenMP pragmas aren't enabled for clang-tidy under the VS integration unless `/openmp` is passed through `AdditionalOptions`.
- **Header filtering properties:** `ClangTidyHeaderFilter` (empty by default) and `ClangTidyWarningsInSystemHeaders` (`false`) are passed to the task. They aren't in the property pages (`1049\clangtidy.xml` lists only `ClangTidyChecks`, `ClangTidyExtraArgs`, `ClangTidyPrependExtraArgs`, `ClangTidyToolPath`, `MaxNumberOfProcesses`). [local VS]
- **Exactly how the task builds the clang-tidy command line** (for example whether `HeaderFilter` maps to `--header-filter`, and where the generated compile database goes) lives in `Microsoft.Build.CppTasks.Common.dll`. **UNVERIFIED.**

### 4.3 premake 5.0.0 support

| Need | premake 5.0.0 | Source |
|---|---|---|
| `<EnableClangTidyCodeAnalysis>` | `clangtidy "On"` (vs2019+) | [PM: clangtidy.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/clangtidy.md) |
| `<RunCodeAnalysis>` | `runcodeanalysis "On"` (vs2019+) | [PM: runcodeanalysis.md](https://github.com/premake/premake-core/blob/v5.0.0/website/docs/runcodeanalysis.md) |
| `EnableMicrosoftCodeAnalysis`, `ClangTidyChecks`, `CodeAnalysisRuleSet`, `EnableCppCoreCheck`, `ClangTidyHeaderFilter` | No API. Can be set with `vsprops` (MSBuild properties; §2.2). | grep of v5.0.0 sources |
| `/analyze:external-`, `/external:templates-` | No API. Can be set with `buildoptions` (AdditionalOptions) or `premake.override` of `m.elements.clCompile` to emit `<DisableAnalyzeExternal>` or `<ExternalTemplatesDiagnostics>`. | §2.2 |
| Run clang-tidy only via CLI (`msbuild /p:RunCodeAnalysis=true ...`) | Needs no premake support. Properties on the command line override the project. | MS doc above |

---

## 5. Standalone clang-tidy with `compile_commands.json`

### 5.1 premake's built-in `compilecommands` action
- **Action definition:** the action name is `compilecommands`. Options are `--cc-config`, `--cc-platform`, `--cc-output` (default `compile_commands.json`) and `--implicit-includes=On|Off` (default `On`). ([PM: modules/compilecommands/_preload.lua L10-L60](https://github.com/premake/premake-core/blob/v5.0.0/modules/compilecommands/_preload.lua#L10-L60))
- **Toolset limits:** `valid_tools.cc = { "clang", "cosmocc", "emcc", "gcc" }` and `toolset = "clang"`. **`msc` isn't supported.**
  - `action.supportsToolset` returns false for a project whose explicit toolset is `msc*`, and `generateproject` then warns "unsupported toolset ... skipping". ([PM: src/base/action.lua L322-L360](https://github.com/premake/premake-core/blob/v5.0.0/src/base/action.lua#L322-L360), [compilecommands.lua](https://github.com/premake/premake-core/blob/v5.0.0/modules/compilecommands/compilecommands.lua))
  - A project with no explicit `toolset` gets the action default, `clang`. ([PM: oven.lua L694](https://github.com/premake/premake-core/blob/v5.0.0/src/base/oven.lua#L694))
- **Command lines it writes:**
  - The `arguments` array is `{ toolname, <toolset cxxflags>, <defines>, <undefines>, <include flags>, <forceincludes>, <buildoptions>, file, "-o", obj }`.
  - It writes `arguments`, not `command`, plus `directory = prj.location`, `file` and `output`.
  - The tool name is `clang++` for C++. ([PM: compilecommands.lua L59-L176, L245-L265](https://github.com/premake/premake-core/blob/v5.0.0/modules/compilecommands/compilecommands.lua); [clang.lua L463-L483](https://github.com/premake/premake-core/blob/v5.0.0/src/tools/clang.lua#L463-L483))
  - With the clang toolset, the flags are GCC-style:
    - `externalincludedirs`→`-isystem`
    - `includedirsafter`→`-idirafter`
    - `openmp "On"`→`-fopenmp`
    - `warnings High`→`-Wall`, `Extra`→`-Wall -Wextra`, `Everything`→`-Weverything`
    - `cppdialect "C++20"`→`-std=c++20`
    - ([PM: gcc.lua L96-L97, L135-L146, L271, L380-L400](https://github.com/premake/premake-core/blob/v5.0.0/src/tools/gcc.lua); clang.lua reuses these)
  - Consequence: anything an MSVC build adds through unfiltered `buildoptions` (`/utf-8`, `/sdl`, `/permissive-`) is copied verbatim into a GNU-mode `clang++` line. Options can be scoped with `filter "toolset:msc*"` or `filter "action:vs*"`. What clang++'s GNU driver does with a `/utf-8` argument (likely treated as an input path) is **UNVERIFIED**.
- **Implicit includes:** with `--implicit-includes=On` (the default), premake runs `clang++ -v -E -x c++ nul` to discover system include dirs and adds them as `-isystem`. If the command fails, it warns and adds none. ([PM: gcc.lua L413-L455](https://github.com/premake/premake-core/blob/v5.0.0/src/tools/gcc.lua#L413-L455)) The VS-bundled LLVM directory on this machine has no `clang++.exe`. [local]
- **Alternative:** emit cl-mode lines (`cl.exe` / `clang-cl` as `arguments[0]`) with a custom Lua script or `premake.override`. premake 5.0.0 doesn't ship this.

### 5.2 How clang tooling reads MSVC-style command lines
- **Driver mode from `arguments[0]`:**
  - The program name is normalized (a `.exe` suffix is stripped).
  - The suffixes `clang-cl` and `cl` map to `--driver-mode=cl`, and `clang++` maps to `--driver-mode=g++`. ([LLVM: clang/lib/Driver/ToolChain.cpp L428-L505](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/clang/lib/Driver/ToolChain.cpp#L428-L505))
  - `--driver-mode=cl` can also be forced with `--extra-arg-before`.
- **`command` strings:** on Windows hosts they're tokenized with Windows rules (AutoDetect). ([LLVM: JSONCompilationDatabase.cpp L130-L150](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/clang/lib/Tooling/JSONCompilationDatabase.cpp#L130-L150))
- **clang-cl handling of this project's flags** ([LLVM: clang/include/clang/Options/Options.td](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/clang/include/clang/Options/Options.td); line numbers below):

| cl flag | clang-cl behavior |
|---|---|
| `/openmp` | Alias of `-fopenmp` (L9336). `/openmp-` → `-fno-openmp`. `/openmp:experimental` → `-fopenmp`. |
| `/external:I dir` | Alias of `-isystem` (L9128): "warnings suppressed". |
| `/external:env:VAR` | Adds the dirs as system includes (L9376; [MSVC.cpp L676-L680](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/clang/lib/Driver/ToolChains/MSVC.cpp#L676-L680)). |
| `/external:W0` | `-Wno-system-headers`. `/external:W1`–`W4` → `-Wsystem-headers` (L9240-L9244). |
| `/external:anglebrackets`, `/external:templates-` | Not defined individually. They match the catch-all `_SLASH_external : CLJoined<"external:">` in the "Unsupported" CL block (L9569). The UsersManual says such options are "ignored with a warning" ([UsersManual, clang-cl](https://releases.llvm.org/22.1.0/tools/clang/docs/UsersManual.html#clang-cl)). |
| `/utf-8` | Ignored; UTF-8 is the default (L9539). |
| `/sdl`, `/sdl-` | Ignored (L9537-L9538). |
| `/permissive-` | Translated by `TranslatePermissiveMinus` ([MSVC.cpp L1016-L1021](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/clang/lib/Driver/ToolChains/MSVC.cpp#L1016-L1021)). |
| `/std:c++20` | `-std=c++20`. `c++23preview`→`-std=c++23`, `c++latest`→`-std=c++26` ([Clang.cpp L7225-L7233](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/clang/lib/Driver/ToolChains/Clang.cpp#L7225-L7233)). |
| `/W1`–`/W3` | `-Wall` (L9246-L9248). |
| `/W4` | `-Wall -Wextra` (L9249). |
| `/Wall` | `-Weverything` (L9250). |
| `/WX` | `-Werror` (L9252). |
| `/wdNNNN` | Mapped to `-Wno-<group>` only when clang knows the MSVC warning ID; otherwise dropped ([Clang.cpp L6339-L6356](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/clang/lib/Driver/ToolChains/Clang.cpp#L6339-L6356)). |
| `/w14265`, `/we4996`, `/wo…` | Swallowed by `CLIgnoredJoined<"w">` (L9541): silently ignored. |
| `/Zc:__cplusplus` | Ignored (L9543). |
| `/analyze-` | Ignored (L9525). |

- **`/WX` in the compile commands:** with clang's `-Werror`, clang warnings become errors in clang-tidy's parse. The Static Analyzer then won't run on that TU (§7.2). Inference from the two sources; not executed.
- **OpenMP in clang-tidy:** `#pragma omp` is parsed only when `-fopenmp` is in effect (`/openmp` in cl mode). The only active OpenMP region is `#pragma omp parallel for` in `src/particles/Beam.cpp:277`; `Mesh.cpp:894` is commented out, and no file includes `omp.h`. In the §8 run with `/openmp`, `Beam.cpp` produced no OpenMP-related errors.

### 5.3 clang-tidy options for keeping vendored headers out
Source: [clang-tidy 22.1.0 docs](https://releases.llvm.org/22.1.0/tools/clang/tools/extra/docs/clang-tidy/index.html).

| Option / config key | Meaning |
|---|---|
| `-p <build-path>` | Directory that holds `compile_commands.json`. If it's omitted, clang-tidy searches parent dirs of the first input. |
| `--extra-arg` / `--extra-arg-before` | Append or prepend a compiler argument. |
| `--removed-arg` / `RemovedArgs` | New in clang-tidy 22: removes arguments from the compile command ([22 release notes](https://releases.llvm.org/22.1.0/tools/clang/tools/extra/docs/ReleaseNotes.html)). |
| `--header-filter` / `HeaderFilterRegex` | Regex for headers whose diagnostics are shown. **Default `.*` in 22** (all non-system headers); an empty string disables header warnings. This changed in 22 ([release notes](https://releases.llvm.org/22.1.0/tools/clang/tools/extra/docs/ReleaseNotes.html)). |
| `--exclude-header-filter` / `ExcludeHeaderFilterRegex` | Excludes headers. Added in clang-tidy 19 ([19 release notes](https://releases.llvm.org/19.1.0/tools/clang/tools/extra/docs/ReleaseNotes.html)). The 22 docs say it's used together with `--header-filter`. |
| `--system-headers` / `SystemHeaders` | Show diagnostics from system headers. In 22, system headers also aren't *analyzed* by default, for performance ([22 release notes](https://releases.llvm.org/22.1.0/tools/clang/tools/extra/docs/ReleaseNotes.html)). |
| `.clang-tidy` discovery | Taken from the closest parent directory of each source file. `InheritParentConfig: true` layers it on top of parent configs. `--config` / `--config-file` override discovery. |
| `run-clang-tidy.py` | Runs over the whole compile database in parallel; all cores by default since 22. `-source-filter` was added in 19. |
| Other 22 breaking change | The release notes say the `clang-analyzer-*` checks are no longer enabled by default and have to be requested explicitly. VS's own default `ClangTidyChecks=clang-analyzer-*` does that ([22 release notes](https://releases.llvm.org/22.1.0/tools/clang/tools/extra/docs/ReleaseNotes.html), [local VS]). |

Main-file diagnostics are always shown regardless of the header filters ([22 docs](https://releases.llvm.org/22.1.0/tools/clang/tools/extra/docs/clang-tidy/index.html)).

**Trade-off summary:**

| Route | Pros | Cons |
|---|---|---|
| VS integration (`clangtidy`/`runcodeanalysis` + `vsprops`) | Uses VS's own cl-style commands and the bundled clang-tidy. Shows results in the Error List. | The command-line construction is in a closed DLL. VS strips `OpenMPSupport`. Configuration lives in MSBuild properties. |
| premake `compilecommands` + `clang-tidy -p` / `run-clang-tidy.py` | Portable. Can be scripted outside VS. | Emits GCC-style `clang++` lines rather than the MSVC flags actually built. Projects with explicit `toolset "msc*"` are skipped. MSVC-only `buildoptions` must be filtered out. |
| Hand-written or overridden cl-mode database | Matches the real MSVC flags through clang-cl translation. | Custom code to maintain. Some MSVC flags are ignored or dropped (table above). |

---

## 6. Known gotchas

1. **C4710/C4711 with `/Wall` + `/WX`.** These are back-end C47xx warnings, which `/external:W0` doesn't suppress ([MS /external, Limitations](https://learn.microsoft.com/en-us/cpp/build/reference/external-external-headers-diagnostics)). Under `/Wall` they can come from inlining decisions on Eigen and spdlog code. Only `/wd` or pragmas suppress them.
2. **Library pragmas outrank `/external:Wn`.** A vendored header can force its own level or errors ([MS /external](https://learn.microsoft.com/en-us/cpp/build/reference/external-external-headers-diagnostics)). Eigen 3.4.0 disables a fixed list and restores it with `pop` at the end of each module (`DisableStupidWarnings.h` / `ReenableStupidWarnings.h`). Warnings raised later, when those templates are instantiated, follow §3.2. How MSVC combines the pragma state at template definition with `/external:templates-` for Eigen in particular is **UNVERIFIED**.
3. **`/external:templates-` brings Eigen warnings back into our TUs.** With it on, Eigen-internal warnings (for example conversions) that our instantiations trigger are reported again, and with `/WX` they're errors. Without it, they're hidden even when the cause is a type we passed in ([MS /external](https://learn.microsoft.com/en-us/cpp/build/reference/external-external-headers-diagnostics)).
4. **premake always emits `<ExternalWarningLevel>Level3</ExternalWarningLevel>` for vs2019+** when `externalwarnings` isn't set. External headers then get `/external:W3`, not the project level ([PM test L1336](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/tests/vc2010/test_compile_settings.lua#L1325-L1339)).
5. **`enablewarnings` always uses level 1 in the vcxproj path (`/w1nnnn`).** The warning is reported at any `/W` of 1 or higher ([PM L2179](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L2162-L2200)).
6. **`fatalwarnings { "All" }` is silently dropped when `warnings "Off"` is set** ([PM L3657-L3661](https://github.com/premake/premake-core/blob/v5.0.0/modules/vstudio/vs2010_vcxproj.lua#L3657-L3661)).
7. **`/sdl` is already `/we` for nine warnings** (§1.3). A C4996 from a vendored header still matters, because `/external:W0` suppresses the warning before it can become an error. Inference from §3.3.
8. **The spdlog/fmt consteval failure under clang-tidy.** See §7.
9. **spdlog's `/utf-8` handling.** spdlog 1.15.1's notes include "Pass `/utf-8` only when the compiler is MSVC" (CMake packaging; not relevant to vendored headers) ([spdlog v1.15.1 release](https://github.com/gabime/spdlog/releases/tag/v1.15.1)). The fmt headers in MSVC builds need `/utf-8` for Unicode support. That isn't documented in the sources read here: **UNVERIFIED**.

---

## 7. fmt/spdlog under clang-tidy

Symptom observed in the §8 trial run:
- **Setup:** VS-bundled clang-tidy 22.1.3, cl mode, `/std:c++20`, spdlog 1.15.0 with bundled fmt 11.0.2.
- **Error:** `call to consteval function 'fmt::basic_format_string<...>::basic_format_string<FMT_COMPILE_STRING, 0>' is not a constant expression`.
- **Locations:** every `FMT_STRING` use.
- **Workaround observed:** the error goes away with `/std:c++17`.

### 7.1 Cause and fix (primary sources)
- **Vendored versions:**
  - The vendored spdlog is 1.15.0 (`include/spdlog/version.h`).
  - The bundled fmt is `FMT_VERSION 110002`. Its `base.h` is byte-identical to fmt tag `11.0.2`, checked by diff against [fmt base.h@11.0.2](https://github.com/fmtlib/fmt/blob/11.0.2/include/fmt/base.h).
- **Root cause, fmt [#4177](https://github.com/fmtlib/fmt/issues/4177)** ("Format string checking incorrectly assumes repeated evaluation of string literal will produce the same pointer", opened 2024-09-27):
  - The `basic_format_string` constructor converts `s` to a string view twice.
  - Clang gained a stricter implementation of the rule that each evaluation of a string literal may produce a different object, and now rejects this pattern.
  - In 11.0.2, `str_(s)` is followed by `parse_format_string<true>(str_, checker(s))` ([base.h@11.0.2 L2873-L2885](https://github.com/fmtlib/fmt/blob/11.0.2/include/fmt/base.h#L2860-L2890)).
- **The clang change:** [llvm/llvm-project#109208](https://github.com/llvm/llvm-project/pull/109208), "[clang] implement current direction of CWG2765 for string literal comparisons in constant evaluation", merged 2024-09-26. The merge commit is in `llvmorg-20.1.0` and not in `llvmorg-19.1.7` (GitHub compare). So clang ≥ 20 is affected, which includes VS's 22.1.3.
- **fmt fix:** commit [`cacc310`](https://github.com/fmtlib/fmt/commit/cacc3108c5b74020dba7bf3c6d3a7e58cdc085b2) (2024-09-28).
  - It's contained in tag `11.1.0` (released 2024-12-25) and not in `11.0.2` (GitHub compare).
  - The 11.1.0 ChangeLog lists #4177 ([ChangeLog.md@11.1.0](https://github.com/fmtlib/fmt/blob/11.1.0/ChangeLog.md)).
  - Duplicate reports that the maintainer resolved as "fixed in #4177": [#4247](https://github.com/fmtlib/fmt/issues/4247) (fmt 11.0.2 + clang 20 + C++20) and [#4264](https://github.com/fmtlib/fmt/issues/4264). The spdlog-side duplicate is [gabime/spdlog#3306](https://github.com/gabime/spdlog/issues/3306) ("This is fmt issue and already fixed fmtlib/fmt#4177").
- **spdlog releases that carry the fix:**

  | spdlog | Bundled fmt |
  |---|---|
  | v1.15.0 | fmt 11.0.2 (`110002`) |
  | v1.15.1 | fmt 11.1.3 (`110103`); "Update bundled fmt version to 11.1.3" ([release notes](https://github.com/gabime/spdlog/releases/tag/v1.15.1)) |
  | v1.15.2 | `110104` |
  | v1.15.3 | `110200` |

  Versions were read from `include/spdlog/fmt/bundled/base.h` at each tag.
- **Why `/std:c++17` avoids it:** in fmt 11.0.2, `FMT_USE_CONSTEVAL` is 0 when `FMT_CPLUSPLUS < 201709L`. `FMT_CONSTEVAL` then expands to nothing, and the runtime-checked path is used ([base.h@11.0.2 L113-L138](https://github.com/fmtlib/fmt/blob/11.0.2/include/fmt/base.h#L113-L138)).
- **Why MSVC itself compiles it:** not documented in these sources. Presumably MSVC doesn't enforce the CWG2765 direction; **UNVERIFIED**.

### 7.2 Workarounds without editing fmt (facts only)
- **Predefining `FMT_USE_CONSTEVAL` doesn't work in 11.0.2.** The header defines it unconditionally in an `#if/#elif` chain with no `#ifndef` guard ([L113-L131](https://github.com/fmtlib/fmt/blob/11.0.2/include/fmt/base.h#L113-L131)). A command-line `-D` would be redefined and draw a macro-redefinition diagnostic. fmt 12.0.0 has the same unguarded chain ([base.h@12.0.0](https://github.com/fmtlib/fmt/blob/12.0.0/include/fmt/base.h)).
- **`SPDLOG_USE_STD_FORMAT`:** spdlog 1.15.0 `common.h` defines `SPDLOG_FMT_STRING(s)` as `FMT_STRING(s)` unless this macro is defined. With it, the macro expands to the bare string ([spdlog common.h@v1.15.0 L52-L62](https://github.com/gabime/spdlog/blob/v1.15.0/include/spdlog/common.h#L52-L62)).
  - That switches spdlog's backend to `std::format`, which changes the library configuration.
  - It doesn't affect fmt's own `FMT_STRING` uses in `format-inl.h` if bundled fmt is still compiled. Measured in §8: with `/DSPDLOG_USE_STD_FORMAT` the same errors remain, because `debug/Log.h` still pulls in bundled fmt.
- **Other levers:**
  - Language mode for clang-tidy only: `--extra-arg=/std:c++17`. This analyzes different code than the build.
  - Upgrading the vendored spdlog to ≥ 1.15.1, or using `SPDLOG_FMT_EXTERNAL` with fmt ≥ 11.1.0.
  - Neither was tested here.

### 7.3 The Static Analyzer skips TUs with errors
- `AnalysisConsumer::HandleTranslationUnit` returns early: "Don't run the actions if an error has occurred with parsing the file", via `if (Diags.hasErrorOccurred() || Diags.hasFatalErrorOccurred()) return;` ([LLVM: clang/lib/StaticAnalyzer/Frontend/AnalysisConsumer.cpp L622-L626](https://github.com/llvm/llvm-project/blob/llvmorg-22.1.3/clang/lib/StaticAnalyzer/Frontend/AnalysisConsumer.cpp#L622-L626)).
- So `clang-analyzer-*` checks produce nothing for a TU that hits the fmt error, or any `-Werror`-promoted warning (§5.2).
- The non-analyzer AST-matcher checks still report on such a TU (measured in §8: 962 diagnostics, while `clang-analyzer-*` gives 0). A probe confirms the analyzer side: a null dereference is reported in a file without `#include "debug/Log.h"` and not reported with it.
- Microsoft's page says clang-tidy in VS "runs after successful compilation" ([MS](https://learn.microsoft.com/en-us/cpp/code-quality/clang-tidy)).

---

## 8. Measurement on the current code

### Method

- Toolset: MSVC 14.51.36231 (`v145`, VS 2026 Community 18), x64. Project: the current hand-written `space_traverse/space_traverse.vcxproj` at `dev` (`a177203`), 11 translation units in `space_traverse/src`.
- The vcxproj was not edited. Each variant was built with `msbuild /t:Rebuild` (Release) or `/t:ClCompile` (Debug), with a props file injected through `/p:ForceImportAfterCppTargets=<variant>.props` and `IntDir`/`OutDir` redirected to a scratch folder. The props file overrides `WarningLevel`, `AdditionalIncludeDirectories`, `ExternalIncludePath`, `ExternalWarningLevel` and `AdditionalOptions`.
- "External" variants move `space_traverse/include` from `/I` to `ExternalIncludePath` (`/external:I`) with `ExternalWarningLevel=TurnOffAllWarnings` (`/external:W0`).
- Release variants are compile-only in effect: the link step fails (see the side finding), and with `/GL` code generation happens at link time, so the warnings it emits never appear. In this run `C4715` and `C4172` showed up only in the Debug build (no `/GL`). The Debug row covers this gap for `/W4`.
- Counts are **unique `(warning code, file, line)` triples**. The same header warning repeated in every TU that includes it counts once. Origin is classified by path: `src/` (own), `include/<lib>/` (vendor), MSVC STL / Windows SDK.

### Results

| Variant | Config | Own (`src/`) | Eigen | fkYAML | spdlog | TinyMAT | STL/SDK | Total |
|---|---|---|---|---|---|---|---|---|
| `/W3` (today) | Release | 16 | 0 | 0 | 0 | 0 | 0 | 16 |
| `/W4` | Release | 18 | 5 | 0 | 0 | 0 | 0 | 23 |
| `/W4` + `/external:W0` | Release | 18 | 0 | 0 | 0 | 0 | 0 | 18 |
| `/W4` + `/external:W0` + `/external:templates-` | Release | 18 | 5 | 0 | 0 | 0 | 0 | 23 |
| `/W4` + `/external:W0` | Debug | 20 | 0 | 0 | 0 | 0 | 0 | 20 |
| `/Wall` | Release | 163 | 658 | 106 | 5 | 9 | 13 | 954 |
| `/Wall` + `/external:W0` | Release | 162 | 0 | 0 | 0 | 0 | 0 | 162 |

### What the numbers say

- **`/W3` → `/W4` costs two warnings in own code.** `C4100` (unreferenced parameter, `mesh/GJK.h:115`, `LOG(... bool result)`) and `C4189` (unused local, `solver/Solver.cpp:323`). Everything else at `/W4` is already there at `/W3`.
- **The `/W3` baseline is not clean.** 16 warnings exist today:
  - `C4267` ×6: `size_t` → `uint32_t` in `particles/Beam.cpp:96,137,149,169`, `mesh/Mesh.cpp:417`, `mesh/TopologyTree.cpp:122`.
  - `C4244` ×7: `double` → `size_t` in `particles/Beam.cpp:127-129`, `double` → `int` in `Beam.cpp:211`, and `int64`/`double` → `T` in the template helpers of `utils/yaml.h:54,59,89` (reported once per instantiation site).
  - `C4828` ×2: `SimuControl.cpp` and `utils/PhysConstants.h` are not valid UTF-8 (Windows-1251 Cyrillic) while the build passes `/utf-8`. The bytes are only in `//` comments, so the code is unaffected; the clang-format ticket already plans to translate those comments.
  - `C4353` ×1: `utils/ToMatlab.cpp:56`, `if (vec.IsRowMajor())`. `IsRowMajor` is an Eigen enum constant, not a member function, so MSVC treats `0()` as `__noop` and the condition is always false. The argument is `Eigen::VectorXd` (column-major), so the dead branch happens to be the unused one and the output is correct.
- **The Debug build shows two more, both already in the inventory ([#6](https://github.com/srMemphis/space_traverse/issues/6)):** `C4172` (returning the address of a temporary, `Mesh.cpp:187`, the dangling reference in `TagToName`) and `C4715` (not all control paths return a value, `Mesh.cpp:248`, the missing return in `GetPhisicalTagOfTetrahedron`). In Release with `/GL`, code generation runs at the LTCG link step, which fails here (see the side finding below), so a compile-only Release count misses these two.
- **Vendor noise at `/W4` is tiny:** 5 × `C5054` (`operator '|'` between enumerations of different types, deprecated in C++20) in `Eigen/src/Core/Matrix.h:46` and `TriangularMatrix.h:211-214`. `/external:W0` removes them.
- **`/external:templates-` brings exactly those 5 back.** The Eigen templates are instantiated from our code, so with `templates-` the warnings in them are reported again. With the default (`/external:templates`, on) they stay suppressed.
- **`/Wall` is dominated by headers outside `src/` (791 of 954: 778 vendored, 13 STL/SDK) and by off-by-default informational warnings.** With `/external:W0`, own code still has 162:
  - `C4365` ×75 (signed/unsigned conversion; 48 of them in `solver/Solver.cpp`), `C4388` ×23 (signed/unsigned comparison; 17 in `mesh/Mesh.cpp`).
  - `C4514` ×26 (unreferenced inline function removed), `C4820` ×4 (padding), `C5219` ×4, `C4061` ×3, `C4626`/`C4800`/`C5266`/`C5267` ×2–3 each.
  - `C5038` ×1 in `particles/Beam.cpp:14`: the constructor's init list names `m_particle` before `m_radius`, but `m_radius` is declared first. The initializers are independent constants, so this is cosmetic.
- **No warnings came from the MSVC STL or Windows SDK** below `/Wall`: MSBuild already passes those directories as external by default.

### MSVC Code Analysis (`/analyze`) trial run

- `msbuild /p:RunCodeAnalysis=true`, Debug, `/W4` + `/external:W0` props as above. MSBuild added `/analyze /analyze:quiet /analyze:plugin EspXEngine.dll` and the default rule set `NativeRecommendedRules.ruleset`; nothing in the vcxproj was set for it.
- Time: **42 s** vs **14 s** for the same Debug rebuild without analysis (×3).
- Unique findings: 49 in total. Of these, 24 are own code: the 20 `/W4` Debug warnings above plus 4 analysis warnings. The other **25 are in vendor headers despite `/external:W0`**: Eigen 18, spdlog 6, TinyMAT 1. The external warning level does not cover analysis warnings; they have their own switch, `/analyze:external-` (see section 3).
- The 4 own analysis findings:
  - `C26495` (member not initialized): `Beam::m_divergence` (`particles/Beam.cpp:13`) and `DetectorData::current` (`DetectorData.h:33`). The first is the "uninitialized `Beam` members" bug from the [#6](https://github.com/srMemphis/space_traverse/issues/6) inventory.
  - `C28020` at `mesh/GJK.h:169` (`tet[maxIDtet]` on a `std::array<…, 4>`, annotation `_Param_(1) < 4` not provable). `maxIDtet` comes from a loop over `0..3`, so this looks like a false positive.
  - `C6993` at `particles/Beam.cpp:277`: code analysis ignores the OpenMP `#pragma omp parallel for` region. The same note comes from Eigen's `SparseDenseProduct.h`. So `/analyze` does not look inside our OpenMP loops.
- Vendor findings are mostly `C26495` in Eigen packet/storage types (10) and `C26498` in spdlog (5).

### clang-tidy trial run

- Binary: `clang-tidy.exe` bundled with VS 2026 under `VC/Tools/Llvm/x64/bin`, **LLVM 22.1.3**. Only `clang-tidy.exe` and `clang-format.exe` are installed there (no `clang-cl`), and that is enough to run it.
- Input: a hand-written `compile_commands.json` whose `arguments` start with `cl.exe` (clang-tidy then uses the cl driver mode) and carry the MSVC flags as-is: `/I src`, `/external:I include`, `/external:W0`, `/W4`, `/sdl`, `/EHsc`, `/MD`, `/openmp`, `/std:c++20`, `/permissive-`, `/utf-8`, `/D ...`. None of these flags were rejected.
- Checks: `bugprone-*`, `clang-analyzer-*`, `performance-*`, `cppcoreguidelines-*`, `modernize-*`, `readability-*`, with `--header-filter` limited to `src/`. Wall time: **44 s** for 11 TUs, run one at a time (3–6 s per TU).
- **Hard errors in every TU.** Every TU includes `debug/Log.h` → spdlog 1.15.0 → bundled fmt 11.0.2, and clang reports 9 unique `error: call to consteval function 'fmt::basic_format_string<...>::basic_format_string<FMT_COMPILE_STRING, 0>' is not a constant expression` in `spdlog/details/fmt_helper.h`, `spdlog/fmt/bundled/format-inl.h`, `spdlog/logger-inl.h` and `spdlog/logger.h` (all `FMT_STRING` uses). `--header-filter` does not hide errors. `/DSPDLOG_USE_STD_FORMAT` does not remove them; `/std:c++17` does (it disables `consteval` in fmt), but the project is C++20.
- **Consequence: `clang-analyzer-*` finds nothing on this code.** A probe file `#include "debug/Log.h"` plus an obvious null dereference gets no `clang-analyzer-core.NullDereference`; the same file without the include gets it. So the static analyzer silently skips TUs that have compile errors, and here that's all of them. AST-matcher checks (`bugprone-*`, `modernize-*`, …) still run.
- One more hard error is ours: `utils/ToMatlab.cpp:56` `vec.IsRowMajor()`, "called object type is not a function", which is the same `C4353` that MSVC only warns about.
- Own-code diagnostics (unique): **962**. By family: `readability` 376, `modernize` 311, `cppcoreguidelines` 294, `bugprone` 62, `performance` 10. Most of it is style:
  - `modernize-use-trailing-return-type` 186, `readability-identifier-length` 152, `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access` 107, `cppcoreguidelines-init-variables` 48, `modernize-use-nodiscard` 39, `readability-else-after-return` 39, `readability-qualified-auto` 38, magic numbers 33 (reported twice, under `readability-` and `cppcoreguidelines-`).
  - `bugprone-narrowing-conversions` 53 (aliased as `cppcoreguidelines-narrowing-conversions`): mostly `size_t` → `Eigen::Index` in `solver/Solver.cpp`, plus the same `Beam.cpp` lines MSVC flags.
  - `bugprone-exception-escape` 3 (`main`, `~Profiler`, `~Timer`), `bugprone-easily-swappable-parameters` 5, `bugprone-suspicious-stringview-data-usage` 1 (`utils/ToMatlab.h:54`), `performance-unnecessary-value-param` 9.

### Side finding: Release does not link with the current toolset

All Release variants compiled, but `link` failed with `C1047` / `LNK1257`: `TinyMAT_Release.lib` contains `/GL` (LTCG) objects built by a different compiler version than 14.51.36231. The Debug lib has no `/GL` and is not affected. This is independent of warning settings and concerns [#11](https://github.com/srMemphis/space_traverse/issues/11) (how TinyMAT is brought in) and the migration verification of [#9](https://github.com/srMemphis/space_traverse/issues/9), which plans to run the old Release exe.

## Unverified / open items
- The LLVM version bundled with VS 2026 isn't documented by Microsoft. 22.1.3 is a local observation only.
- How the VS `ClangTidy` MSBuild task builds the clang-tidy command line (`HeaderFilter` mapping, compile DB location). It's in a closed DLL.
- That `msbuild /p:RunCodeAnalysis=true /p:EnableClangTidyCodeAnalysis=true` runs clang-tidy on the command line. This was inferred from the targets and not executed.
- `/WX` combined with `/external:W0`: no explicit MS statement. The behavior was inferred.
- Which C47xx back-end warnings Eigen actually triggers under `/Wall` in this codebase. The §8 `/Wall` run saw one `C4710` (fkYAML) at compile time. Release uses `/GL`, so most code-generation warnings would come out at the LTCG link, and that link fails (§8 side finding).
- What GNU-mode `clang++` does with stray `/utf-8`-style arguments from unfiltered `buildoptions`.
- Why MSVC accepts fmt 11.0.2's consteval path when clang 20+ rejects it.
- How long a full clang-tidy run takes in parallel (`run-clang-tidy.py` or VS `MaxNumberOfProcesses`). §8 only measured a serial run.
