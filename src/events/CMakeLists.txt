file(GLOB EVENTS_SRC "*.cpp")

add_library(mutgos_events SHARED ${EVENTS_SRC})

target_link_libraries(
    mutgos_events
        mutgos_executor
        mutgos_logging
        mutgos_osinterface)