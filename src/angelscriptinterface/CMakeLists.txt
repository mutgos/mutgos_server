add_subdirectory(add_on)

file(GLOB ANGELSCRIPT_SRC "*.cpp")

add_library(mutgos_angelscriptinterface SHARED ${ANGELSCRIPT_SRC})

target_link_libraries(
        mutgos_angelscriptinterface
            mutgos_logging
            mutgos_utilities
            mutgos_security
            mutgos_primitives
            mutgos_softcode
            mutgos_angelscriptaddons
            mutgos_executor
            mutgos_channels
            angelscript)