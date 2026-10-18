#include "support/approx.h"

#include <doctest/doctest.h>
#include <Eigen/Core>

#include <cmath>

// The failing cases use doctest::expected_failures to show that each check fails when it should. They are not
// known bugs, so they carry no "Known bug" marker.
TEST_SUITE("approx")
{
	TEST_CASE("a scalar within the default relative tolerance is near")
	{
		CHECK_NEAR(1.0 + 1e-10, 1.0);
	}

	TEST_CASE("a scalar outside the default relative tolerance is not near" * doctest::expected_failures(1))
	{
		CHECK_NEAR(1.0 + 1e-8, 1.0);
	}

	TEST_CASE("a scalar failure reports actual, expected, difference and allowed with 17 digits")
	{
		CHECK(approx::NearRel(1.5, 1.0, 1e-9).report ==
			  "actual = 1.5, expected = 1, |diff| = 0.5, allowed = 1.0000000000000001e-09");
	}

	TEST_CASE("a relative comparison with a zero expected scalar fails" * doctest::expected_failures(2))
	{
		CHECK_NEAR(0.0, 0.0);
		CHECK_NEAR_REL(0.0, 0.0, 0.5);
	}

	TEST_CASE("a relative comparison with a zero expected scalar points to the absolute form")
	{
		CHECK(approx::NearRel(0.0, 0.0, 1e-9).report == "expected is zero: use the _ABS form");
	}

	TEST_CASE("a scalar within a given relative tolerance is near")
	{
		CHECK_NEAR_REL(1.0e-12 * (1.0 + 1e-4), 1.0e-12, 1e-3);
		CHECK_NEAR_REL(-2.0e6, -2.0e6 * (1.0 - 1e-4), 1e-3);
	}

	TEST_CASE("a scalar outside a given relative tolerance is not near" * doctest::expected_failures(1))
	{
		CHECK_NEAR_REL(1.0e-12 * (1.0 + 1e-2), 1.0e-12, 1e-3);
	}

	TEST_CASE("a scalar within a given absolute tolerance is near")
	{
		CHECK_NEAR_ABS(1e-13, 0.0, 1e-12);
		CHECK_NEAR_ABS(-3.0, -3.5, 0.5);
	}

	TEST_CASE("a scalar outside a given absolute tolerance is not near" * doctest::expected_failures(1))
	{
		CHECK_NEAR_ABS(1e-11, 0.0, 1e-12);
	}

	TEST_CASE("a scalar that is not a number is never near" * doctest::expected_failures(2))
	{
		CHECK_NEAR(std::nan(""), 1.0);
		CHECK_NEAR_ABS(std::nan(""), 0.0, 1.0);
	}

	TEST_CASE("a vector within the default relative tolerance of its magnitude is near")
	{
		// Zero components need no special case: the tolerance scales with the largest component.
		CHECK_VEC_NEAR(Eigen::Vector3d(1e-10, 0.0, -1.0), Eigen::Vector3d(0.0, 0.0, -1.0));
	}

	TEST_CASE("a vector outside the default relative tolerance of its magnitude is not near" *
			  doctest::expected_failures(1))
	{
		CHECK_VEC_NEAR(Eigen::Vector3d(1e-8, 0.0, -1.0), Eigen::Vector3d(0.0, 0.0, -1.0));
	}

	TEST_CASE("a vector within a given relative tolerance is near")
	{
		CHECK_VEC_NEAR_REL(Eigen::Vector3d(2.0, 1.0e6, 3.0), Eigen::Vector3d(1.0, 1.0e6 + 5.0, 3.0), 1e-5);
	}

	TEST_CASE("a vector outside a given relative tolerance is not near" * doctest::expected_failures(1))
	{
		CHECK_VEC_NEAR_REL(Eigen::Vector3d(20.0, 1.0e6, 3.0), Eigen::Vector3d(1.0, 1.0e6, 3.0), 1e-5);
	}

	TEST_CASE("a vector within a given absolute tolerance is near")
	{
		CHECK_VEC_NEAR_ABS(Eigen::Vector3d(1e-13, -1e-13, 0.0), Eigen::Vector3d::Zero(), 1e-12);
	}

	TEST_CASE("a vector outside a given absolute tolerance is not near" * doctest::expected_failures(1))
	{
		CHECK_VEC_NEAR_ABS(Eigen::Vector3d(1e-13, -1e-11, 0.0), Eigen::Vector3d::Zero(), 1e-12);
	}

	TEST_CASE("a relative comparison with the zero vector fails" * doctest::expected_failures(2))
	{
		CHECK_VEC_NEAR(Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero());
		CHECK_VEC_NEAR_REL(Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(), 0.5);
	}

	TEST_CASE("a relative comparison with the zero vector points to the absolute form")
	{
		CHECK(approx::NearVecRel(Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(), 1e-9).report ==
			  "expected is zero: use the _ABS form");
	}

	TEST_CASE("a vector with a component that is not a number is never near" * doctest::expected_failures(2))
	{
		CHECK_VEC_NEAR(Eigen::Vector3d(std::nan(""), 0.0, 1.0), Eigen::Vector3d(0.0, 0.0, 1.0));
		CHECK_VEC_NEAR_ABS(Eigen::Vector3d(0.0, std::nan(""), 0.0), Eigen::Vector3d::Zero(), 1.0);
	}

	TEST_CASE("vectors of different sizes are never near" * doctest::expected_failures(1))
	{
		CHECK_VEC_NEAR_ABS(Eigen::Vector2d(0.0, 0.0), Eigen::Vector3d::Zero(), 1.0);
	}

	TEST_CASE("a vector failure reports both vectors, the largest difference, allowed and the worst component")
	{
		CHECK(approx::NearVecRel(Eigen::Vector3d(1.0, 2.0, 3.0), Eigen::Vector3d(1.0, 2.5, 4.0), 0.125).report ==
			  "actual = (1, 2, 3), expected = (1, 2.5, 4), max |diff| = 1, allowed = 0.5, worst component = 2");
		CHECK(approx::NearVecAbs(Eigen::Vector3d(0.1, 0.0, 0.0), Eigen::Vector3d::Zero(), 0.0).report ==
			  "actual = (0.10000000000000001, 0, 0), expected = (0, 0, 0), max |diff| = 0.10000000000000001, "
			  "allowed = 0, worst component = 0");
	}

	TEST_CASE("a size mismatch reports both sizes")
	{
		CHECK(approx::NearVecAbs(Eigen::Vector2d(0.0, 0.0), Eigen::Vector3d::Zero(), 1.0).report ==
			  "size mismatch: actual has 2 components, expected has 3");
	}
}
