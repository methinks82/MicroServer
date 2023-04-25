/******************************
 * @file Base64.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 * Utility functions that are use for working with Base64 encoding
 * Used by SHA-1
 ******************************/

#ifndef AMS_BASE64_HPP
#define AMS_BASE64_HPP

#include <string>

namespace ams
{
	/// convert a string of 8-bit data to 64-bit encoding
	/// @param data Character buffer containing the data to encode
	/// @param size Number of bytes to be encoded
	static const std::string encodeBase64(const uint8_t * data, const int size)
	{
		// As per RFC-4648
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
}

#endif // !AMS_BASE64_HPP
