/******************************
 * @file HelperFunctions.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 * Various commonly used functions
 ******************************/

#ifndef AMS_HELPER_FUNCTIONS_HPP
#define AMS_HELPER_FUNCTIONS_HPP

#include <string>
#include <sstream>
#include <fstream>

namespace ams
{
	/// Tries to find a variable name/value pair from the input string
	/// @param variableName The name of the variable being sought
	/// @param input The string in which to search for the variable
	/// @param delimiter The character separating tokens (parts of the string)
	/// @return The first chunk of the string after the variable name. Empty if the variable isn't found
	std::string readVariableFromString(const std::string & variableName, const std::string & input, const char delimiter = ' ')
	{
		// have to read line by line
		std::istringstream ss(input);

		std::string line;
		std::string result;

		while (getline(ss, line))	// iterate through all lines of string
		{
			std::string::size_type position = line.find(variableName);
			if (position != std::string::npos)	// if found within this line
			{
				std::string::size_type start = line.find(delimiter, position) + 1;	// go to start of next token, skip delimiter
				std::string::size_type end = line.find(delimiter, start);	// go to end of next token
				if (end == std::string::npos)
				{
					end = line.length() - 1;
				}
				result = line.substr(start, end - start);	// get the value
			}
		}
		return result;
	}

	/// Try to open a file from disk and write the resulting data into a string
	/// @param filePath Path and name of file to open, relative to the running executable
	/// @return String containing contents of the file. Can also contain binary data
	std::string readFile(const std::string & filePath)
	{
		std::string result;

		std::ifstream file(filePath, std::ios::binary);
		if (file.is_open())
		{
			int dataSize;
			// go to end of data to calculate size
			file.seekg(0, std::ios::end);
			// TODO: is there a safer way to do this?
			dataSize = (int)file.tellg();

			// go back to begining of data to begin read
			file.seekg(0, std::ios::beg);
			char* buffer = new char[dataSize];
			file.read(buffer, dataSize);
			result.assign(buffer, dataSize);
			file.close();
			delete[] buffer;
		}
		return result;
	}

	/*
	string htmlResponder(const std::string & msg, const string & defaultPath = "pages", const string & defaultFile = "/index.html")
	{
		string result;
		string targetFile = readVariableFromString("GET", msg);
		if (targetFile.length() > 1)	// ignore leading slash
		{
			targetFile = defaultPath + targetFile;
		}
		else
		{
			targetFile = defaultPath + defaultFile;
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
		return result;
	}*/
}

#endif // !AMS_HELPER_FUNCTIONS_HPP
