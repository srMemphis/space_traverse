#include "support/fixture.h"

#include <doctest/doctest.h>

TEST_SUITE("fixture")
{
	TEST_CASE("a fixture path is absolute and points into the test data folder")
	{
		const auto path = FixturePath("cube.msh");

		CHECK(path.is_absolute());
		CHECK(path.filename() == "cube.msh");
		CHECK(path.parent_path().filename() == "data");
		CHECK(path.parent_path().parent_path().filename() == "tests");
	}
}
