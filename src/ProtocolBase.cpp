/******************************
 * @file ProtocolBase.cpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 ******************************/

#include "ProtocolBase.hpp"

using namespace ams;

ProtocolBase::ProtocolBase(const unsigned int secondsToTimeout, const unsigned int port) : connectionCount(0)
{
	FD_ZERO(&receivingSockets);
	secondsUntilConnectionCloses = std::chrono::seconds{ secondsToTimeout };

    auto socketType = SOCK_STREAM; // change to SOCK_DGRM for udp

	listenerSocket = 0;

	if (port != 0)	// Listen on given port
	{
		try {
			// set up the listener
			struct sockaddr_in listeningAddress;
			listeningAddress.sin_family = AF_INET;
			listeningAddress.sin_port = htons(port);
			listeningAddress.sin_addr.s_addr = INADDR_ANY;

			listenerSocket = socket(AF_INET, socketType, IPPROTO_IP);
			if (listenerSocket == INVALID_SOCKET)
			{
				perror("Unable to create listener socket");
				throw std::runtime_error("Unable to create listener socket");
			}

			if (bind(listenerSocket, (struct sockaddr*)&listeningAddress, sizeof(listeningAddress)) < 0) // bind requires scope operator to prevent calling std::bind
			{
				perror("Unable to bind listener socket");
				throw std::runtime_error("Unable to bind socket");
			}

			if (listen(listenerSocket, SOMAXCONN) != 0)
			{
				perror("Unable to listen on socket");
				throw std::runtime_error("Unable to listen");
			}

			FD_SET(listenerSocket, &receivingSockets);

			connectionCount++;
		}
		catch (const std::string & e)
		{
			throw(e);
		}
	}
}

ProtocolBase::~ProtocolBase()
{}

const void ProtocolBase::sendData(Connection & connection, const string & data)
{
	send(connection.sock, data.data(), static_cast<int>(data.length()), 0);
}

const void ProtocolBase::broadcast(const string & data)
{
	for (Connection connection : connections)
	{
		ProtocolBase::sendData(connection, data);
	}
}

void ProtocolBase::closeConnection(Connection & connection)
{
	gaf::util::Log::debug("Closing Connection");
	CLOSE_SOCKET(connection.sock);
	removeConnection(connection);
}

void ProtocolBase::run()
{
	fd_set receivingSocketsCopy = receivingSockets;	// make a copy so select doesn't destroy original
	timeval	selectWaitTime{ 0, 1000 };	// how long the select function waits for data
	int count = select(0, &receivingSocketsCopy, nullptr, nullptr, &selectWaitTime);

	// check if there's a new connection to the listening socket
	if (listenerSocket != 0)	// there is a listener
	{
		if (FD_ISSET(listenerSocket, &receivingSocketsCopy))	// listener received data
		{
			Connection newConn(accept(listenerSocket, nullptr, nullptr));

			gaf::util::Log::debug("New Connection: ");
			if (isRoomForNewConnection())
			{
				connections.push_back(newConn);
				FD_SET(newConn.sock, &receivingSockets);
				readReceivedData(newConn);
			}
			else
			{
				gaf::util::Log::warning("Unable to add connection, limit exceeded");
				char flushBuffer[DEFAULT_BUFFER_SIZE];
				recv(newConn.sock, flushBuffer, DEFAULT_BUFFER_SIZE, 0); // flush the socket
				CLOSE_SOCKET(newConn.sock);	// close connection
			}
		}
	}

	// check if any other sockets received data
	if (count > 0)	// if a socket is waiting
	{
		for (auto connection : connections)	// find the correct socket
		{
			if (FD_ISSET(connection.sock, &receivingSocketsCopy))
			{
				readReceivedData(connection);
			}
		}
	}

	//TODO: add time-out check
	//removeExpiredConnections();
}

bool ProtocolBase::isRoomForNewConnection()
{	
	if (++connectionCount < FD_SETSIZE)
	{
		return true;
	}
	else
	{
		--connectionCount;
		return false;
	}
}

void ProtocolBase::removeConnection(Connection & connection)
{
	auto i = find(connections.begin(), connections.end(), connection);
	if (i != connections.end())
	{
		connections.erase(i);
	}
	FD_CLR(connection.sock, &receivingSockets);
	connectionCount--;
}

void ProtocolBase::updateConnectionLife(Connection & connection)
{
	// find the vector element containing the connection
	auto target = find_if(connections.begin(), connections.end(),
		[&connection](Connection & rhs) {return (connection.sock == rhs.sock); });
	if (target != connections.end())	// there is a result
	{
		connection.updateTime();
		// if not at end, move to end
		//connections.push_back(*target);	// copy to end of collection
		//connections.erase(target);	// remove from middle
	}
}

void ProtocolBase::readReceivedData(Connection & connection)
{
	char buffer[DEFAULT_BUFFER_SIZE];
	memset(buffer, 0, DEFAULT_BUFFER_SIZE);
	SSIZE_T bytesIn = recv(connection.sock, buffer, DEFAULT_BUFFER_SIZE, 0);

	if (bytesIn <= 0)	// no data, connection closed by client
	{
		// TODO: Not getting called
		closeConnection(connection);
	}
	else // valid data
	{
		updateConnectionLife(connection);
		std::string message(buffer, bytesIn);
		receiveData(connection, message);
	}
}

void ProtocolBase::removeExpiredConnections()
{
	//std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	//std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	
	//for (auto current : connections)
	//for(std::vector<Connection>::iterator it = connections.begin(); it != connections.end();)
	std::vector<Connection>::iterator it = connections.begin();
	while (it != connections.end())
	{
		// we want to know where the next object is so we can delete the current one if needed
		auto current = it;
		it++;
		//see if the connection is expired
		//if (now - current->lastUse > secondsUntilConnectionCloses)
		if(current->isAlive(secondsUntilConnectionCloses))	// if this is alive
		{
			// all subsequent connections will still be alive, so stop looking
			//it = connections.end();	// break out of loop
		}
		else
		{
			// close connection
			closeConnection(*current);
		}
	}
}
