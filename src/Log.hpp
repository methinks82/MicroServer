/******************************
 * @file Log.hpp
 * Gumee Application Framework (GAF)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 ******************************/

#ifndef GAF_LOG_HPP
#define GAF_LOG_HPP

#include <string>	// string
#include <sstream>	// stringstream
#include <iostream>	// cout
#include <fstream>	// ofstream
#include <chrono>	// time
#include <ctime>	// convert to local time


namespace gaf
{
	namespace util
	{
		/// Simple logging class. Currently outputs to console or log file
		class Log	
		{
		public:

			/// Levels of event severity that determines how each event is handled
			enum class LEVEL { INFO, DEBUG_MSG, WARNING, ERR, CRITICAL };
			/// Flags used to map each event severity to how it should be outputted
			enum class OUTPUTS { CONSOLE = 1, FILE = 2, FILE_1 = 4, FILE_2 = 8, CUSTOM_FUNCTION = 16 };

			/// Remove all existing entries from log
			static void clear()
			{
				std::ofstream out("Log.txt");
				out.clear();
			}

			/// Create an event with the level of "INFO"
			/// Events at this level are used to indicate information that is not related to a problem
			/// @param msg The message to log
			/// @param caller [Optional] Name of the class or function logging this event. Blank if not set
			static const void info(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::INFO, caller);
			}

			/// Create an event with the level of "DEBUG"
			/// Events of this level are used during development to test program flow and logic.
			/// They are generally ignored for release builds.
			/// @param msg The message to log
			/// @param caller [Optional] Name of the class or function logging this event. Blank if not set
			static const void debug(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::DEBUG_MSG, caller);
			}

			/// Create an event with the level of "WARNING"
			/// Indicates that something was not able to function as hoped, but that the problem was allowed for or caught
			/// Example: Invalid values in config or script files
			/// @param msg The message to log
			/// @param caller [Optional] Name of the class or function logging this event. Blank if not set
			static const void warning(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::WARNING, caller);
			}

			/// Create an event with the level of "ERROR"
			/// Indicates a problem within the program itself that was able to correct  or allow for
			/// @param msg The message to log
			/// @param caller [Optional] Name of the class or function logging this event. Blank if not set
			static const void error(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::ERR, caller);
			}

			/// Create an event with the level of "CRITTICAL"
			/// Indicates a problem that may cause the application to crash
			/// @param msg The message to log
			/// @param caller [Optional] Name of the class or function logging this event. Blank if not set
			static const void critical(const std::string & msg, const std::string & caller = "")
			{
				log(msg, LEVEL::CRITICAL, caller);
			}

			/// Manually create a generic event
			/// @param msg The message to log
			/// @param level The severity level of this event
			/// @param caller Name of the class or function logging this event. Blank if not set
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
	}
}

#endif	// GAF_LOG_HPP
