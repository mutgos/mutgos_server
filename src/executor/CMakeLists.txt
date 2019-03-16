file(GLOB EXECUTOR_SRC "*.cpp")

add_library(mutgos_executor SHARED ${EXECUTOR_SRC})

target_link_libraries(
    mutgos_executor
        mutgos_logging
        mutgos_utilities
        mutgos_osinterface)