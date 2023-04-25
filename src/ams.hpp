/******************************
 * @file ams.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2018-01-30
 *
 * Single header version of the AMS library
 ******************************/

#ifndef AMS_AMS_HPP
#define AMS_AMS_HPP

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <exception>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <time.h>
#include <map>
#include <thread>
#include <stdint.h>
#include <functional>

using std::string;
using std::function;

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

#endif

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>

inline void CLOSE_SOCKET(SOCKET sock) { closesocket(sock); }

#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__)) 
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

using SSIZE_T = ssize_t;
using SOCKET = int;
const SOCKET INVALID_SOCKET = -1;

inline void CLOSE_SOCKET(SOCKET sock) { close(sock); }

#endif 

namespace gaf
{
	namespace util
	{
		class Log
		{
		public:
			enum class LEVEL { INFO, DEBUG_MSG, WARNING, ERR, CRITICAL };
			enum class OUTPUTS { CONSOLE = 1, FILE = 2, FILE_1 = 4, FILE_2 = 8, CUSTOM_FUNCTION = 16 };

			static void clear()
			{
				std::ofstream out("Log.txt");
				out.clear();
			}

			static const void info(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::INFO, caller);
			}
			static const void debug(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::DEBUG_MSG, caller);
			}
			static const void warning(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::WARNING, caller);
			}
			static const void error(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::ERR, caller);
			}
			static const void critical(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::CRITICAL, caller);
			}
			static const void log(const std::string & msg, LEVEL level, const std::string & caller)
			{
				std::string outputString = formatOutput(msg, caller);

				std::cerr << outputString << std::endl;
				writeFile(outputString);
			}

		private:

			static const void writeFile(const std::string & msg)
			{
				const std::string filePath = "Log.txt";
				std::ofstream out(filePath, std::ios::app);
				out << msg << '\n';
				out.close();
			}

			static const std::string formatOutput(const std::string & msg, const std::string & caller)
			{
				std::stringstream ss;
				std::time_t current = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				ss << "[" << current << "@" << caller << "] " << msg;
				return ss.str();
			}
		};

		inline const bool isPlatformBigEndian()
		{
			uint16_t testVal = 1;
			const uint8_t * testByte = reinterpret_cast<const uint8_t*>(&testVal);
			return *testByte != 1;
		}
		template<typename T>
		inline const T swapBytes(const T & data)
		{
			T result = 0;
			const uint8_t * readPointer = reinterpret_cast<const uint8_t*>(&data);
			for (unsigned int i = 0; i < sizeof(data); i++)
			{
				result = result << 8;
				uint8_t temp = *(readPointer + i);
				result += temp;
			}
			return result;
		}
		template <typename T>
		inline const T correctForNetByteOrder(const T & data)
		{
			if (isPlatformBigEndian())
			{
				return data;
			}
			else
			{
				return swapBytes(data);
			}
		}
	}
}

namespace ams
{
	class Connection
	{
	public:
		Connection() { updateTime(); };
		Connection(SOCKET sock) : sock(sock) { updateTime(); }
		bool operator == (const Connection & other)
		{
			return other.sock == sock;
		}
		void updateTime()
		{
			lastUse = std::chrono::steady_clock::now();
		}

