file(GLOB COMMINTERFACE_SRC "*.cpp")

add_library(mutgos_comminterface SHARED ${COMMINTERFACE_SRC})

target_link_libraries(
        mutgos_comminterface
            mutgos_dbtypes
            mutgos_channels
            mutgos_logging
            mutgos_socketcomm
            mutgos_websocketcomm
            mutgos_osinterface)
