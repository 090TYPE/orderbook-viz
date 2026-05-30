# Custom FindMbedTLS — redirects to FetchContent-built targets
# IXWebSocket calls find_package(MbedTLS) — we intercept it here.

if(TARGET mbedtls)
    # FetchContent already made these targets available
    get_target_property(_inc mbedtls INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT _inc)
        set(_inc "${mbedtls_SOURCE_DIR}/include")
    endif()

    set(MBEDTLS_INCLUDE_DIRS "${_inc}"   CACHE PATH   "" FORCE)
    set(MBEDTLS_LIBRARY      mbedtls     CACHE STRING "" FORCE)
    set(MBEDX509_LIBRARY     mbedx509    CACHE STRING "" FORCE)
    set(MBEDCRYPTO_LIBRARY   mbedcrypto  CACHE STRING "" FORCE)
    set(MbedTLS_FOUND TRUE)
    set(MBEDTLS_FOUND TRUE)
else()
    # Fallback: system search
    find_path(MBEDTLS_INCLUDE_DIRS mbedtls/ssl.h)
    find_library(MBEDTLS_LIBRARY   NAMES mbedtls)
    find_library(MBEDX509_LIBRARY  NAMES mbedx509)
    find_library(MBEDCRYPTO_LIBRARY NAMES mbedcrypto)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(MbedTLS DEFAULT_MSG
        MBEDTLS_LIBRARY MBEDTLS_INCLUDE_DIRS
        MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)
endif()
