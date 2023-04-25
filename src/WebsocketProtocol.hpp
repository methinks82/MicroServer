/******************************
 * @file WebsocketProtocol.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 ******************************/

#ifndef AMS_WEBSOCKET_PROTOCOL_HPP
#define AMS_WEBSOCKET_PROTOCOL_HPP

#include <functional>
#include "Log.hpp"
#include "ProtocolBase.hpp"
#include "HelperFunctions.hpp"
#include "SHA-1.hpp"
#include "WebsocketFrame.hpp"

using std::string;
using std::function;

namespace ams
{
	/// @brief Implementation of protocol to handle websockets
	/// Used for 2-way communication with webpage
	class WebsocketProtocol : public ProtocolBase
	{
	public:
		/// Default Constructor
		WebsocketProtocol() : ProtocolBase(30), onConnect(nullptr), onDisconnect(nullptr), onReceive(nullptr) {}

		/// Destructor
		virtual ~WebsocketProtocol() {}

		/// Add an existing connection to this pool
		/// @param connection Existing connection that will become part of this pool
		/// @param data Any data that was received by the socket but not processed yet
		virtual void addConnection(Connection connection, const string & data) override
		{
			if (isRoomForNewConnection())
			{
				// validate websocket

				// get the client's key
				string validationKey = readVariableFromString("Sec-WebSocket-Key:", data);
				if (!validationKey.empty())
				{
					// concantinate magic string
					string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";	// defined in RFC 6455

					// hash and encode
					SHA1 hash;
					string acceptKey = hash.hashStringAndGetBase64(validationKey + magic);
					string response = "HTTP/1.1 101 Switching Protocols\nUpgrade: websocket\nConnection: Upgrade\nSec-WebSocket-Accept: " + acceptKey + "\r\n\r\n";

					// return to client
					ProtocolBase::sendData(connection, response);

					// remember this connection
					connections.push_back(connection);
					FD_SET(connection.sock, &receivingSockets);

					if (onConnect != nullptr)
					{
						onConnect(this, connection);
					}
				}
				else // invalid connection attempt
				{
					// send error to client
					string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
					ProtocolBase::sendData(connection, response);
					// close connection
					closeConnection(connection);
				}
			}
			else
			{
				gaf::util::Log::warning("Unable to accept connection, limit excedded");
				CLOSE_SOCKET(connection.sock);
			}
		}

		/// Send encoded data to a websocket
		/// @param connection Which client to transmit to
		/// @param data The encoded data to send to the client websocket
		virtual const void sendData(Connection & connection, const string & data) override
		{
			string encoded = writeToWebsocketFrame(data, WebsocketOpCodes::TEXT);
			ProtocolBase::sendData(connection, encoded);
		}

		/// Send encoded data to all websockets
		/// @param data The encoded data to be broadcast
		virtual void const broadcast(const string & data) override
		{
			string encoded = writeToWebsocketFrame(data, WebsocketOpCodes::TEXT);
			ProtocolBase::broadcast(encoded);
		}

		/// Sever the connection to client and remove it's connection from the protocol
		/// @param connection The connection of the client to remove
		virtual void closeConnection(Connection & connection) override
		{
			if (onDisconnect != nullptr)
			{
				onDisconnect(this, connection);
			}
			ProtocolBase::closeConnection(connection);
		}

		/// Set a function to be called when a new connection is made
		/// @param callback The function to set
		void setOnConnect(function<void(ProtocolBase * protocol, Connection & connection)> callback)
		{
			onConnect = callback;
		}

		/// Set a function to be called when a connection receives data
		/// @param callback The function to set
		void setOnReceive(function<void(ProtocolBase * protocol, Connection & connection, const string & data)> callback)
		{
			onReceive = callback;
		}

		/// Set a function to be called when a connection is terminated
		/// @param callback The function to be set
		void setOnDisconnect(function<void(ProtocolBase * protocol, Connection & connection)> callback)
		{
			onDisconnect = callback;
		}

	protected:

		/// Process data received from socket.
		/// @param connection Connection that received data
		/// @param data String containing data received by connection 
		void receiveData(Connection & connection, const string & data) override
		{
			switch (getWebsocketFrameOp(data))
			{
				case WebsocketOpCodes::CLOSE:
				{
					gaf::util::Log::debug("Websocket: Close message received");
					closeConnection(connection);
					break;
				}

				case WebsocketOpCodes::TEXT:
				{
					if (onReceive != nullptr)
					{
						string msg = readFromWebsocketFrame(data);
						if (onReceive != nullptr)
						{
							onReceive(this, connection, msg);
						}
						gaf::util::Log::debug("Websocket received message: " + msg);
					}
					break;
				}
			}
		}

	private:
		function<void(ProtocolBase * protocol, Connection & connection)>onConnect;
		function<void(ProtocolBase * protocol, Connection & connection)> onDisconnect;
		function<void(ProtocolBase * protocol, Connection & connection, const string & data)> onReceive;
	};
}

#endif // !AMS_WEBSOCKET_PROTOCOL_HPP
