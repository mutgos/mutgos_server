add_executable(vheap_td vheap_td.cpp)

# Added mutgos_dbtypes to get around circular dependency during library build
target_link_libraries(vheap_td mutgos_utilities mutgos_dbtypes)
