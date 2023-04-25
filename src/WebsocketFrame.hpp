/******************************
 * @file WebsocketFrame.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 ******************************/

/**
From ABNF(RFC 5234) via RFC 6455

FIN:  1 bit - indicates that this is the final fragment of the message
RSV1, RSV2, RSV3:  1 bit each. Must be 0 unless extension is used, otherwise non 0 must cause the connection to fail
Opcode:  4 bits. Unknown code must cause the connection to fail
• %x0 denotes a continuation frame
• %x1 denotes a text frame
• %x2 denotes a binary frame
• %x3-7 are reserved for further non-control frames
• %x8 denotes a connection close
• %x9 denotes a ping
• %xA denotes a pong
• %xB-F are reserved for further control frames
Mask:  1 bit - If the mask is set. Client always sets mask, host never does
Payload length:  7 bits, 7+16 bits, or 7+64 bits
Masking-key:  0 or 4 bytes
Payload data:  (x+y) bytes
Extension data:  x bytes
Application data:  y bytes


|0              |    1          |        2      |            3  |
|0 1 2 3 4 5 6 7|8 9 0 1 2 3 4 5|6 7 8 9 0 1 2 3|4 5 6 7 8 9 0 1|
+-+-+-+-+-------+-+-------------+---------------+---------------+
|F|R|R|R| opcode|M|   Payload   |    Extended payload length    |
|I|S|S|S|  (4)  |A|   Length    |             (16/64)           |
|N|V|V|V|       |S|     (7)     |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
| ext length cont. (len == 127) | Masking-key, if MASK set to 1 |
+-------------------------------+-------------------------------+
|    Masking-key (continued)    |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+

*/

// TODO: add and test support for continued frames
// TODO: test different type of data

#ifndef AMS_WEBSOCKET_FRAME_HPP
#define AMS_WEBSOCKET_FRAME_HPP

#include <string>
#include <time.h>	// use for rand() to generate mask
#include "Endians.hpp"

//using namespace std;

namespace ams
{
	enum WebsocketOpCodes : uint8_t { CONTINUATION = 0, TEXT = 1, BINARY = 2, CLOSE = 8, PING = 9, PONG = 10 };

	const uint8_t getWebsocketFrameOp(const std::string & dataToRead)
	{
		const uint8_t OP_MASK = 0b00001111;
		return dataToRead[0] & OP_MASK;
	}

	/// Take a websocket frame received from the network and extract data from it
	/// @param receivedData String containing the frame to read
	/// @return What type of message was received
	const std::string readFromWebsocketFrame(const std::string & receivedData)
	{
		std::string result;

		//const uint8_t FIN_BITS			= 0x80;	// 0b10000000;
		const uint8_t OP_BITS			= 0x0f; // 0b00001111;
		const uint8_t IS_MASKED_BITS	= 0x80;	// 0b10000000;
		const uint8_t LENGTH_BITS		= 0x7f; // 0b01111111;

		//bool isFinal = receivedData[0] & FIN_BITS;		// is this the final frame, used to implement continuous frames
		uint8_t opCode = receivedData[0] & OP_BITS;		// what time of op is this
		bool isMasked = receivedData[1] & IS_MASKED_BITS;			// does this frame have a mask

		// find size of payload in bytes
		uint8_t initialLength = receivedData[1] & LENGTH_BITS;	// get the length of the message
		uint64_t length = initialLength;
		unsigned int position = 2;

		// check for extended payload length
		if (initialLength == 126)	// length is stored in the next 16 bits
		{
			const uint16_t * readPointer = reinterpret_cast<const uint16_t*>(&receivedData[position]);
			length = static_cast<uint16_t>(gaf::util::correctForNetByteOrder(*readPointer));
			position += 2;
		}
		else if (initialLength == 127)	// length is stored in the next 64 bits
		{
			const uint64_t * readPointer = reinterpret_cast<const uint64_t*>(&receivedData[position]);
			length = gaf::util::correctForNetByteOrder(*readPointer);
			position += 8;
		}

		// If this op reads data
		if (opCode == WebsocketOpCodes::CONTINUATION || opCode == WebsocketOpCodes::TEXT || opCode == WebsocketOpCodes::BINARY)
		{
			result.resize(static_cast<size_t>(length));

			if (isMasked)	// read and unmask the payload
			{
				uint8_t mask[4] = {
					static_cast<uint8_t>(receivedData[position]),
					static_cast<uint8_t>(receivedData[position + 1]),
					static_cast<uint8_t>(receivedData[position + 2]),
					static_cast<uint8_t>(receivedData[position + 3])
				};

				position += 4;

				// copy byte-by-byte, applying mask to each
				for (unsigned int i = 0; i < length; i++)
				{
					result[i] = receivedData[i + position] ^ mask[i % 4];
				}
			}
			else // no mask, do a direct copy
			{
				result = receivedData.substr(position, static_cast<size_t>(length));
			}
		}
		return result;
	}

