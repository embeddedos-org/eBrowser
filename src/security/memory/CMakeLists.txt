add_library(eb_memory_safety memory_safety.c)
target_include_directories(eb_memory_safety PUBLIC ${CMAKE_SOURCE_DIR}/include)
