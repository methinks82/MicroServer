/******************************
 * @file Platforms.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2018-01-30
 *
 * Platform specific details that are determined at compile time
 ******************************/


#ifndef AMS_PLATFORMS_HPP
#define AMS_PLATFORMS_HPP


////////////// Windows ///////////////////
#ifdef _WIN32

	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
		// Also consider using #define VC_EXTRALEAN
	#endif // !WIN32_LEAN_AND_MEAN

	#pragma comment(lib, "ws2_32.lib")
	#include <WinSock2.h>

	inline void CLOSE_SOCKET(SOCKET sock) { closesocket(sock); }

////////// Linux / osx //////////
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__)) // __unix works, still need to test apple
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <unistd.h>

	using SSIZE_T = ssize_t;
	using SOCKET = int;
	const SOCKET INVALID_SOCKET = -1;

	inline void CLOSE_SOCKET(SOCKET sock) { close(sock); }

#endif //!__unix__


#endif // !AMS_PLATFORMS_HPP
