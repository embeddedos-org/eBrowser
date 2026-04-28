add_library(eb_firewall firewall.c)
target_include_directories(eb_firewall PUBLIC ${CMAKE_SOURCE_DIR}/include)
