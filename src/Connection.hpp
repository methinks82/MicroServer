/******************************
 * @file Connection.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 ******************************/

#ifndef AMS_CONNECTION_HPP
#define AMS_CONNECTION_HPP

#include <chrono>
#include "Platforms.hpp"

namespace ams
{
	/// Represents a single connection between the server and a client
	class Connection
	{
	public:
		/// Default constructor
		Connection() { updateTime(); };

		/// Constructor
		/// @param sock The socket used by this connection
		Connection(SOCKET sock) : sock(sock) { updateTime(); }

		/// Comparison operator, used to search for connections
		/// @param other The connection being compared
		/// @return If this connection matches the other connection
		bool operator == (const Connection & other)
		{
			return other.sock == sock;
		}

		const bool isAlive(std::chrono::seconds& lifespan)
		{
			return (std::chrono::steady_clock::now() - lastUse) < lifespan;
		}

		/// reset the connections time-out counter
		void updateTime()
		{
			lastUse = std::chrono::steady_clock::now();
		}

		SOCKET sock;	/// Socket that this connection uses
		std::chrono::steady_clock::time_point lastUse;	/// The last time that this connection did something, used for connection expiry
	};
}

#endif // !AMS_CONNECTION_HPP
