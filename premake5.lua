-- Build definition for premake 5.0.0 (tools/premake/premake5.exe).
-- Run generate_vs2026.bat to write build/space_traverse.slnx.

workspace "space_traverse"
    configurations { "Debug", "Profile", "Release" }
    platforms { "x64" }
    architecture "x86_64"
    location "build"
    startproject "space_traverse"

    language "C++"
    cppdialect "C++20"
    conformancemode "On"
    characterset "Unicode"
    systemversion "latest"
    multiprocessorcompile "On"
    warnings "Default"
    -- No premake API for /sdl, /utf-8 or FavorSizeOrSpeed (/Ot).
    buildoptions { "/sdl", "/utf-8", "/Ot" }
    -- premake does not add _CONSOLE on its own; the old project defined it everywhere.
    defines { "_CONSOLE" }

    includedirs {
        "space_traverse/src",
        "space_traverse/include",
    }

    targetdir "build/%{cfg.buildcfg}"
    objdir "build/obj/%{cfg.buildcfg}/%{prj.name}"

    symbols "On"

    filter "configurations:Debug"
        defines { "_DEBUG" }
        optimize "Off"

    filter "configurations:Profile or Release"
        defines { "NDEBUG" }
        optimize "Speed"
        -- /GL with /LTCG:incremental, the Visual Studio default the old project used.
        linktimeoptimization "Fast"

    filter "configurations:Profile"
        defines { "PROFILER" }

    filter {}

project "space_traverse_core"
    kind "StaticLib"
    location "build/projects"
    openmp "On"

    files { "space_traverse/src/**" }

project "space_traverse"
    kind "ConsoleApp"
    location "build/projects"
    openmp "On"
    debugdir "space_traverse"

    files { "space_traverse/app/main.cpp" }

    links { "space_traverse_core" }
    libdirs { "space_traverse/lib/TinyMAT" }

    filter "configurations:Debug"
        links { "TinyMAT_Debug" }

    filter "configurations:Profile or Release"
        links { "TinyMAT_Release" }

    filter {}
