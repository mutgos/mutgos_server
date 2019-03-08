file(GLOB BCRYPT_SRC "*.c")

add_library(mutgos_bcrypt SHARED ${BCRYPT_SRC})

target_link_libraries(
    mutgos_bcrypt)