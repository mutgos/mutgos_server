file(GLOB SQLITEINTERFACE_SRC "*.cpp")

add_library(mutgos_sqliteinterface SHARED ${SQLITEINTERFACE_SRC})

target_link_libraries(
    mutgos_sqliteinterface
        mutgos_osinterface
        mutgos_concurrency
        mutgos_utilities
        mutgos_dbtypes
        mutgos_logging
        sqlite3
        boost_serialization
        boost_atomic
        boost_thread
        boost_system)
