-- Soul Gem Hub - SKSE plugin. GPL-3.0-or-later, see LICENSE.txt.
set_xmakever("3.0.0")
set_project("SoulGemHub")
set_version("1.1.0")
set_license("GPL-3.0-or-later")
set_arch("x64")
set_languages("c++23")
-- the DLL carries its own Visual C++ runtime, so an older runtime on a player's PC cannot stop it loading
set_runtimes("MT")
add_rules("mode.releasedbg")
set_defaultmode("releasedbg")

-- GLOW_VR=1 (set by PC Runner\pluginbuild.py) builds for Skyrim VR only; anything else is the SE + AE build
local vr_only = os.getenv("GLOW_VR") == "1"
set_config("skyrim_se", not vr_only)
set_config("skyrim_ae", not vr_only)
set_config("skyrim_vr", vr_only)

-- REPRODUCIBLE BUILDS: the same source gives the same DLL, byte for byte, in whatever folder it is built (as every plugin of
-- ours: /Brepro, /pathmap, /PDBALTPATH - see Dynamic Wards' xmake.lua). MSVC only.
add_cxflags("/Brepro", "/experimental:deterministic", "/pathmap:" .. os.projectdir() .. "=.", { force = true, tools = "cl" })
add_shflags("/Brepro", "/PDBALTPATH:%_PDB%", { force = true, tools = "link" })

includes("lib/commonlibsse-ng")

-- DevBench replies and Light Placer's configs are JSON (MIT-licensed library; the version CommonLib pins for its own option)
add_requires("nlohmann_json v3.12.0")
-- a gem model's color is read off its textures at data load (Microsoft's DirectXTex, MIT-licensed; CPU decoding only)
add_requires("directxtex 2025.07", { configs = { dx11 = false, dx12 = false, runtimes = "MT" } })

target("SoulGemHub", function()
    add_deps("commonlibsse-ng")
    add_packages("nlohmann_json", "directxtex")
    add_defines("NOMINMAX")  -- windows.h min/max macros break std::min/max and numeric_limits::max
    add_rules("commonlibsse-ng.plugin", {
        name = "SoulGemHub",
        author = "izzydoingit",
        description = "Illuminated - Soul Gem Hub - every soul gem glows in its own color, on Community Shaders, ENB and Vanilla",
    })
    add_files("src/*.cpp")
    add_headerfiles("src/*.h")
    add_includedirs("src")
    set_pcxxheader("src/PCH.h")
    -- every warning in OUR files is an error (his C++ standard: "/W4 /WX ... fix all warnings")
    set_warnings("allextra", "error")
end)

-- the game-free part (src/GemMath.h), tested without the game: xmake build testgems && xmake run testgems
-- (PC Runner\plugin build.py builds and runs it, then again under clang's address + undefined-behavior checkers)
target("testgems", function()
    set_kind("binary")
    set_default(false)
    add_files("tests/testgems.cpp")
    add_includedirs("src")
    set_warnings("allextra", "error")
end)
