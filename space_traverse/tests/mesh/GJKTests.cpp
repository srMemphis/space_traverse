#include "mesh/GJK.h"

#include <doctest/doctest.h>

#include <vector>

namespace
{
	// Runs the intersection test of a box and the tetrahedron with vertices a, b, c, d.
	bool Intersect(const Box& box, Node a, Node b, Node c, Node d)
	{
		const std::vector<Node> nodes{a, b, c, d};
		const ETetrahedron tet{{0, 1, 2, 3}};
		return GJK::Check(box, tet, nodes);
	}

	const Box unitBox(0, 0, 0, 1, 1, 1);
} // namespace

TEST_SUITE("GJK")
{
	TEST_CASE("a tetrahedron with a vertex inside a box intersects it")
	{
		CHECK(Intersect(unitBox, {0.5, 0.5, 0.5}, {3, 0, 0}, {0, 3, 0}, {0, 0, 3}));
	}

	TEST_CASE("a tetrahedron passing through a box with every vertex outside it intersects it")
	{
		// The face at z = 0.5 covers the box's cross-section there.
		CHECK(Intersect(unitBox, {-1, -1, 0.5}, {3, -1, 0.5}, {1, 3, 0.5}, {1, 1, 3}));
	}

	TEST_CASE("a tetrahedron far from a box does not intersect it")
	{
		CHECK_FALSE(Intersect(unitBox, {5, 0, 0}, {6, 0, 0}, {5, 1, 0}, {5, 0, 1}));
	}

	TEST_CASE("a tetrahedron beyond a box corner does not intersect it although their bounding boxes overlap")
	{
		// Every vertex has x + y + z >= 3.2, every box point has x + y + z <= 3.
		CHECK_FALSE(Intersect(unitBox, {2, 0.6, 0.6}, {0.6, 2, 0.6}, {0.6, 0.6, 2}, {2, 2, 2}));
	}

	TEST_CASE("a tetrahedron inside a box intersects it")
	{
		CHECK(Intersect(Box(-2, -2, -2, 2, 2, 2), {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}));
	}

	TEST_CASE("a box inside a tetrahedron intersects it")
	{
		// Every box point has x + y + z <= 6 < 10.
		CHECK(Intersect(Box(1, 1, 1, 2, 2, 2), {0, 0, 0}, {10, 0, 0}, {0, 10, 0}, {0, 0, 10}));
	}

	TEST_CASE("a tetrahedron centred on the box centre intersects it")
	{
		// Alternate corners of a cube centred on the origin form a regular tetrahedron with its centroid there.
		const Box box(-1, -1, -1, 1, 1, 1);

		SUBCASE("tetrahedron smaller than the box")
		{
			CHECK(Intersect(box, {0.5, 0.5, 0.5}, {0.5, -0.5, -0.5}, {-0.5, 0.5, -0.5}, {-0.5, -0.5, 0.5}));
		}
		SUBCASE("tetrahedron larger than the box")
		{
			CHECK(Intersect(box, {3, 3, 3}, {3, -3, -3}, {-3, 3, -3}, {-3, -3, 3}));
		}
	}

	// The test gives up after a fixed number of iterations. It must then keep the tetrahedron: a box that drops
	// a tetrahedron it touches hides it from the element search, while a kept one costs only an exact check there.
	// This pair is about 0.064 apart, yet the search cycles until it gives up.
	TEST_CASE("a tetrahedron the test cannot settle within its iteration limit is kept as intersecting")
	{
		CHECK(Intersect(unitBox, {0.6, 0.3, 1.6}, {-0.1, -0.9, 1.8}, {0.8, -0.2, 0.8}, {-0.6, 0.2, 1.8}));
	}
}
