file(GLOB USERAGENT_SRC "*.cpp")

add_library(mutgos_useragent SHARED ${USERAGENT_SRC})

target_link_libraries(
    mutgos_useragent
        mutgos_logging
        mutgos_utilities
        mutgos_security
        mutgos_channels
        mutgos_softcode)
