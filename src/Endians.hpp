/******************************
 * @file Endians.hpp
 * Gumee Application Framework (GAF)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 * Functions for dealing with endian-ness of data.
 * Commonly used for networking
 ******************************/

#ifndef GAF_UTIL_ENDIANS_HPP
#define GAF_UTIL_ENDIANS_HPP

#include <stdint.h>	// defines data types (uint8_t, etc)

namespace gaf
{
	namespace util
	{
		/// Test if the platform being used is Big or Little endian.
		/// Used when sharing data between computers that may use different platfroms
		/// @return The platform is big-endian
		inline const bool isPlatformBigEndian()	// TODO: turn into compile-time check (constexpr). Works in VS (C++14?) but not gcc (C++11?)
		{
			uint16_t testVal = 1;
			const uint8_t * testByte = reinterpret_cast<const uint8_t*>(&testVal);
			return *testByte != 1;
		}

		/// Reverse the order in which bytes are stored
		/// Used for changing between big and little endian
		/// @param data The variable who's bytes need to be re-arranged
		/// @return Variable containing the data in the new order
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

		/// Make sure that data can move between host platform and net byte order
		/// Check if the platform matches netbyte order and translate data if required
		/// @param data Data to be translated
		/// @return Data that has been properly aligned
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
#endif // !GAF_UTIL_ENDIANS_HPP