		SOCKET sock;
		std::chrono::steady_clock::time_point lastUse;
	};
	class ProtocolBase
	{
	public:
		ProtocolBase(const unsigned int secondsToTimeout, const unsigned int port = 0) : connectionCount(0)
		{
			FD_ZERO(&receivingSockets);
			secondsUntilConnectionCloses = std::chrono::seconds{ secondsToTimeout };

			auto socketType = SOCK_STREAM;

			listenerSocket = 0;

			if (port != 0)
			{
				try {
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

					if (bind(listenerSocket, (struct sockaddr*)&listeningAddress, sizeof(listeningAddress)) < 0)
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
		virtual ~ProtocolBase() {}
		virtual void addConnection(Connection connection, const string & data) = 0;
		virtual const void sendData(Connection & connection, const string & data)
		{
			send(connection.sock, data.data(), static_cast<int>(data.length()), 0);
		}
		virtual const void broadcast(const string & data)
		{
			for (Connection connection : connections)
			{
				ProtocolBase::sendData(connection, data);
			}
		}
		virtual void closeConnection(Connection & connection)
		{
			gaf::util::Log::debug("Closing Connection");
			CLOSE_SOCKET(connection.sock);
			removeConnection(connection);
		}
		void run()
		{
			fd_set receivingSocketsCopy = receivingSockets;
			timeval	selectWaitTime{ 0, 1000 };
			int count = select(0, &receivingSocketsCopy, nullptr, nullptr, &selectWaitTime);
			if (listenerSocket != 0)
			{
				if (FD_ISSET(listenerSocket, &receivingSocketsCopy))
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
						recv(newConn.sock, flushBuffer, DEFAULT_BUFFER_SIZE, 0);
						CLOSE_SOCKET(newConn.sock);
					}
				}


			}
			if (count > 0)
			{
				for (auto i : connections)
				{
					if (FD_ISSET(i.sock, &receivingSocketsCopy))
					{
						readReceivedData(i);
					}
				}
			}
			removeExpiredConnections();
		}

		bool isRoomForNewConnection()
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

	protected:
		virtual void receiveData(Connection & connection, const std::string & data) = 0;

		void removeConnection(Connection & connection)
		{
			auto i = find(connections.begin(), connections.end(), connection);
			if (i != connections.end())
			{
				connections.erase(i);
			}
			FD_CLR(connection.sock, &receivingSockets);
			connectionCount--;
		}
		fd_set receivingSockets;
		std::vector<Connection> connections;

	private:
		void updateConnectionLife(Connection & connection)
		{
			auto target = find_if(connections.begin(), connections.end(),
				[&connection](Connection & rhs) {return (connection.sock == rhs.sock); });
			if (target != connections.end())
			{
				connection.updateTime();
			}
		}
		void readReceivedData(Connection & connection)
		{
			char buffer[DEFAULT_BUFFER_SIZE];
			memset(buffer, 0, DEFAULT_BUFFER_SIZE);
			SSIZE_T bytesIn = recv(connection.sock, buffer, DEFAULT_BUFFER_SIZE, 0);

			if (bytesIn <= 0)
			{
				closeConnection(connection);
			}
			else
			{
				updateConnectionLife(connection);
				std::string message(buffer, bytesIn);
				receiveData(connection, message);
			}
		}
		void removeExpiredConnections() {}

		static const unsigned int DEFAULT_BUFFER_SIZE = 4096;
		unsigned int connectionCount;
		SOCKET listenerSocket;
		std::chrono::seconds secondsUntilConnectionCloses;
	};

	class Server
	{
	public:
		Server()
		{
			#ifdef _WIN32
				WSADATA wsa;
				if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
				{
					throw("Unable to initialize network subsystem");
				}
			#endif
		};
		~Server()
		{
			protocols.clear();

			#ifdef _WIN32
				WSACleanup();
			#endif
		}

		void addProtocol(ProtocolBase * pool)
		{
			protocols.push_back(pool);
		}

		void loop()
		{
			for (auto i : protocols)
			{
				i->run();
			}
		}

	private:
		std::vector<ProtocolBase*> protocols;
	};

	class ThreadedServer : public Server
	{
	public:
		ThreadedServer() : Server() {}
		~ThreadedServer() {}
		void start()
		{
			isRunning = true;
			serverThread = new std::thread([this]()
				{ continuousLoop(); });
		}
		void stop()
		{
			isRunning = false;
			serverThread->join();
			delete serverThread;
		}

	private:
		void continuousLoop()
		{
			while (isRunning)
			{
				loop();
			}
		}

		bool isRunning;
		std::thread * serverThread;
	};

	std::string readVariableFromString(const std::string & variableName, const std::string & input, const char delimiter = ' ')
	{
		std::istringstream ss(input);

		std::string line;
		std::string result;

		while (getline(ss, line))
		{
			std::string::size_type position = line.find(variableName);
			if (position != std::string::npos)	
			{
				std::string::size_type start = line.find(delimiter, position) + 1;	
				std::string::size_type end = line.find(delimiter, start);
				if (end == std::string::npos)
				{
					end = line.length() - 1;
				}
				result = line.substr(start, end - start);
			}
		}
		return result;
	}
	std::string readFile(const std::string & filePath)
	{
		std::string result;

		std::ifstream file(filePath, std::ios::binary);
		if (file.is_open())
		{
			int dataSize;
			file.seekg(0, std::ios::end);
			dataSize = (int)file.tellg();
			file.seekg(0, std::ios::beg);
			char* buffer = new char[dataSize];
			file.read(buffer, dataSize);
			result.assign(buffer, dataSize);
			file.close();
			delete[] buffer;
		}
		return result;
	}

