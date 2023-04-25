/******************************
 * @file HttpProtocol.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2018-01-30
 *
 * Handles HTTP requests
 ******************************/

#ifndef AMS_HTTP_PROTOCOL_HPP
#define AMS_HTTP_PROTOCOL_HPP

#include <map>
#include "ProtocolBase.hpp"
#include "HelperFunctions.hpp"

using std::string;

namespace ams
{
	/// @brief Implementation of protocol to handle HTTP requests.
	/// Used as basic webserver
	class HttpProtocol : public ProtocolBase
	{
	public:
		/// Default Constructor
		HttpProtocol(int port = 80) : ProtocolBase(10, port), path(DEFAULT_PATH) {}

		/// Destructor
		virtual ~HttpProtocol() {}

		/// Add an existing connection to this pool
		/// HttpPool doesn't accept new connections.
		/// @param connection Existing connection that will become part of this pool
		/// @param data Any data that was received by the socket but not processed yet
		virtual void addConnection(Connection connection, const string & data) override
		{
			gaf::util::Log::warning("Adding connections not supported by HttpPool");
			closeConnection(connection);	// remove the invalid connection
		}

		/// Add other connection pools that connectios can be upgraded (moved) to
		/// @param name The identifier by which the pool is referenced
		/// @param protocol The protocol to add
		void addUpgradeProtocol(const string & name, ProtocolBase * protocol)
		{
			upgradeProtocols[name] = protocol;
		}

		/// Change the location to look for HTML files
		/// @param filePath The location to look for files, relative to execution directory
		void setPath(const string & filePath)
		{
			path = filePath;
		}

	protected:
		/// Process data received from socket.
		/// @param connection Connection that received data
		/// @param data String containing data received by connection
		virtual void receiveData(Connection & connection, const std::string & data) override
		{
			gaf::util::Log::debug("HttpProtocol::onReceive\nReceived message\n-----------------\n" + data);

			string result;

			// check for upgrade
			string upgrade = readVariableFromString("Upgrade:", data);
			if (!upgrade.empty())
			{
				auto pool = upgradeProtocols.find(upgrade);
				if (pool != upgradeProtocols.end())
				{
					gaf::util::Log::debug("Upgrading to " + upgrade);
					// move the connection to the upgrade pool
					pool->second->addConnection(connection, data);
					removeConnection(connection);
				}
				else
				{
					// upgrade failed, close connection
					gaf::util::Log::warning("HTTP Upgrade to " + upgrade + " not supported by current server configuration");
					closeConnection(connection);
				}
			}
			else
			{
				string targetFile = readVariableFromString("GET", data);
				if (targetFile.length() > 1)	// ignore leading slash
				{
					targetFile = path + targetFile;
				}
				else
				{
					targetFile = path + DEFAULT_FILE;
				}

				result = readFile(targetFile);
				if (result.empty())
				{
					result = "HTTP/1.1 404 Not Found\r\n\r\n";
				}
				else
				{
					result = "HTTP/1.1 200 OK\r\n\r\n" + result;
				}

				sendData(connection, result);
				closeConnection(connection);	// TODO: Explore keeping connection open
			}
		}

	private:
		const string DEFAULT_PATH = "pages";
		const string DEFAULT_FILE = "/index.html";
		string path;
		std::map<string, ProtocolBase *>upgradeProtocols;
	};
}

#endif // !AMS_HTTP_PROTOCOL_HPP
