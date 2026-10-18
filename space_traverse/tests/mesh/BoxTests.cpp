#include "mesh/Box.h"

#include <doctest/doctest.h>

namespace
{
	// Every bound of a split half is a midpoint or a bound of the parent, all exactly representable, so the
	// halves are compared exactly.
	void CheckBox(const Box& actual, const Box& expected)
	{
		CHECK(actual.xMin == expected.xMin);
		CHECK(actual.yMin == expected.yMin);
		CHECK(actual.zMin == expected.zMin);
		CHECK(actual.xMax == expected.xMax);
		CHECK(actual.yMax == expected.yMax);
		CHECK(actual.zMax == expected.zMax);
	}
} // namespace

TEST_SUITE("Box")
{
	TEST_CASE("a box contains a point inside it")
	{
		const Box box(-1, 0, 2, 1, 3, 4);

		CHECK(box.Contains(0, 1, 3));
	}

	TEST_CASE("a box does not contain a point outside it along any axis")
	{
		const Box box(-1, 0, 2, 1, 3, 4);

		SUBCASE("below x")
		{
			CHECK_FALSE(box.Contains(-1.5, 1, 3));
		}
		SUBCASE("above x")
		{
			CHECK_FALSE(box.Contains(1.5, 1, 3));
		}
		SUBCASE("below y")
		{
			CHECK_FALSE(box.Contains(0, -0.5, 3));
		}
		SUBCASE("above y")
		{
			CHECK_FALSE(box.Contains(0, 3.5, 3));
		}
		SUBCASE("below z")
		{
			CHECK_FALSE(box.Contains(0, 1, 1.5));
		}
		SUBCASE("above z")
		{
			CHECK_FALSE(box.Contains(0, 1, 4.5));
		}
	}

	TEST_CASE("a box contains the points on its boundary")
	{
		const Box box(-1, 0, 2, 1, 3, 4);

		SUBCASE("lower x face")
		{
			CHECK(box.Contains(-1, 1, 3));
		}
		SUBCASE("upper x face")
		{
			CHECK(box.Contains(1, 1, 3));
		}
		SUBCASE("lower y face")
		{
			CHECK(box.Contains(0, 0, 3));
		}
		SUBCASE("upper y face")
		{
			CHECK(box.Contains(0, 3, 3));
		}
		SUBCASE("lower z face")
		{
			CHECK(box.Contains(0, 1, 2));
		}
		SUBCASE("upper z face")
		{
			CHECK(box.Contains(0, 1, 4));
		}
		SUBCASE("edge")
		{
			CHECK(box.Contains(1, 3, 3));
		}
		SUBCASE("lowest corner")
		{
			CHECK(box.Contains(-1, 0, 2));
		}
		SUBCASE("highest corner")
		{
			CHECK(box.Contains(1, 3, 4));
		}
	}

	TEST_CASE("a box splits across its longest side into two halves that meet at its midpoint")
	{
		Box lower, upper;

		SUBCASE("longest along x")
		{
			const Box box(0, 1, -1, 4, 2, 1);
			CHECK(box.Split(lower, upper));
			CheckBox(lower, Box(0, 1, -1, 2, 2, 1));
			CheckBox(upper, Box(2, 1, -1, 4, 2, 1));
		}
		SUBCASE("longest along y")
		{
			const Box box(0, -3, -1, 1, 3, 1);
			CHECK(box.Split(lower, upper));
			CheckBox(lower, Box(0, -3, -1, 1, 0, 1));
			CheckBox(upper, Box(0, 0, -1, 1, 3, 1));
		}
		SUBCASE("longest along z")
		{
			const Box box(0, 0, 1, 1, 1, 6);
			CHECK(box.Split(lower, upper));
			CheckBox(lower, Box(0, 0, 1, 1, 1, 3.5));
			CheckBox(upper, Box(0, 0, 3.5, 1, 1, 6));
		}
	}

	TEST_CASE("a box with equally long sides splits across the first of them in x, y, z order")
	{
		Box lower, upper;

		SUBCASE("cube splits across x")
		{
			const Box box(0, 0, 0, 2, 2, 2);
			CHECK(box.Split(lower, upper));
			CheckBox(lower, Box(0, 0, 0, 1, 2, 2));
			CheckBox(upper, Box(1, 0, 0, 2, 2, 2));
		}
		SUBCASE("equal y and z split across y")
		{
			const Box box(0, 0, 0, 1, 2, 2);
			CHECK(box.Split(lower, upper));
			CheckBox(lower, Box(0, 0, 0, 1, 1, 2));
			CheckBox(upper, Box(0, 1, 0, 1, 2, 2));
		}
	}
}