	class HttpProtocol : public ProtocolBase
	{
	public:
		HttpProtocol(int port = 80) : ProtocolBase(10, port), path(DEFAULT_PATH) {}
		virtual ~HttpProtocol() {}

		virtual void addConnection(Connection connection, const string & data) override
		{
			gaf::util::Log::warning("Adding connections not supported by HttpPool");
			closeConnection(connection);
		}

		void addUpgradeProtocol(const string & name, ProtocolBase * protocol)
		{
			upgradeProtocols[name] = protocol;
		}
		void setPath(const string & filePath)
		{
			path = filePath;
		}

	protected:
		virtual void receiveData(Connection & connection, const std::string & data) override
		{
			gaf::util::Log::debug("HttpProtocol::onReceive\nReceived message\n-----------------\n" + data);

			string result;
			string upgrade = readVariableFromString("Upgrade:", data);
			if (!upgrade.empty())
			{
				auto pool = upgradeProtocols.find(upgrade);
				if (pool != upgradeProtocols.end())
				{
					gaf::util::Log::debug("Upgrading to " + upgrade);
					pool->second->addConnection(connection, data);
					removeConnection(connection);
				}
				else
				{
					gaf::util::Log::warning("HTTP Upgrade to " + upgrade + " not supported by current server configuration");
					closeConnection(connection);
				}
			}
			else
			{
				string targetFile = readVariableFromString("GET", data);
				if (targetFile.length() > 1)
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
				closeConnection(connection);
			}
		}

	private:
		const string DEFAULT_PATH = "pages";
		const string DEFAULT_FILE = "/index.html";
		string path;
		std::map<string, ProtocolBase *>upgradeProtocols;
	};
	static const std::string encodeBase64(const uint8_t * data, const int size)
	{
		std::string charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		std::string result;
		uint8_t mask6Bit = 63;
		int i = 0;
		while (i < size)
		{
			uint32_t quantum = 0;

			quantum = data[i] << 16;

			i++;

			if (i < size)
			{
				quantum += data[i] << 8;

				i++;
				if (i < size)
				{
					quantum += data[i];
					i++;
				}
			}

			uint8_t temp;
			temp = (quantum >> 18) & mask6Bit;
			result += charSet[temp];

			temp = (quantum >> 12) & mask6Bit;
			result += charSet[temp];

			temp = (quantum >> 6) & mask6Bit;
			result += charSet[temp];

			temp = (quantum)& mask6Bit;
			result += charSet[temp];
		}

		if (size % 3 >= 1)
		{
			result[result.length() - 1] = '=';

			if (size % 3 == 1)
			{
				result[result.length() - 2] = '=';
			}
		}

		return result;
	}
	class SHA1
	{
	public:
		SHA1()
		{
			memset(output, 0, REGISTER_COUNT * sizeof(WORD));
			reset();
		}
		void reset()
		{
			digest[0] = 0x67452301;
			digest[1] = 0xEFCDAB89;
			digest[2] = 0x98BADCFE;
			digest[3] = 0x10325476;
			digest[4] = 0xC3D2E1F0;

			dataSize = 0;
			clearBlock();
		}
		uint8_t * hash(const uint8_t * data, const unsigned int size)
		{
			addData(data, size);
			return finish();
		}
		uint8_t * hash(const std::string data)
		{
			return hash((uint8_t*)data.c_str(), static_cast<unsigned int>(data.length()));
		}
		void addData(const uint8_t * data, const unsigned int size)
		{
			int targetIndex = getStartingIndex();
			int sourceIndex = 0;

			dataSize += size;

			int remainingSize = size;
			while (remainingSize > 0)
			{
				int sizeToWrite = BYTES_PER_BLOCK - targetIndex;
				if (remainingSize < sizeToWrite)
				{
					memcpy(block + targetIndex, data + sourceIndex, remainingSize);
					remainingSize = 0;
				}
				else
				{
					memcpy(block + targetIndex, data + sourceIndex, sizeToWrite);
					processBlock();
					remainingSize -= sizeToWrite;
					sourceIndex += sizeToWrite;
					targetIndex = 0;
				}
			}
		}
		uint8_t * finish()
		{
			int currentIndex = getStartingIndex();
			markEndOfMessage(currentIndex);

			if (!doesLengthFitInBlock(currentIndex))
			{
				processBlock();
			}
			writeLengthToBlock();
			processBlock();

			writeDigestToOutput();
			reset();
			return output;
		}
		uint8_t * getDigest()
		{
			return output;
		}
		std::string getBase64()
		{
			return encodeBase64(output, BYTES_PER_DIGEST);
		}
		std::string hashStringAndGetBase64(const std::string & data)
		{
			hash(data);
			return encodeBase64(output, BYTES_PER_DIGEST);
		}
		std::string hashAndGetBase64(const uint8_t * data, const unsigned int size)
		{
			hash(data, size);
			return encodeBase64(output, BYTES_PER_DIGEST);
		}

