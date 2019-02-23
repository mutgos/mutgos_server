file(GLOB SOFTCODE_SRC "*.cpp")

add_library(mutgos_softcode SHARED ${SOFTCODE_SRC})

target_link_libraries(
    mutgos_softcode
        mutgos_logging
        mutgos_utilities
        mutgos_security
        mutgos_channels)