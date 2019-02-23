file(GLOB SOCKETCOMM_SRC "*.cpp")

add_library(mutgos_socketcomm SHARED ${SOCKETCOMM_SRC})

target_link_libraries(
    mutgos_socketcomm
        mutgos_channels
        mutgos_logging
        mutgos_osinterface
        mutgos_utilities
        mutgos_text)