	private:
		typedef uint64_t MESSAGE_LENGTH_TYPE;
		typedef uint32_t WORD;
		void processBlock()
		{

			WORD hash[REGISTER_COUNT];
			fillHashRegisters(hash);


			WORD wValues[ROUNDS];
			fillWValues(block, wValues);

			compress(hash, wValues);
			addHashToDigest(hash);

			clearBlock();
		}

		inline const int getStartingIndex()
		{
			return dataSize % BYTES_PER_BLOCK;
		}

		inline bool doesLengthFitInBlock(int currentPositionInBlock)
		{
			return currentPositionInBlock + sizeof(MESSAGE_LENGTH_TYPE) <= BYTES_PER_BLOCK;
		}

		inline void clearBlock()
		{
			memset(block, 0, BYTES_PER_BLOCK);
		}

		inline void markEndOfMessage(int & index)
		{
			block[index++] = 0x80;
		}

		inline const void writeLengthToBlock()
		{
			const static unsigned int bytesInLength = sizeof(MESSAGE_LENGTH_TYPE);
			const MESSAGE_LENGTH_TYPE numberOfBitsInMessage = dataSize * 8;
			uint8_t * pointerToBytesInLength = (uint8_t*)&numberOfBitsInMessage;

			for (unsigned int i = 0; i < bytesInLength; i++)
			{
				block[BYTES_PER_BLOCK - i - 1] = pointerToBytesInLength[i];
			}
		}

		inline void writeDigestToOutput()
		{
			for (unsigned int i = 0; i < REGISTER_COUNT; i++)
			{
				output[i * 4] = digest[i] >> 24;
				output[(i * 4) + 1] = digest[i] >> 16;
				output[(i * 4) + 2] = digest[i] >> 8;
				output[(i * 4) + 3] = digest[i];
			}
		}

		inline const void fillHashRegisters(WORD * hash)
		{
			for (unsigned int i = 0; i < REGISTER_COUNT; i++)
			{
				hash[i] = digest[i];
			}
		}

		inline const void fillWValues(const uint8_t * data, WORD * wValues)
		{
			const unsigned int WORDS_PER_BLOCK = 16;	

			for (unsigned int i = 0; i < WORDS_PER_BLOCK; i++)
			{
				wValues[i] = writeBytesToWord(&(data[i * 4]));
			}
			for (unsigned int i = WORDS_PER_BLOCK; i < ROUNDS; i++)
			{
				wValues[i] = rotateLeft(wValues[i - 16] ^ wValues[i - 14] ^ wValues[i - 8] ^ wValues[i - 3]);
			}
		}

		inline const WORD writeBytesToWord(const uint8_t * source)
		{
			return (source[0] << 24) + (source[1] << 16) + (source[2] << 8) + source[3];
		}

		inline const WORD rotateLeft(const uint32_t value, const int shift = 1)
		{
			return value << shift | value >> (32 - shift);
		}

