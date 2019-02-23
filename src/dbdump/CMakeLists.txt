file(GLOB DBDUMP_SRC "*.cpp")

add_library(mutgos_dbdump SHARED ${DBDUMP_SRC})

target_link_libraries(
    mutgos_dbdump
        mutgos_dbtypes
        mutgos_dbinterface
        mutgos_osinterface
        mutgos_concurrency
        mutgos_logging
        boost_system)
