-- premake5.lua
workspace "YagetCore"
    configurations { "Debug", "Release" }
    platforms { "Win64" }
    location "builds"

filter { "platforms:Win64" }
    system "Windows"
    architecture "x86_64"

project "YagetCore"
    kind "StaticLib"
    language "C++"
    location "builds"
    --targetdir "bin/%{cfg.buildcfg}"

    files {
        "include/**.h",
        "Source/**.cpp"
    }

    vpaths {
        ["Source Files/*"] = { 
            "include/**.h", "Source/**.cpp"
        }
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

