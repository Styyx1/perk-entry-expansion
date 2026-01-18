-- set minimum xmake version
set_xmakever("2.8.2")

-- includes
includes("lib/commonlibsse-ng")

-- set project
set_project("PerkEntryExpansion")
set_version("1.0.0")
set_license("Apache-2.0")

-- set defaults
set_languages("c++23")
set_warnings("allextra")

-- set policies
set_policy("package.requires_lock", true)

-- add rules
add_rules("mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

set_config("skse_xbyak", true)
add_requires("magic_enum")

-- targets
target("PerkEntryExpansion")
    -- add dependencies to target
    add_deps("commonlibsse-ng")
    add_packages("magic_enum")

    -- add commonlibsse-ng plugin
    add_rules("commonlibsse-ng.plugin", {
        name = "PerkEntryExpansion",
        description = "An SKSE plugin that adds a perk entry point library for other mods to use."
    })

    -- add src files
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")