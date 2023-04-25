/******************************
 * @file Server.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *	
 * Stores, manages and runs all the sockets via socket protocol objects
 ******************************/


#ifndef AMS_SERVER_HPP
#define AMS_SERVER_HPP

#include <vector>
#include "Platforms.hpp"
#include "ProtocolBase.hpp"

namespace ams
{
	class Server
	{
	public:
		/// Default Constructor
		Server()
		{
			#ifdef _WIN32
				WSADATA wsa;
				if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
				{
					throw("Unable to initialize network subsystem");
				}
			#endif // _WIN32
		};
	
		/// Destructor
		~Server()
		{
			protocols.clear();	// make sure all protocols are destroyed in the same place they are created

			#ifdef _WIN32
				WSACleanup();
			#endif // _WIN32

		}

		/// add an existing SocketPool to the server
		/// @param pool Pointer to the SocketPool to be added
		void addProtocol(ProtocolBase * pool)
		{
			protocols.push_back(pool);
		}

		/// Check all of the connection one single time
		/// Can be used to call from an external loop
		void loop()
		{
			for (auto i : protocols)
			{
				i->run();
			}
		}

	private:
		std::vector<ProtocolBase*> protocols;
	};
}
#endif // !AMS_SERVER_HPP
