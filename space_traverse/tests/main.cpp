#define DOCTEST_CONFIG_IMPLEMENT
#include "debug/Log.h"

#include <doctest/doctest.h>

int main(int argc, char** argv)
{
	// The LOG_* macros dereference a null logger until Log::Init runs.
	Log::Init("tests.log");

	return doctest::Context(argc, argv).run();
}
