file(GLOB DBTYPES_SRC "*.cpp")

add_library(mutgos_dbtypes SHARED ${DBTYPES_SRC})

target_link_libraries(
    mutgos_dbtypes
        bcrypt
        mutgos_utilities
        mutgos_osinterface
        mutgos_concurrency
        mutgos_logging
        boost_serialization
        boost_atomic
        boost_thread
        boost_system)
