add_library(eBrowser_telemetry STATIC
    logger.c
    metrics.c
)
target_include_directories(eBrowser_telemetry PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
