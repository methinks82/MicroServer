#include "..\test\catch.hpp"
#include "Endians.hpp"

using namespace gaf::util;

TEST_CASE("Endian Tests")
{
	SECTION("Test endian")
	{
		// If testing on a little endian system
		REQUIRE_FALSE(isPlatformBigEndian());
		// Big endian system
		// REQUIRE(isBigEndian());
	}

	SECTION("Fix 64bit byte order")
	{
		REQUIRE(swapBytes(0x01234567) == 0x67452301);
	}

	SECTION("Test net byte order correction")
	{
		// If testing on a Little Endian System:
		REQUIRE(correctForNetByteOrder(0x11223344) == 0x44332211);
		// Test for Big Endian:
		// REQUIRE(correctForNetByteOrder(0x11223344) == 0x11223344);
	}
}