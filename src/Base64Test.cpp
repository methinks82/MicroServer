#include "../test/catch.hpp"
#include "Base64.hpp"

TEST_CASE("Convert to base64")
{
	SECTION("Encode full value")
	{
		uint8_t data[] = "light wor";
		REQUIRE(ams::encodeBase64(data, 9) == "bGlnaHQgd29y");
	}

	SECTION("Encode with single padding")
	{
		uint8_t data[] = "light work.";
		REQUIRE(ams::encodeBase64(data, 11) == "bGlnaHQgd29yay4=");
	}

	SECTION("Encode with double padding")
	{
		uint8_t data[] = "light work";
		REQUIRE(ams::encodeBase64(data, 10) == "bGlnaHQgd29yaw==");
	}
}