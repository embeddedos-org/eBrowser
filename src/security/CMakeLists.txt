add_library(eBrowser_security STATIC
    crypto.c
    tls.c
    security.c
    secure_storage.c
    permissions.c
)
target_include_directories(eBrowser_security PUBLIC ${CMAKE_SOURCE_DIR}/include)

if(eBrowser_USE_MBEDTLS)
    # PUBLIC, not PRIVATE. EB_USE_MBEDTLS adds members to eb_tls_ctx_t in the
    # public header eBrowser/tls.h, so it changes the size of a struct that
    # callers allocate themselves. Defined PRIVATE, this library saw a
    # 10832-byte eb_tls_ctx_t while every consumer saw 7944 bytes, and
    # eb_tls_init() wrote 2888 bytes past the end of a caller's stack object.
    # The include directories must propagate for the same reason: the public
    # header includes mbedtls/ssl.h.
    target_link_libraries(eBrowser_security PUBLIC mbedtls mbedcrypto mbedx509)
    target_compile_definitions(eBrowser_security PUBLIC EB_USE_MBEDTLS=1)
endif()
