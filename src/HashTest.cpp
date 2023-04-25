#include "../test/catch.hpp"
#include "SHA-1.hpp"

using namespace ams;

TEST_CASE("Finish Empty Hash", "[sha1],[finish],[hash]")
{
	SHA1 sha1;

	SECTION("Hash Empty")
	{
		uint8_t* result = sha1.finish();
		// da39a3ee 5e6b4b0d 3255bfef 95601890 afd80709
		CHECK(result[0] == 0xda);
		CHECK(result[1] == 0x39);
		CHECK(result[2] == 0xa3);
		CHECK(result[3] == 0xee);
		CHECK(result[4] == 0x5e);
		CHECK(result[5] == 0x6b);
		CHECK(result[6] == 0x4b);
		CHECK(result[7] == 0x0d);

		CHECK(result[16] == 0xaf);
		CHECK(result[17] == 0xd8);
		CHECK(result[18] == 0x07);
		CHECK(result[19] == 0x09);
	}

	SECTION("Hash and get result as Base64 Encoding")
	{
		sha1.finish();
		CHECK(sha1.getBase64() == "2jmj7l5rSw0yVb/vlWAYkK/YBwk=");
		CHECK(sha1.hashStringAndGetBase64("abc") == "qZk+NkcGgWq6PiVxeFDCbJzQ2J0=");
	}

	SECTION("Get Digest", "[sha1],[getDigest],[hash]")
	{
		char string1[] = "foo";
		char string2[] = "bar";
		uint8_t * result;
		result = sha1.getDigest();
		CHECK(result[0] == 0);
		CHECK(result[4] == 0);

		sha1.addData((uint8_t*)string1, 3);
		sha1.finish();

		sha1.addData((uint8_t*)string2, 3);
		result = sha1.getDigest();

		// should still be hash for string one
		// 0beec7b5 ea3f0fdb c95d0dd4 7f3c5bc2 75da8a33
		CHECK(result[0] == 0x0b);
		CHECK(result[1] == 0xee);
		CHECK(result[18] == 0x8a);
		CHECK(result[19] == 0x33);

		sha1.finish();
		result = sha1.getDigest();

		// 62cdb702 0ff920e5 aa642c3d 4066950d d1f01f4d
		CHECK(result[0] == 0x62);
		CHECK(result[1] == 0xcd);
		CHECK(result[18] == 0x1f);
		CHECK(result[19] == 0x4d);
	}

	/*
	SECTION("Add data to existing hash", "[sha1],[addData],[hash]")
	{
		char emptyString[] = "";
		char shortString[] = "abc";
		char tooLongForLength[] = "1.This.. 2.is... 3.where 4.we... 5.test. 6length 7.over. 8.flo"; // 62 bits
		char fullBlock[] = "1.This.. 2.test. 4should 5.fit.. 5.right 6.into. 7.one.. 8.block";	// 64 bytes, 512 bits, one block
		char blockOversize[] = "1                                                              3+2"; //66
		char moreThanDoubleSize[] = "0       1       2       3       4       5 x     6       7    !648       9       a       b       c       d x     e       f    !129"; // 64 bytes, test


		uint32_t * result;

		// add short
		sha1.addData((uint8_t*)shortString, 3);
		result = sha1.finish();
		// a9993e36 4706816a ba3e2571 7850c26c 9cd0d89d
		CHECK(result[0] == 0xa9993e36);
		CHECK(result[4] == 0x9cd0d89d);

		sha1.addData((uint8_t*)tooLongForLength, 62);
		result = sha1.finish();
		// 4c6f5b75 a5283900 fc9d5676 a51bdb3b d5338847
		CHECK(result[0] == 0x4c6f5b75);
		CHECK(result[4] == 0xd5338847);

		sha1.addData((uint8_t*)fullBlock, 64);
		result = sha1.finish();
		// 600f5c49 2c0a40db 91e01a35 56d3d193 800f2e35
		CHECK(result[0] == 0x600f5c49);
		CHECK(result[4] == 0x800f2e35);

		sha1.addData((uint8_t*)blockOversize, 66);
		result = sha1.finish();
		// 6f4a3d15 918c04bc e0bcf17a fc1ceee5 9b279639
		CHECK(result[0] == 0x6f4a3d15);
		CHECK(result[4] == 0x9b279639);
	}

/*	TEST_CASE("Append Data", "[sha1],[addData]")
	{
		char short1[] = "foo";
		char short2[] = "bar!";
		char fullBlock[] = "1.This.. 2.test. 4should 5.fit.. 5.right 6.into. 7.one.. 8.block";	// 64 bytes, 512 bits, one block
		char halfBlock[] = "0.This.. 1.is.a. 5.half. 5.block";	// 32 bytes

		SHA1 s;
		uint32_t * result;

		s.addData((uint8_t*)short1, 3);
		s.addData((uint8_t*)short2, 4);
		result = s.finish();
		// 2efa71cc 7e1039a9 2fde8d36 680e6762 8c8803b6
		CHECK(result[0] == 0x2efa71cc);
		CHECK(result[4] == 0x8c8803b6);

		s.addData((uint8_t*)fullBlock, 64);
		s.addData((uint8_t*)short1, 3);
		result = s.finish();
		// eef86c0c 427fba08 adf42538 d56d3933 a89eead0
		CHECK(result[0] == 0xeef86c0c);
		CHECK(result[4] == 0xa89eead0);

		s.addData((uint8_t*)short1, 3);
		s.addData((uint8_t*)fullBlock, 64);
		result = s.finish();
		// c524fd17 7df78ae5 ee5b2752 5816be91 7cee3b55
		CHECK(result[0] == 0xc524fd17);
		CHECK(result[4] == 0x7cee3b55);

		s.addData((uint8_t*)halfBlock, 32);
		s.addData((uint8_t*)halfBlock, 32);
		result = s.finish();
		// 91e904de f3b7d767 64056c38 2b3b012b 1f923629
		CHECK(result[0] == 0x91e904de);
		CHECK(result[4] == 0x1f923629);
	}

	TEST_CASE("Hash in one call")
	{
		char s1[] = "abc";

		SHA1 sha;
		uint32_t * result;

		result = sha.hash((uint8_t*)s1, 3);
		// a9993e36 4706816a ba3e2571 7850c26c 9cd0d89d
		CHECK(result[0] == 0xa9993e36);
		CHECK(result[4] == 0x9cd0d89d);
	}

	TEST_CASE("Hash String")
	{
		char s1[] = "abc";

		SHA1 sha;
		uint32_t * result;

		result = sha.hash(s1, 3);
		// a9993e36 4706816a ba3e2571 7850c26c 9cd0d89d
		CHECK(result[0] == 0xa9993e36);
		CHECK(result[4] == 0x9cd0d89d);
	}*/
}