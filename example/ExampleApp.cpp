/******************************
 * @file ExampleApp.cpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 * Used to build simple test app of AMS server
 ******************************/



#include <iostream>

#include "../src/ThreadedServer.hpp"
#include "../src/HttpProtocol.hpp"
#include "../src/WebsocketProtocol.hpp"

int main(int argc, char* argv[])
{
    ams::ThreadedServer server;

	// configure HTML protocol
	ams::HttpProtocol http;
	server.addProtocol(&http);
		
	if (argc > 1)	// program has argument
	{
		http.setPath(argv[1]);	// use the argument as the new path for http files
	}

	// configure Websocket protocol
	ams::WebsocketProtocol websocket;
	// set function to be called when the connection is made
	websocket.setOnConnect([](ams::ProtocolBase * protocol, ams::Connection & connection)
		{
			string msg = std::to_string(connection.sock) + " has joined to the server";
			protocol->broadcast(msg);
		});

	// set function to be called when data is received by the websocket
	websocket.setOnReceive([](ams::ProtocolBase * protocol, ams::Connection & connection, const string & data)
		{
			string msg = std::to_string(connection.sock) + " : " + data;
			protocol->broadcast(msg);
		});

	// set the function to be called when the websocket disconnects
	websocket.setOnDisconnect([](ams::ProtocolBase * protocol, ams::Connection & connection)
		{
			string msg = std::to_string(connection.sock) + " has left the server";
			protocol->broadcast(msg);
		});

	http.addUpgradeProtocol("websocket", &websocket);
	server.addProtocol(&websocket);
		
	// run the server
	std::cout << "Server running, press any key to exit:\n";
	server.start();
	std::getchar();	// wait for key-press
	std::cout <<"\nStopoping Server";
	server.stop();
}
