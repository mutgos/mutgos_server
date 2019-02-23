file(GLOB DBINTERFACE_SRC "*.cpp")

add_library(mutgos_dbinterface SHARED ${DBINTERFACE_SRC})

target_link_libraries(
    mutgos_dbinterface
        mutgos_sqliteinterface
        mutgos_dbtypes
        mutgos_osinterface
        mutgos_concurrency
        mutgos_logging
        boost_thread
        boost_atomic
        boost_serialization
        boost_system)
