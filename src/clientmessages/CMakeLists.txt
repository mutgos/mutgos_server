file(GLOB CLIENTMESSAGES_SRC "*.cpp")

add_library(mutgos_clientmessages SHARED ${CLIENTMESSAGES_SRC})

target_link_libraries(
        mutgos_clientmessages
        mutgos_logging
        mutgos_osinterface)
