add_library(eb_privacy privacy.c tracker_blocker.c)
target_include_directories(eb_privacy PUBLIC ${CMAKE_SOURCE_DIR}/include)
