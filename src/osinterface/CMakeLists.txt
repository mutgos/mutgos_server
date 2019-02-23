file(GLOB OSINTERFACE_SRC "*.cpp")

add_library(mutgos_osinterface SHARED ${OSINTERFACE_SRC})

target_link_libraries(
    mutgos_osinterface
        pthread)