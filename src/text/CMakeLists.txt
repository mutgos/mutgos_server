file(GLOB TEXT_SRC "*.cpp")

add_library(mutgos_text SHARED ${TEXT_SRC})

target_link_libraries(
    mutgos_text
        mutgos_logging
        mutgos_osinterface)