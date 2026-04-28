add_library(eb_ui tab_manager.c bookmark.c download.c devtools.c theme.c reader_mode.c)
target_include_directories(eb_ui PUBLIC ${CMAKE_SOURCE_DIR}/include)
