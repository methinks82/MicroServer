/******************************
 * @file ProtocolBase.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 ******************************/

#ifndef AMS_PROTOCOL_BASE_HPP
#define AMS_PROTOCOL_BASE_HPP

#include <exception>
#include <cstring>          // memset, memcpy
#include <vector>			// list of connections
#include <algorithm>		// find_if, rotate
#include <chrono>			// connection timeout
#include <time.h>			// select time-out
#include "Log.hpp"
#include "Connection.hpp"

#include <iostream>

using std::string;

namespace ams
{
	/// A collection of connections that use the same protocol
	class ProtocolBase
	{
	public:
		/// Constructor
		/// @param secondsToTimeout How long a connection stays alive
		/// @param port The port on which to listen
		ProtocolBase(const unsigned int secondsToTimeout, const unsigned int port = 0);

		/// Destructor
		virtual ~ProtocolBase();

		/// Add an existing connection to this pool
		/// @param connection Existing connection that will become part of this pool
		/// @param data Any data that was received by the socket but not processed yet
		virtual void addConnection(Connection connection, const string & data) = 0;

		/// send data to the to a socket
		/// @param connection Which connection to send to
		/// @param data Information to send to the socket
		virtual const void sendData(Connection & connection, const string & data);

		/// send data to all sockets
		/// @param data Data to be transmitted
		virtual const void broadcast(const string & data);

		/// sever the connection to the client and remove it's connection from the protocol
		/// @param connection The connection to close
		virtual void closeConnection(Connection & connection);

		/// Listen to each connection in the pool and respond to received data
		void run();

		/// Check if adding a connection exceeds the maximum number of FD_SET connections permitted by platform
		/// If the connection can be added, increment the counter
		/// @return If the total number of connections exceeds the platform's limit
		bool isRoomForNewConnection();

	protected:
		/// Process data received from socket.
		/// Implemented by inherited class
		/// @param connection Connection that received data
		/// @param data String containing data received by connection
		virtual void receiveData(Connection & connection, const std::string & data) = 0;

		/// Remove a connection from this protocol without disconnecting
		/// Used when moving connection to a different protocol
		/// @param connection the connection to remove
		void removeConnection(Connection & connection);

		// move to private and create protected accessors?
		fd_set receivingSockets;	/// a connection set that tracks what sockets have received data
		std::vector<Connection> connections;	/// structure to hold all connections

	private:
		/// reset the expiry of the connection
		void updateConnectionLife(Connection & connection);

		/// Check received data for validity and pass it on to the appropriate handler
		void readReceivedData(Connection & connection);

		/// Find and remove any connections that have timed-out
		void removeExpiredConnections();

		static const unsigned int DEFAULT_BUFFER_SIZE = 4096;	/// max number of bytes that can be read at once
		unsigned int connectionCount;
		SOCKET listenerSocket;	/// the socket that waits for incoming connections
		std::chrono::seconds secondsUntilConnectionCloses;	/// how many seconds a connection stays alive
	};
}


#endif // !AMS_PROTOCOL_BASE_HPP
