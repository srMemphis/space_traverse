#include "utils/UnsignedRange.h"

#include <doctest/doctest.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
	std::string Print(const UnsignedRange& range)
	{
		std::ostringstream stream;
		stream << range;
		return stream.str();
	}

	UnsignedRange FromIntervals(const std::vector<std::pair<size_t, size_t>>& intervals)
	{
		UnsignedRange range;
		for (auto& [first, last] : intervals)
		{
			range.AddInterval(first, last);
		}
		return range;
	}
	// The elements a walk visits, in order.
	std::vector<size_t> Walk(const UnsignedRange& range)
	{
		std::vector<size_t> elements;
		for (RangeIterator iterator(range); !iterator.AtEnd(); iterator.Advance())
		{
			elements.push_back(iterator.Get());
		}
		return elements;
	}
} // namespace

TEST_SUITE("UnsignedRange")
{
	TEST_CASE("an empty range holds no elements")
	{
		UnsignedRange range;

		CHECK(range.Size() == 0);
		CHECK_FALSE(range.Contains(0));
		CHECK(Print(range) == "");
	}

	TEST_CASE("separate elements form disjoint ranges in ascending order")
	{
		UnsignedRange range;
		range.Add(7);
		range.Add(3);
		range.Add(5);

		CHECK(Print(range) == "[3,4) [5,6) [7,8) ");
		CHECK(range.Size() == 3);
		CHECK(range.Contains(5));
		CHECK_FALSE(range.Contains(4));
		CHECK_FALSE(range.Contains(6));
	}

	TEST_CASE("an element that fills the gap between two ranges joins them")
	{
		UnsignedRange range;
		range.Add(1);
		range.Add(3);
		range.Add(2);

		CHECK(Print(range) == "[1,4) ");
		CHECK(range.Size() == 3);
	}

	TEST_CASE("an element next to a range extends it")
	{
		UnsignedRange range;
		range.Add(5);

		SUBCASE("on the left")
		{
			range.Add(4);
			CHECK(Print(range) == "[4,6) ");
		}
		SUBCASE("on the right")
		{
			range.Add(6);
			CHECK(Print(range) == "[5,7) ");
		}
	}

	TEST_CASE("adding an element twice keeps one copy")
	{
		UnsignedRange range;
		range.Add(5);
		range.Add(5);

		CHECK(range.Size() == 1);
	}

	TEST_CASE("an interval holds both of its bounds in either order")
	{
		UnsignedRange range;

		SUBCASE("ascending")
		{
			range.AddInterval(2, 5);
		}
		SUBCASE("descending")
		{
			range.AddInterval(5, 2);
		}

		CHECK(Print(range) == "[2,6) ");
	}

	TEST_CASE("an interval overlapping several ranges joins them")
	{
		UnsignedRange range = FromIntervals({{0, 1}, {4, 5}, {8, 9}});
		range.AddInterval(1, 8);

		CHECK(Print(range) == "[0,10) ");
		CHECK(range.Size() == 10);
	}

	TEST_CASE("an interval that covers a range and stops short of the next one joins only what it covers")
	{
		UnsignedRange range = FromIntervals({{0, 1}, {4, 4}, {10, 11}});
		range.AddInterval(1, 6);

		// Elements 0..6 and 10 11.
		CHECK(Print(range) == "[0,7) [10,12) ");
		CHECK(range.Size() == 9);
		CHECK(range.GetPositionOf(10) == 7);
	}

	TEST_CASE("an interval that ends right before a range joins it")
	{
		UnsignedRange range = FromIntervals({{5, 7}, {2, 4}});
		// 5 is already in the range, so adding it changes nothing.
		range.Add(5);

		CHECK(Print(range) == "[2,8) ");
		CHECK(range.Size() == 6);
	}

	TEST_CASE("an interval left of all ranges is placed first")
	{
		UnsignedRange range = FromIntervals({{8, 9}});
		range.AddInterval(2, 4);

		CHECK(Print(range) == "[2,5) [8,10) ");
	}

	TEST_CASE("removing the middle of a range splits it")
	{
		UnsignedRange range = FromIntervals({{0, 9}});
		range.RemoveInterval(3, 5);

		CHECK(Print(range) == "[0,3) [6,10) ");
		CHECK(range.Size() == 7);
	}

	TEST_CASE("removing an interval drops the ranges inside it and trims the ones it overlaps")
	{
		UnsignedRange range = FromIntervals({{0, 4}, {7, 8}, {11, 12}});
		range.RemoveInterval(3, 11);

		CHECK(Print(range) == "[0,3) [12,13) ");
	}

	TEST_CASE("removing an interval outside every range changes nothing")
	{
		UnsignedRange range = FromIntervals({{2, 4}, {10, 12}});
		range.RemoveInterval(6, 8);

		CHECK(Print(range) == "[2,5) [10,13) ");
	}

	TEST_CASE("the position of an element counts the elements below it")
	{
		// Elements 2 3 4 | 8 9 | 15 at positions 0 1 2 | 3 4 | 5.
		UnsignedRange range = FromIntervals({{2, 4}, {8, 9}, {15, 15}});

		CHECK(range.GetPositionOf(2) == 0);
		CHECK(range.GetPositionOf(4) == 2);
		CHECK(range.GetPositionOf(8) == 3);
		CHECK(range.GetPositionOf(9) == 4);
		CHECK(range.GetPositionOf(15) == 5);
	}

	TEST_CASE("the position of an element outside the range is an error")
	{
		UnsignedRange range = FromIntervals({{2, 4}});

		CHECK_THROWS_AS(range.GetPositionOf(5), std::out_of_range);
		CHECK_THROWS_AS(UnsignedRange().GetPositionOf(0), std::out_of_range);
	}

	TEST_CASE("the union of two ranges holds the elements of both")
	{
		UnsignedRange left = FromIntervals({{0, 2}, {10, 12}});
		UnsignedRange right = FromIntervals({{2, 4}, {7, 7}});

		CHECK(Print(left + right) == "[0,5) [7,8) [10,13) ");
	}

	TEST_CASE("the difference of two ranges holds the elements of the first not in the second")
	{
		UnsignedRange left = FromIntervals({{0, 9}});
		UnsignedRange right = FromIntervals({{0, 1}, {5, 5}, {9, 9}});

		CHECK(Print(left - right) == "[2,5) [6,9) ");
	}

	TEST_CASE("splitting nodes into interior and boundary numbers each set from zero")
	{
		// Nodes 0..9, of which 0 1 2 8 9 lie on a Boundary.
		UnsignedRange all = FromIntervals({{0, 9}});
		UnsignedRange boundary = FromIntervals({{0, 2}, {8, 9}});

		UnsignedRange interior = all - boundary;

		CHECK(interior.Size() == 5);
		CHECK(interior.GetPositionOf(3) == 0);
		CHECK(interior.GetPositionOf(7) == 4);
		CHECK(boundary.GetPositionOf(8) == 3);
	}
}

