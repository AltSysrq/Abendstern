#ifndef WINVER
#define WINVER 0x0510
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT WINVER
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS WINVER
#endif

//#ifndef _WIN32_IE                       // Specifies that the minimum required platform is Internet Explorer 7.0.
//#define _WIN32_IE 0x0700        // Change this to the appropriate value to target other versions of IE.
//#endif

//Asio must be included before Winsock, but doing so seems to remove access to the crypto functions...
#if defined(__cplusplus) && !defined(CRYPTO_CXX_)
#include <asio.hpp>
#endif
#include <windows.h>
//#include <limits>
#define NAN std::numeric_limits<float>::quiet_NaN()
#define INFINITY std::numeric_limits<float>::infinity()
#define THREAD_LOCAL __declspec(thread)
