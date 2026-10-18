#pragma once

// Tolerance checks for doctest, following the test conventions in
// https://github.com/srMemphis/space_traverse/issues/13. Use these instead of doctest::Approx.
//
// Scalars:  CHECK_NEAR(a, e)               |a - e| <= 1e-9 * |e|
//           CHECK_NEAR_REL(a, e, rel)      |a - e| <= rel * |e|
//           CHECK_NEAR_ABS(a, e, abs)      |a - e| <= abs
// Vectors:  CHECK_VEC_NEAR(a, e)           max|a_i - e_i| <= 1e-9 * max|e_i|
//           CHECK_VEC_NEAR_REL(a, e, rel)  max|a_i - e_i| <= rel * max|e_i|
//           CHECK_VEC_NEAR_ABS(a, e, abs)  max|a_i - e_i| <= abs
//
// Each macro is one doctest assertion. A relative form with a zero expected value fails: give an absolute
// tolerance sized to the quantity's scale instead.

#include <doctest/doctest.h>
#include <Eigen/Core>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace approx
{
	// The outcome of one comparison. doctest prints the report when the check fails.
	struct Comparison
	{
		bool passed = false;
		std::string report;

		explicit operator bool() const { return passed; }
	};

	namespace detail
	{
		inline std::ostream& Precise(std::ostream& out)
		{
			out.precision(17);
			return out;
		}

		inline Comparison Within(double actual, double expected, double allowed)
		{
			const double diff = std::abs(actual - expected);
			std::ostringstream report;
			report << Precise << "actual = " << actual << ", expected = " << expected << ", |diff| = " << diff
				   << ", allowed = " << allowed;
			return {diff <= allowed, report.str()};
		}

		// A relative tolerance of a zero expected value allows nothing, so the test must state an absolute one.
		inline Comparison ExpectedIsZero()
		{
			return {false, "expected is zero: use the _ABS form"};
		}

		template<typename Derived>
		void WriteVector(std::ostream& out, const Eigen::MatrixBase<Derived>& v)
		{
			out << "(";
			for (Eigen::Index i = 0; i < v.size(); ++i)
				out << (i == 0 ? "" : ", ") << static_cast<double>(v.coeff(i));
			out << ")";
		}

		template<typename Actual, typename Expected>
		Comparison SizeMismatch(const Eigen::MatrixBase<Actual>& actual, const Eigen::MatrixBase<Expected>& expected)
		{
			std::ostringstream report;
			report << "size mismatch: actual has " << actual.size() << " components, expected has " << expected.size();
			return {false, report.str()};
		}

		template<typename Derived>
		double MaxAbs(const Eigen::MatrixBase<Derived>& v)
		{
			double result = 0.0;
			for (Eigen::Index i = 0; i < v.size(); ++i)
				result = std::max(result, std::abs(static_cast<double>(v.coeff(i))));
			return result;
		}

		// Assumes equal sizes. A component that is not a number makes the whole comparison fail.
		template<typename Actual, typename Expected>
		Comparison VecWithin(const Eigen::MatrixBase<Actual>& actual, const Eigen::MatrixBase<Expected>& expected,
							 double allowed)
		{
			double maxDiff = 0.0;
			Eigen::Index worst = 0;
			for (Eigen::Index i = 0; i < expected.size(); ++i)
			{
				const double diff =
					std::abs(static_cast<double>(actual.coeff(i)) - static_cast<double>(expected.coeff(i)));
				if (!std::isnan(maxDiff) && !(diff <= maxDiff))
				{
					maxDiff = diff;
					worst = i;
				}
			}

			std::ostringstream report;
			report << Precise << "actual = ";
			WriteVector(report, actual);
			report << ", expected = ";
			WriteVector(report, expected);
			report << ", max |diff| = " << maxDiff << ", allowed = " << allowed << ", worst component = " << worst;
			return {maxDiff <= allowed, report.str()};
		}
	} // namespace detail

	inline Comparison NearRel(double actual, double expected, double rel)
	{
		if (expected == 0.0)
			return detail::ExpectedIsZero();
		return detail::Within(actual, expected, rel * std::abs(expected));
	}

	inline Comparison NearAbs(double actual, double expected, double abs)
	{
		return detail::Within(actual, expected, abs);
	}

	template<typename Actual, typename Expected>
	Comparison NearVecRel(const Eigen::MatrixBase<Actual>& actual, const Eigen::MatrixBase<Expected>& expected,
						  double rel)
	{
		static_assert(Actual::IsVectorAtCompileTime && Expected::IsVectorAtCompileTime, "compare vectors only");
		if (actual.size() != expected.size())
			return detail::SizeMismatch(actual, expected);
		const double scale = detail::MaxAbs(expected);
		if (scale == 0.0)
			return detail::ExpectedIsZero();
		return detail::VecWithin(actual, expected, rel * scale);
	}

	template<typename Actual, typename Expected>
	Comparison NearVecAbs(const Eigen::MatrixBase<Actual>& actual, const Eigen::MatrixBase<Expected>& expected,
						  double abs)
	{
		static_assert(Actual::IsVectorAtCompileTime && Expected::IsVectorAtCompileTime, "compare vectors only");
		if (actual.size() != expected.size())
			return detail::SizeMismatch(actual, expected);
		return detail::VecWithin(actual, expected, abs);
	}
} // namespace approx

template<>
struct doctest::StringMaker<approx::Comparison>
{
	static doctest::String convert(const approx::Comparison& comparison) { return comparison.report.c_str(); }
};

#define CHECK_NEAR(actual, expected) CHECK(::approx::NearRel((actual), (expected), 1e-9))
#define CHECK_NEAR_REL(actual, expected, rel) CHECK(::approx::NearRel((actual), (expected), (rel)))
#define CHECK_NEAR_ABS(actual, expected, abs) CHECK(::approx::NearAbs((actual), (expected), (abs)))

#define CHECK_VEC_NEAR(actual, expected) CHECK(::approx::NearVecRel((actual), (expected), 1e-9))
#define CHECK_VEC_NEAR_REL(actual, expected, rel) CHECK(::approx::NearVecRel((actual), (expected), (rel)))
#define CHECK_VEC_NEAR_ABS(actual, expected, abs) CHECK(::approx::NearVecAbs((actual), (expected), (abs)))
