/******************************
 * @file SHA-1.hpp
 * Alex's MicroServer (AMS)
 * @author Alex Schlieck
 * @version 0.1
 * @date 2019-01-30
 *
 ******************************/

// TODO: replace character buffers with strings

#ifndef AMS_SHA1_HPP
#define AMS_SHA1_HPP
#include <cstring>
#include "Base64.hpp"

namespace ams
{
	/// Implementation of the SHA-1 Hash algorithm as defined in RFC-3174
	class SHA1
	{
	public:
		/// Default Constructor
		SHA1()
		{
			memset(output, 0, REGISTER_COUNT * sizeof(WORD) );
			reset();
		}

		/// set everything back to it's initial values
		void reset()
		{
			// fill digest with starting values defined by SHA1 Standard
			digest[0] = 0x67452301;
			digest[1] = 0xEFCDAB89;
			digest[2] = 0x98BADCFE;
			digest[3] = 0x10325476;
			digest[4] = 0xC3D2E1F0;

			dataSize = 0;
			clearBlock();
		}

		/// Hash a complete data buffer
		/// @param data point to data to be hashed
		/// @param size number of bytes to be hashed
		/// @return the digest of the hashed data
		uint8_t * hash(const uint8_t * data, const unsigned int size)
		{
			addData(data, size);
			return finish();
		}

		/// Hash a complete string
		/// @param data string containing data to be hashed
		/// @return the digest of the hashed data
		uint8_t * hash(const std::string data)
		{
			return hash((uint8_t*)data.c_str(), static_cast<unsigned int>(data.length()));
		}

		/// Append a chunk of data to the existing data
		/// @param data pointer to data to be added
		/// @param size number of bytes to be added
		void addData(const uint8_t * data, const unsigned int size)
		{
			int targetIndex = getStartingIndex();
			int sourceIndex = 0;

			dataSize += size;

			int remainingSize = size;
			while(remainingSize > 0)
			{
				int sizeToWrite = BYTES_PER_BLOCK - targetIndex;
				if (remainingSize < sizeToWrite)	// partial block
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
					targetIndex = 0;	// go back to beginning of next block
				}
			}
		}

		/// process any pending data (such as a partial block)
		/// @return pointer to digest
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
			reset();	// prepare for next hash
			return output;
		}

		/// get current values in the digest
		/// @return pointer to digest
		uint8_t * getDigest()
		{
			return output;
		}

		/// get the digest as a base-64 encoded string
		/// @return string containing the base-64 encoded digest
		std::string getBase64()
		{
			return encodeBase64(output, BYTES_PER_DIGEST);
		}

		/// hash a string and get it's base-64 encoded result
		/// @param data string of data to be hashed
		/// @return Base-64 encoded representation of the digest
		std::string hashStringAndGetBase64(const std::string & data)
		{
			hash(data);
			return encodeBase64(output, BYTES_PER_DIGEST);
		}

		/// hash a data buffer and get it's base-64 encoded result
		/// @param data pointer to data to be hashed
		/// @param size the number of bytes to be hashed
		/// @return base-64 representation of the digest
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
		{	memset(block, 0, BYTES_PER_BLOCK);	}

		inline void markEndOfMessage(int & index)
		{
			block[index++] = 0x80;	// 10000000 after last byte
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
			// convert words to bytes
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
			const unsigned int WORDS_PER_BLOCK = 16;	// 512 bits per block / 32 bits per word = 16 words

			for (unsigned int i = 0; i < WORDS_PER_BLOCK; i++)
			{
				wValues[i] = writeBytesToWord(&(data[i * 4]));
			}
			for (unsigned int i = WORDS_PER_BLOCK; i < ROUNDS; i++)
			{
				wValues[i] = rotateLeft(wValues[i-16] ^ wValues[i-14] ^ wValues[i-8] ^ wValues[i-3]);
			}
		}

		inline const WORD writeBytesToWord(const uint8_t * source)
		{
			return (source[0] << 24) + (source[1] << 16) + (source[2] << 8) + source[3];
		}

		inline const WORD rotateLeft(const uint32_t value, const int shift = 1)
		{
			// 0 < shift < 32, otherwise there will be problems
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

				if (round < stage1)	// stage 1
				{
					temp = (hash[1] & hash[2]) | ((~hash[1]) & hash[3]); // (b and c) or ((not b) and d)
					k = 0x5A827999;
				}
				else if (round < stage2)
				{
					temp = hash[1] ^ hash[2] ^ hash[3];	// b xor c xor d
					k = 0x6ED9EBA1;
				}
				else if (round < stage3)
				{
					temp = (hash[1] & hash[2]) | (hash[1] & hash[3]) | (hash[2] & hash[3]);	// (a and b) or (a and c) or (b and c)
					k = 0x8F1BBCDC;
				}
				else
				{
					temp = hash[1] ^ hash[2] ^ hash[3];	// b xor c xor d
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

		// Values defined in the SHA-1 Specs
		const static unsigned int BYTES_PER_BLOCK = 64;	// 512 bits
		const static unsigned int REGISTER_COUNT = 5;
		const static unsigned int ROUNDS = 80;
		const static unsigned int BYTES_PER_DIGEST = 20;

		WORD digest[REGISTER_COUNT];
		MESSAGE_LENGTH_TYPE dataSize;
		uint8_t block[BYTES_PER_BLOCK];
		uint8_t output[REGISTER_COUNT * 4];	// hold the last result while a new hash is ongoing, convert to bytes
	};
}

#endif // !AMS_SHA1_HPP
