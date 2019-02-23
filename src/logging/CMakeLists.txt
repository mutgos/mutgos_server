file(GLOB LOGGING_SRC "*.cpp")

add_library(mutgos_logging SHARED ${LOGGING_SRC})

target_link_libraries(
    mutgos_logging
        mutgos_osinterface
        boost_log
        boost_log_setup)