		void compress(WORD * hash, WORD * wValues)
		{
			const int stage1 = 20;
			const int stage2 = 40;
			const int stage3 = 60;

			for (unsigned int round = 0; round < ROUNDS; round++)
			{
				WORD temp;
				WORD k;

				if (round < stage1)
				{
					temp = (hash[1] & hash[2]) | ((~hash[1]) & hash[3]);
					k = 0x5A827999;
				}
				else if (round < stage2)
				{
					temp = hash[1] ^ hash[2] ^ hash[3];
					k = 0x6ED9EBA1;
				}
				else if (round < stage3)
				{
					temp = (hash[1] & hash[2]) | (hash[1] & hash[3]) | (hash[2] & hash[3]);
					k = 0x8F1BBCDC;
				}
				else
				{
					temp = hash[1] ^ hash[2] ^ hash[3];
					k = 0xCA62C1D6;
				}

				temp = temp + hash[4];
				temp = temp + rotateLeft(hash[0], 5);
				temp = temp + wValues[round];
				temp = temp + k;

				hash[4] = hash[3];
				hash[3] = hash[2];
				hash[2] = rotateLeft(hash[1], 30);
				hash[1] = hash[0];
				hash[0] = temp;
			}
		}

		inline void addHashToDigest(WORD * hash)
		{
			for (unsigned int i = 0; i < REGISTER_COUNT; i++)
			{
				digest[i] = digest[i] + hash[i];
			}
		}

		const static unsigned int BYTES_PER_BLOCK = 64;
		const static unsigned int REGISTER_COUNT = 5;
		const static unsigned int ROUNDS = 80;
		const static unsigned int BYTES_PER_DIGEST = 20;

		WORD digest[REGISTER_COUNT];
		MESSAGE_LENGTH_TYPE dataSize;
		uint8_t block[BYTES_PER_BLOCK];
		uint8_t output[REGISTER_COUNT * 4];
	};

	enum WebsocketOpCodes : uint8_t { CONTINUATION = 0, TEXT = 1, BINARY = 2, CLOSE = 8, PING = 9, PONG = 10 };

	const uint8_t getWebsocketFrameOp(const string & dataToRead)
	{
		const uint8_t OP_MASK = 0b00001111;
		return dataToRead[0] & OP_MASK;
	}
	const string readFromWebsocketFrame(const string & receivedData)
	{
		string result;
		const uint8_t OP_BITS = 0x0f;
		const uint8_t IS_MASKED_BITS = 0x80;
		const uint8_t LENGTH_BITS = 0x7f;

		uint8_t opCode = receivedData[0] & OP_BITS;	
		bool isMasked = receivedData[1] & IS_MASKED_BITS;

		uint8_t initialLength = receivedData[1] & LENGTH_BITS;
		uint64_t length = initialLength;
		unsigned int position = 2;

		if (initialLength == 126)
		{
			const uint16_t * readPointer = reinterpret_cast<const uint16_t*>(&receivedData[position]);
			length = static_cast<uint16_t>(gaf::util::correctForNetByteOrder(*readPointer));
			position += 2;
		}
		else if (initialLength == 127)	
		{
			const uint64_t * readPointer = reinterpret_cast<const uint64_t*>(&receivedData[position]);
			length = gaf::util::correctForNetByteOrder(*readPointer);
			position += 8;
		}

		if (opCode == WebsocketOpCodes::CONTINUATION || opCode == WebsocketOpCodes::TEXT || opCode == WebsocketOpCodes::BINARY)
		{
			result.resize(static_cast<size_t>(length));

			if (isMasked)
			{
				uint8_t mask[4] = {
					static_cast<uint8_t>(receivedData[position]),
					static_cast<uint8_t>(receivedData[position + 1]),
					static_cast<uint8_t>(receivedData[position + 2]),
					static_cast<uint8_t>(receivedData[position + 3])
				};

				position += 4;

				for (unsigned int i = 0; i < length; i++)
				{
					result[i] = receivedData[i + position] ^ mask[i % 4];
				}
			}
			else
			{
				result = receivedData.substr(position, static_cast<size_t>(length));
			}
		}
		return result;
	}