TEST_SUITE("RangeIterator")
{
	TEST_CASE("a walk visits every element once in ascending order")
	{
		UnsignedRange range;

		std::vector<size_t> expected;
		SUBCASE("disjoint ranges")
		{
			range = FromIntervals({{2, 4}, {8, 9}, {15, 15}});
			expected = {2, 3, 4, 8, 9, 15};
		}
		SUBCASE("a single element")
		{
			range.Add(5);
			expected = {5};
		}
		SUBCASE("an empty range")
		{
		}

		CHECK(Walk(range) == expected);
	}

	TEST_CASE("a walk cannot be copied")
	{
		// A copy would keep walking the original's elements.
		CHECK_FALSE(std::is_copy_constructible_v<RangeIterator>);
		CHECK_FALSE(std::is_copy_assignable_v<RangeIterator>);
	}

	TEST_CASE("advancing a walk that has reached its end keeps it there")
	{
		UnsignedRange range;
		SUBCASE("an empty range")
		{
		}
		SUBCASE("a single element")
		{
			range.Add(5);
		}

		RangeIterator iterator(range);
		while (!iterator.AtEnd())
		{
			iterator.Advance();
		}

		size_t element = 0;
		CHECK_FALSE(iterator.Advance());
		CHECK_FALSE(iterator.Advance(element));
		CHECK(iterator.AtEnd());
	}
	TEST_CASE("advancing yields every element in ascending order and reports the last one")
	{
		UnsignedRange range;

		std::vector<size_t> expected;
		SUBCASE("disjoint ranges")
		{
			range = FromIntervals({{2, 4}, {8, 9}, {15, 15}});
			expected = {2, 3, 4, 8, 9, 15};
		}
		SUBCASE("adjacent ranges")
		{
			range = FromIntervals({{2, 4}, {5, 6}});
			expected = {2, 3, 4, 5, 6};
		}
		SUBCASE("a single element")
		{
			range.Add(5);
			expected = {5};
		}

		RangeIterator iterator(range);
		std::vector<size_t> elements{iterator.Get()};
		std::vector<bool> advanced;
		while (true)
		{
			advanced.push_back(iterator.Advance());
			if (!advanced.back())
			{
				break;
			}
			elements.push_back(iterator.Get());
		}

		// Every step lands on an element except the one past the last element.
		std::vector<bool> expectedAdvanced(expected.size(), true);
		expectedAdvanced.back() = false;

		CHECK(elements == expected);
		CHECK(advanced == expectedAdvanced);
	}

	TEST_CASE("the largest element is the end of the last range")
	{
		UnsignedRange range = FromIntervals({{2, 4}, {8, 9}, {15, 17}});
		RangeIterator iterator(range);

		CHECK(iterator.GetMax() == 17);
		iterator.Advance();
		CHECK(iterator.GetMax() == 17);
	}

	TEST_CASE("resetting a walk returns it to the smallest element")
	{
		UnsignedRange range = FromIntervals({{2, 4}, {8, 9}});
		RangeIterator iterator(range);
		iterator.Advance();
		iterator.Advance();
		iterator.Advance();
		REQUIRE(iterator.Get() == 8);

		iterator.Reset();

		CHECK(iterator.Get() == 2);
	}

	TEST_CASE("advancing into an element also stores it")
	{
		UnsignedRange range = FromIntervals({{2, 3}, {8, 8}});
		RangeIterator iterator(range);
		size_t element = 0;

		CHECK(iterator.Advance(element));
		CHECK(element == 3);
		CHECK(iterator.Advance(element));
		CHECK(element == 8);
	}
}