	/// Take a databuffer and encode it into a websocket frame
	/// @param dataToWrite String containing the data to encode
	/// @param opCode What kind of operation this frame is encoded for
	/// @param isFinal is this the last frame for the data
	/// @param isMasked Is the data masked. Client MUST mask, Server MUST NOT mask
	/// @return Fully encoded frame
	const std::string writeToWebsocketFrame(const std::string & dataToWrite, const WebsocketOpCodes opCode, const bool isFinal = true, const bool isMasked = false)
	{
		const uint8_t FIN_BITS			= 0x80;	// 0b10000000;
		const uint8_t OP_BITS			= 0x0f; // 0b00001111;
		const uint8_t IS_MASKED_BITS	= 0x80;	// 0b10000000;

		std::string result;

		//// calculate size and make room to write ////
		int headerSize = 2;	// Fin, op, mask, length
		uint64_t msgLength = dataToWrite.length();
		uint8_t shortPayloadLength;	// will trunkate if message is longer than 127

		if (msgLength > 125)	// more than 7 bit
		{
			headerSize += 2;
			shortPayloadLength = 126;
			if (msgLength > 0xff)	// more than 16bit
			{
				headerSize += 6;	// 64bit
				shortPayloadLength = 127;
			}
		}
		else
		{
			shortPayloadLength = static_cast<uint8_t>(msgLength);
		}

		if (isMasked)
		{
			headerSize += 4;
		}

		result.resize(headerSize + dataToWrite.length());	// make room for the header and payload

		//// write the header ////
		int position = 0;
		// write fin		
		if (isFinal)
		{
			result[position] = FIN_BITS;
		}
		// write op	
		result[position] += OP_BITS & opCode;

		position++;
		// write mask flag
		if (isMasked)
		{
			result[position] = IS_MASKED_BITS;
		}

		// write initial (7 bit) length
		result[position] += shortPayloadLength;
		position++;

		// write extended length
		if (shortPayloadLength == 126)
		{
			// write 2 bytes to buffer
			uint16_t sizedLength = static_cast<uint16_t>(msgLength);	// trunkate the length
			uint16_t correctedLength = gaf::util::correctForNetByteOrder(sizedLength);	// convert to correct byte order
			uint16_t * writePointer = reinterpret_cast<uint16_t*>(&result[position]);	// get write pointer
			*writePointer = correctedLength;	// write
			position += 2;
		}
		else if (shortPayloadLength == 127)
		{
			// write 8 bytes to buffer
			uint64_t correctedLength = gaf::util::correctForNetByteOrder(msgLength);	// convert to correct byte order
			uint64_t * writePointer = reinterpret_cast<uint64_t*>(&result[position]);	// get write pointer
			*writePointer = correctedLength;	// write
			position += 8;
		}
		
		// write mask
		if (isMasked)
		{
			srand(static_cast<unsigned int>(time(NULL)));	
			const uint32_t randomValue = rand();
			const uint8_t * mask = reinterpret_cast<const uint8_t*>(&randomValue);

			for (int i = 0; i < 4; i++)
			{
				result[position + i] = mask[i];
			}
			position += 4;

			// mask and write data
			for (unsigned int i = 0; i < dataToWrite.length(); i++)
			{
				result[position + i] = dataToWrite[i] ^ mask[i % 4];
			}
		}
		else
		{
			// append the data
			result.replace(position, dataToWrite.length(), dataToWrite);
		}

		// write message
		return result;
	}
}

#endif // !AMS_WEBSOCKET_FRAME_HPP
