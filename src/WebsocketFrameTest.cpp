#include "..\test\catch.hpp"
#include "WebsocketFrame.hpp"
//#include <iostream>	// Remove, debug only
//#include <bitset>	// Remove, debug only

using namespace std;
using namespace ams;

/* Debug only, remove
void dumpFrame(string frame, int size = 0)
{
	cout << "\nWebsocket frame:  Length: " << frame.length();
	int maxSize = frame.length();
	if (size != 0)
	{
		maxSize = size;
	}
	for (int i = 0; i < maxSize; i++)
	{
		if (i % 8 == 0)
		{
			cout <<'\n';
		}
		cout << ' ' << bitset<8>(frame[i]);
	}
}*/

TEST_CASE("Websocket Frame")
{
	string shortMessage = "Test";
	string encodedShort;	// manual calculation of what encoded frame should look like
	encodedShort += (char)129;	// first byte: fin + op
	encodedShort += (char)4;		// second byte: mask + data size
	encodedShort += shortMessage;

	SECTION("Write short")
	{
		string result = writeToWebsocketFrame(shortMessage, WebsocketOpCodes::TEXT);
		REQUIRE(result.compare(encodedShort) == 0);
	}
	
	SECTION("Read short")
	{
		string result = readFromWebsocketFrame(encodedShort);
		REQUIRE(result.compare(shortMessage) == 0);
	}

	SECTION("Read and write masked")
	{
		string encoded = writeToWebsocketFrame(shortMessage, WebsocketOpCodes::TEXT, true, true);
		string decoded = readFromWebsocketFrame(encoded);
		REQUIRE(decoded.compare(shortMessage) == 0);
	}

	string longMessage = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus cursus lacus massa, vel suscipit eros interdum et. Nam laoreet ligula ipsum, at tincidunt tortor mollis vulputate. Sed viverra fusce.";
	string encodedLong;
	encodedLong += (char)129;
	encodedLong += (char)126;
	encodedLong += (char)0;
	encodedLong += (char)200;
	encodedLong += longMessage;

	SECTION("Write Long Message")
	{
		string result = writeToWebsocketFrame(longMessage, WebsocketOpCodes::TEXT);
		REQUIRE(result.compare(encodedLong) == 0);
	}

	SECTION("Read Long Message")
	{
		string result =	readFromWebsocketFrame(encodedLong);
		REQUIRE(result.compare(longMessage) == 0);
	}

	string extraLongMessage;
	for (int i = 0; i < 300; i++)
	{
		extraLongMessage += longMessage;
	}

	string encodedExtraLong;
	encodedExtraLong += (char)129;	// fin, opcode
	encodedExtraLong += (char)127;	// 64bit length
	encodedExtraLong += (char)0;	// 1
	encodedExtraLong += (char)0;	// 2
	encodedExtraLong += (char)0;	// 3
	encodedExtraLong += (char)0;	// 4
	encodedExtraLong += (char)0;	// 5
	encodedExtraLong += (char)0;	// 6
	encodedExtraLong += (char)0xea;	// 7 - first byte of size
	encodedExtraLong += (char)0x60;	// 8 - second byte of size
	encodedExtraLong += extraLongMessage;	// payload

	SECTION("Write Extra Long Message")
	{
		string result = writeToWebsocketFrame(extraLongMessage, WebsocketOpCodes::TEXT);
		REQUIRE(result.compare(encodedExtraLong) == 0);
	}

	SECTION("Read Extra Long Message")
	{
		string result = readFromWebsocketFrame(encodedExtraLong);
		REQUIRE(result.compare(extraLongMessage) == 0);
	}

	SECTION("Read and write extra long masked message")
	{
		string encoded = writeToWebsocketFrame(extraLongMessage, WebsocketOpCodes::TEXT, true, true);
		string result =	readFromWebsocketFrame(encoded);
		REQUIRE(result.compare(extraLongMessage) == 0);
	}
}