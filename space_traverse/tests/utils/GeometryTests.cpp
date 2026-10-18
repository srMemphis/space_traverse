#include "support/approx.h"
#include "utils/Geometry.h"

#include <doctest/doctest.h>
#include <Eigen/Geometry>

#include <cmath>

namespace
{
	using Eigen::Vector3d;
	using Eigen::Vector4d;

	// An arbitrary rigid motion: a rotation about a skew axis followed by a shift.
	Vector3d Moved(const Vector3d& p)
	{
		const Eigen::AngleAxisd rotation(0.7, Vector3d(1.0, 2.0, -0.5).normalized());
		return rotation * p + Vector3d(1.0, -2.0, 5.0);
	}
} // namespace

TEST_SUITE("Geometry")
{
	TEST_CASE("a right triangle has half the product of its legs as its area")
	{
		CHECK_NEAR(Geometry::TriangleArea({0, 0, 0}, {3, 0, 0}, {0, 4, 0}), 6.0);
	}

	TEST_CASE("an equilateral triangle with side 2 has area sqrt(3)")
	{
		CHECK_NEAR(Geometry::TriangleArea({0, 0, 0}, {2, 0, 0}, {1, std::sqrt(3.0), 0}), std::sqrt(3.0));
	}

	TEST_CASE("moving a triangle keeps its area")
	{
		CHECK_NEAR(Geometry::TriangleArea(Moved({0, 0, 0}), Moved({3, 0, 0}), Moved({0, 4, 0})), 6.0);
	}

	TEST_CASE("the unit right tetrahedron has volume one sixth")
	{
		CHECK_NEAR(
			Geometry::TetrahedronVolume(Vector3d{0, 0, 0}, Vector3d{1, 0, 0}, Vector3d{0, 1, 0}, Vector3d{0, 0, 1}),
			1.0 / 6.0);
	}

	TEST_CASE("a right tetrahedron with legs 2, 3 and 4 has volume 4")
	{
		CHECK_NEAR(
			Geometry::TetrahedronVolume(Vector3d{0, 0, 0}, Vector3d{2, 0, 0}, Vector3d{0, 3, 0}, Vector3d{0, 0, 4}),
			4.0);
	}

	TEST_CASE("swapping two vertices of a tetrahedron flips the sign of its volume")
	{
		CHECK_NEAR(
			Geometry::TetrahedronVolume(Vector3d{0, 0, 0}, Vector3d{0, 1, 0}, Vector3d{1, 0, 0}, Vector3d{0, 0, 1}),
			-1.0 / 6.0);
	}

	TEST_CASE("moving a tetrahedron keeps its volume")
	{
		CHECK_NEAR(Geometry::TetrahedronVolume(Moved({0, 0, 0}), Moved({1, 0, 0}), Moved({0, 1, 0}), Moved({0, 0, 1})),
				   1.0 / 6.0);
	}

	TEST_CASE("a tetrahedron given by mesh nodes has the same signed volume")
	{
		SUBCASE("positive vertex order")
		{
			CHECK_NEAR(Geometry::TetrahedronVolume(Node{0, 0, 0}, Node{2, 0, 0}, Node{0, 3, 0}, Node{0, 0, 4}), 4.0);
		}
		SUBCASE("negative vertex order")
		{
			CHECK_NEAR(Geometry::TetrahedronVolume(Node{0, 0, 0}, Node{0, 3, 0}, Node{2, 0, 0}, Node{0, 0, 4}), -4.0);
		}
	}

	// The barycentric coordinate of the first point is 1 there and 0 on the plane of the other three, so its
	// gradient points from that plane to the point and has length one over the point's height above the plane.
	TEST_CASE("the barycentric gradients of the unit right tetrahedron")
	{
		const Vector3d o{0, 0, 0}, x{1, 0, 0}, y{0, 1, 0}, z{0, 0, 1};

		SUBCASE("right-angle vertex")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(o, x, y, z), Vector3d(-1, -1, -1));
		}
		SUBCASE("vertex on x")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(x, o, y, z), Vector3d(1, 0, 0));
		}
		SUBCASE("vertex on y")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(y, x, o, z), Vector3d(0, 1, 0));
		}
		SUBCASE("vertex on z")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(z, x, y, o), Vector3d(0, 0, 1));
		}
	}

	TEST_CASE("the barycentric gradients of a moved right tetrahedron with legs 2, 3 and 4")
	{
		// Legs along the axes, shifted by (1, -2, 5): the coordinates are 1 - x'/2 - y'/3 - z'/4, x'/2, y'/3, z'/4.
		const Vector3d shift{1, -2, 5};
		const Vector3d o = shift, x = shift + Vector3d{2, 0, 0}, y = shift + Vector3d{0, 3, 0},
					   z = shift + Vector3d{0, 0, 4};

		SUBCASE("right-angle vertex")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(o, x, y, z), Vector3d(-1.0 / 2, -1.0 / 3, -1.0 / 4));
		}
		SUBCASE("vertex on x")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(x, o, y, z), Vector3d(1.0 / 2, 0, 0));
		}
		SUBCASE("vertex on y")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(y, x, o, z), Vector3d(0, 1.0 / 3, 0));
		}
		SUBCASE("vertex on z")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(z, x, y, o), Vector3d(0, 0, 1.0 / 4));
		}
	}

	TEST_CASE("a barycentric gradient does not depend on the order of the opposite face's vertices")
	{
		const Vector3d o{0, 0, 0}, x{1, 0, 0}, y{0, 1, 0}, z{0, 0, 1};

		SUBCASE("x, y, z")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(o, x, y, z), Vector3d(-1, -1, -1));
		}
		SUBCASE("y, x, z")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(o, y, x, z), Vector3d(-1, -1, -1));
		}
		SUBCASE("z, y, x")
		{
			CHECK_VEC_NEAR(Geometry::BarycentricGradient(o, z, y, x), Vector3d(-1, -1, -1));
		}
	}

	TEST_CASE("the plane through three points is given by its unit normal and offset")
	{
		SUBCASE("horizontal plane z = 2")
		{
			CHECK_VEC_NEAR(Geometry::ThreePointPlane({0, 0, 2}, {1, 0, 2}, {0, 1, 2}), Vector4d(0, 0, 1, -2));
		}
		SUBCASE("tilted plane x + y + z = 1")
		{
			const double k = 1.0 / std::sqrt(3.0);
			CHECK_VEC_NEAR(Geometry::ThreePointPlane({1, 0, 0}, {0, 1, 0}, {0, 0, 1}), Vector4d(k, k, k, -k));
		}
	}

	TEST_CASE("the plane normal follows the right-hand rule of the point order")
	{
		CHECK_VEC_NEAR(Geometry::ThreePointPlane({0, 0, 2}, {0, 1, 2}, {1, 0, 2}), Vector4d(0, 0, -1, 2));
	}
}
