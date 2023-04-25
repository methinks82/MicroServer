#include "../../src/ams.hpp"

int main(int argc, char * argv[])
{
	ams::ThreadedServer server;
	ams::HttpProtocol http;

	ams::WebsocketProtocol websocket;
	websocket.setOnReceive([](
		ams::ProtocolBase * protocol, ams::Connection & connection, const string & msg)
		{
			protocol->broadcast("Received: " + msg);
		});

	http.addUpgradeProtocol("websocket", &websocket);

	server.addProtocol(&http);
	server.addProtocol(&websocket);

	std::cout << "Server Running, press any key to exit\n";
	server.start();
	std::getchar();
	server.stop();
}