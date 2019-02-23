file(GLOB UTILITIES_SRC "*.cpp")

add_library(mutgos_utilities SHARED ${UTILITIES_SRC})

target_link_libraries(
        mutgos_utilities
            mutgos_logging
            mutgos_text
            mutgos_osinterface
            boost_system)