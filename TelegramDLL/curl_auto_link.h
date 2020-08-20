#pragma once

#define CURL_LIB_DIR "curl/lib/"

#if defined(_MSC_VER)

/* Architecture */
#ifdef _WIN64
#define CURL_LIB_ARCH "x64/"
#else
#define CURL_LIB_ARCH "x86/"
#endif /* _WIN64 */

/* Configuration */
#ifdef _DEBUG
#define CURL_LIB_CFG "Debug/"
#else
#define CURL_LIB_CFG "Release/"
#endif /* _DEBUG */

#endif /* _MSC_VER */

#define CURL_STATICLIB

#if defined(CURL_LIB_ARCH) && defined(CURL_LIB_CFG)
#pragma comment(lib, CURL_LIB_DIR       \
                     CURL_LIB_ARCH      \
                     CURL_LIB_CFG       \
                     "libcurl.lib")

// Additional dependencies CurlLibs
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "Normaliz.lib")

#else
#error Not supported compiler
#endif

#ifdef CURL_LIB_CFG
#undef CURL_LIB_CFG
#endif

#ifdef CURL_LIB_ARCH
#undef CURL_LIB_ARCH
#endif

#ifdef CURL_DIR
#undef CURL_DIR
#endif
