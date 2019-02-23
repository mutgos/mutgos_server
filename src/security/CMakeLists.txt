file(GLOB SECURITY_SRC "*.cpp")

add_library(mutgos_security SHARED ${SECURITY_SRC})

target_link_libraries(
        mutgos_security
        mutgos_dbtypes
        mutgos_dbinterface
        mutgos_events
        mutgos_logging
        mutgos_osinterface)
