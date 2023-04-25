/******************************
 * @file ThreadedServer.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 ******************************/

#ifndef AMS_THREADED_SERVER_HPP
#define AMS_THREADED_SERVER_HPP

#include <thread>
#include "Server.hpp"

namespace ams
{
	/// A child of Server that automates threading
	class ThreadedServer : public Server
	{
	public:
		/// Default Constructor
		ThreadedServer() : Server() {}
		/// Destructor
		~ThreadedServer() {}
		
		/// Start the server running in it's own thread
		void start()
		{
			isRunning = true;
			serverThread = new std::thread([this]()
				{ continuousLoop(); });
		}

		/// Stop server and thread
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
}

#endif // !AMS_THREADED_SERVER_HPP
