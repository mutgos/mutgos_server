file(GLOB PRIMITIVES_SRC "*.cpp")

add_library(mutgos_primitives SHARED ${PRIMITIVES_SRC})

target_link_libraries(
        mutgos_primitives
        mutgos_logging
        mutgos_osinterface
        mutgos_dbtypes
        mutgos_channels
        mutgos_comminterface
        mutgos_dbinterface
        mutgos_events
        mutgos_executor
        mutgos_security
        mutgos_text)
