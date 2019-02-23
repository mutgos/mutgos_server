file(GLOB CHANNELS_SRC "*.cpp")

add_library(mutgos_channels SHARED ${CHANNELS_SRC})

target_link_libraries(
        mutgos_channels
        mutgos_logging
        mutgos_osinterface)
