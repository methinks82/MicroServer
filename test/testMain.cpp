#ifdef AUTOMATIC_TEST

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#else

#define CATCH_CONFIG_RUNNER

#include <iostream>
#include "catch.hpp"

// if running manually, wait for user input before closing the window
int main(int argc, char * argv[])
{
	int result = Catch::Session().run(argc, argv);
	system("PAUSE");
	return (result < 0xff ? result : 0xff);
}

#endif // AUTOMATIC_TEST