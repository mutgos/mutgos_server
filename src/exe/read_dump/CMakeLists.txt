add_executable(readdump read_dump.cpp)

target_link_libraries(readdump mutgos_dbdump)
target_link_libraries(readdump mutgos_logging)
