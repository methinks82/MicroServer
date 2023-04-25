#include <chrono> // used to sleep
#include <thread> // used to sleep


#include "../test/catch.hpp"
#include "Server.hpp"
#include "ProtocolBase.hpp"

using namespace ams;

class TestProtocol : public ProtocolBase
{
public:
	TestProtocol(int& state) :ProtocolBase(2), state(state) { state = 1; }	// each connection expires after 3 seconds
	~TestProtocol() { state = 2; }

	virtual void addConnection(Connection connection, const std::string& data) override
	{
		connections.push_back(connection);
	}	// not used


protected:
	virtual void receiveData(Connection& connection, const std::string& data) {}

private:
	int& state;

};

TEST_CASE("Connection Time-out")
{

	SECTION("Connection Life")
	{
		Connection connection;
		std::chrono::seconds sec{ 2 };
		REQUIRE(connection.isAlive(sec));

		std::this_thread::sleep_for(sec);
		REQUIRE(!connection.isAlive(sec));

		connection.updateTime();
		REQUIRE(connection.isAlive(sec));
	}


	SECTION("Sort by age")
	{
		int state = 0;
		TestProtocol protocol(state);


	}
/*
	int state = 0;
	TestProtocol protocol(state);

	
	SECTION("Add connections")
	{
		Connection connection;
		protocol.addConnection(connection, "");
		protocol.run();
		REQUIRE(state == 1);
	}

/*	SECTION("Time out")
	{
		Connection connection;
		protocol.addConnection(connection, "");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		protocol.run();
		REQUIRE(state == 2);
	}
*/
}