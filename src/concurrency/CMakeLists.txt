file(GLOB CONCURRENCY_SRC "*.cpp")

add_library(mutgos_concurrency SHARED ${CONCURRENCY_SRC})

target_link_libraries(
    mutgos_concurrency
        mutgos_logging
        boost_system)
