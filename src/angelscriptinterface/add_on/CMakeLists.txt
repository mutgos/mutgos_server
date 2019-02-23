# This contains any classes from the add_ons directory in AngelScript.
# The files here are UNMODIFIED from the original source, to allow for easier
# upgrades.

file(GLOB ANGELSCRIPTADDONS_SRC "*.cpp")

add_library(mutgos_angelscriptaddons SHARED ${ANGELSCRIPTADDONS_SRC})

target_link_libraries(
        mutgos_angelscriptaddons
        angelscript)