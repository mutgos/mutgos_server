file(GLOB WEBSOCKETCOMM_SRC "*.cpp")

add_library(mutgos_websocketcomm SHARED ${WEBSOCKETCOMM_SRC})

target_link_libraries(
        mutgos_websocketcomm
            mutgos_channels
            mutgos_logging
            mutgos_osinterface
            mutgos_clientmessages
            mutgos_utilities)
