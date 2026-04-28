add_library(eb_extensions extension.c)
target_include_directories(eb_extensions PUBLIC ${CMAKE_SOURCE_DIR}/include)