	const string writeToWebsocketFrame(const string & dataToWrite, const WebsocketOpCodes opCode, const bool isFinal = true, const bool isMasked = false)
	{
		const uint8_t FIN_BITS = 0x80;
		const uint8_t OP_BITS = 0x0f;
		const uint8_t IS_MASKED_BITS = 0x80;

		string result;

		int headerSize = 2;
		uint64_t msgLength = dataToWrite.length();
		uint8_t shortPayloadLength;	

		if (msgLength > 125)
		{
			headerSize += 2;
			shortPayloadLength = 126;
			if (msgLength > 0xff)
			{
				headerSize += 6;
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

		result.resize(headerSize + dataToWrite.length());

		int position = 0;
		if (isFinal)
		{
			result[position] = FIN_BITS;
		}
		result[position] += OP_BITS & opCode;

		position++;
		if (isMasked)
		{
			result[position] = IS_MASKED_BITS;
		}
		result[position] += shortPayloadLength;
		position++;

		if (shortPayloadLength == 126)
		{
			uint16_t sizedLength = static_cast<uint16_t>(msgLength);
			uint16_t correctedLength = gaf::util::correctForNetByteOrder(sizedLength);
			uint16_t * writePointer = reinterpret_cast<uint16_t*>(&result[position]);
			*writePointer = correctedLength;
			position += 2;
		}
		else if (shortPayloadLength == 127)
		{
			uint64_t correctedLength = gaf::util::correctForNetByteOrder(msgLength);
			uint64_t * writePointer = reinterpret_cast<uint64_t*>(&result[position]);
			*writePointer = correctedLength;
			position += 8;
		}

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

			for (unsigned int i = 0; i < dataToWrite.length(); i++)
			{
				result[position + i] = dataToWrite[i] ^ mask[i % 4];
			}
		}
		else
		{
			result.replace(position, dataToWrite.length(), dataToWrite);
		}

		return result;
	}

	class WebsocketProtocol : public ProtocolBase
	{
	public:
		WebsocketProtocol() : ProtocolBase(600), onConnect(nullptr), onDisconnect(nullptr), onReceive(nullptr) {}

		virtual ~WebsocketProtocol() {}

		virtual void addConnection(Connection connection, const string & data) override
		{
			if (isRoomForNewConnection())
			{
				string validationKey = readVariableFromString("Sec-WebSocket-Key:", data);
				if (!validationKey.empty())
				{
					string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

					SHA1 hash;
					string acceptKey = hash.hashStringAndGetBase64(validationKey + magic);
					string response = "HTTP/1.1 101 Switching Protocols\nUpgrade: websocket\nConnection: Upgrade\nSec-WebSocket-Accept: " + acceptKey + "\r\n\r\n";

					ProtocolBase::sendData(connection, response);

					connections.push_back(connection);
					FD_SET(connection.sock, &receivingSockets);

					if (onConnect != nullptr)
					{
						onConnect(this, connection);
					}
				}
				else
				{
					string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
					ProtocolBase::sendData(connection, response);
					closeConnection(connection);
				}
			}
			else
			{
				gaf::util::Log::warning("Unable to accept connection, limit excedded");
				CLOSE_SOCKET(connection.sock);
			}
		}

		virtual const void sendData(Connection & connection, const string & data) override
		{
			string encoded = writeToWebsocketFrame(data, WebsocketOpCodes::TEXT);
			ProtocolBase::sendData(connection, encoded);
		}
		virtual void const broadcast(const string & data) override
		{
			string encoded = writeToWebsocketFrame(data, WebsocketOpCodes::TEXT);
			ProtocolBase::broadcast(encoded);
		}

		virtual void closeConnection(Connection & connection) override
		{
			if (onDisconnect != nullptr)
			{
				onDisconnect(this, connection);
			}
			ProtocolBase::closeConnection(connection);
		}
		void setOnConnect(function<void(ProtocolBase * protocol, Connection & connection)> callback)
		{
			onConnect = callback;
		}

		void setOnReceive(function<void(ProtocolBase * protocol, Connection & connection, const string & data)> callback)
		{
			onReceive = callback;
		}
		void setOnDisconnect(function<void(ProtocolBase * protocol, Connection & connection)> callback)
		{
			onDisconnect = callback;
		}

	protected:

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

#endif // AMS_AMS_HPP