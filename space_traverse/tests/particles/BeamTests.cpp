#include "particles/Beam.h"
#include "support/approx.h"

#include <doctest/doctest.h>
#include <Eigen/Core>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
	const double speedOfLight = 299792458;                              // m/s, exact
	const double electronMass = 9.1093837139e-31;                       // kg, CODATA 2022
	const double electronCharge = -1.602176634e-19;                     // C, exact
	const double electronRestEnergy = 0.511e6;                          // eV, the value the Scenario sets
	const double relativisticEnergy = 0.511e6;                          // eV, gives the Lorentz factor 2
	const double relativisticSpeed = speedOfLight * std::sqrt(3.0) / 2; // speed at the Lorentz factor 2

	// An electron Beam of 3 * 2^2 = 12 Tracks with zero Divergence, ready to run once it has its stubs.
	void SetUpElectronBeam(Beam& beam, double energy, const Eigen::Vector3d& direction, double environmentSize,
						   uint32_t stepCount)
	{
		beam.SetParticle({electronCharge, electronRestEnergy});
		beam.SetEnergy(energy);
		beam.SetCenterPosition(0, 0, 0);
		beam.SetDirection(direction.x(), direction.y(), direction.z());
		beam.SetRadius(1e-3);
		beam.SetDivergence(0);
		beam.DetermineStepSize(environmentSize, stepCount);
		beam.GeneratePrimaryPosition(2);
	}

	auto UniformField(const Eigen::Vector3d& field)
	{
		return [field](double, double, double) { return field; };
	}

	bool StopAfterFirstStep(double, double, double)
	{
		return true;
	}
} // namespace

TEST_SUITE("Beam")
{
	TEST_CASE("a beam of n rings starts 3 n^2 tracks")
	{
		uint32_t n = 0;
		SUBCASE("n = 1")
		{
			n = 1;
		}
		SUBCASE("n = 2")
		{
			n = 2;
		}
		SUBCASE("n = 5")
		{
			n = 5;
		}

		Beam beam;
		beam.SetDirection(0, 0, 1);
		beam.GeneratePrimaryPosition(n);

		CHECK(beam.GetTracks().size() == 3 * n * n);
	}

	TEST_CASE("every start point lies in the plane through the beam origin across the beam direction")
	{
		const Eigen::Vector3d origin{0.1, -0.2, 0.3};
		const Eigen::Vector3d direction = Eigen::Vector3d(1, 2, 2) / 3;
		const double radius = 0.05;

		Beam beam;
		beam.SetCenterPosition(origin.x(), origin.y(), origin.z());
		beam.SetDirection(1, 2, 2);
		beam.SetRadius(radius);
		beam.GeneratePrimaryPosition(3);

		for (const auto& track : beam.GetTracks())
		{
			REQUIRE(track.GetPoints().size() == 1);
			// Zero expected: the tolerance is the default relative one applied to the Beam radius.
			CHECK_NEAR_ABS((track.GetLastPoint() - origin).dot(direction), 0, 1e-9 * radius);
		}
	}

	// Known bug: https://github.com/srMemphis/space_traverse/issues/37
	// Now: the outer ring of start points lies outside the beam radius (by 0.6% for n = 3). Correct: every start
	// point lies within the beam radius of the beam origin.
	TEST_CASE("every start point lies within the beam radius of the beam origin" * doctest::expected_failures(1))
	{
		const Eigen::Vector3d origin{0.1, -0.2, 0.3};
		const double radius = 0.05;

		Beam beam;
		beam.SetCenterPosition(origin.x(), origin.y(), origin.z());
		beam.SetDirection(1, 2, 2);
		beam.SetRadius(radius);
		beam.GeneratePrimaryPosition(3);

		double farthest = 0;
		for (const auto& track : beam.GetTracks())
			farthest = std::max(farthest, (track.GetLastPoint() - origin).norm());

		// The margin admits round-off for start points placed exactly on the edge of the disk.
		CHECK(farthest <= radius * (1 + 1e-9));
	}

	TEST_CASE("a beam with no origin set is centred on the coordinate origin")
	{
		Beam beam;
		beam.SetDirection(0, 0, 1);
		beam.SetRadius(1);
		beam.GeneratePrimaryPosition(2);

		// The start points of every ring are spread evenly in azimuth, so their mean is the Beam origin.
		Eigen::Vector3d mean = Eigen::Vector3d::Zero();
		for (const auto& track : beam.GetTracks())
			mean += track.GetLastPoint();
		mean /= static_cast<double>(beam.GetTracks().size());

		// Zero expected: the tolerance is the default relative one applied to the Beam radius.
		CHECK_VEC_NEAR_ABS(mean, Eigen::Vector3d::Zero(), 1e-9);
	}

	TEST_CASE("with zero divergence every track starts along the beam direction")
	{
		Beam beam;
		SetUpElectronBeam(beam, 1000, {1, 2, 2}, 0.1, 100);
		beam.SetFieldFunction(UniformField(Eigen::Vector3d::Zero()));
		beam.SetStoppingFunction(StopAfterFirstStep);

		beam.Run();

		// Step length = 0.1 m / 100 Steps.
		const Eigen::Vector3d firstStep = 1e-3 * Eigen::Vector3d(1, 2, 2) / 3;
		for (const auto& track : beam.GetTracks())
		{
			REQUIRE(track.GetPoints().size() == 2);
			CHECK_VEC_NEAR(track.GetPoints()[1] - track.GetPoints()[0], firstStep);
		}
	}

	TEST_CASE("the start speed of every track follows from the beam energy")
	{
		Beam beam;
		SetUpElectronBeam(beam, relativisticEnergy, {0, 0, 1}, 0.1, 100);
		beam.SetFieldFunction(UniformField(Eigen::Vector3d::Zero()));
		beam.SetStoppingFunction(StopAfterFirstStep);

		beam.Run();

		// The time of the first Step is the Step length over the start speed.
		for (const auto& track : beam.GetTracks())
			CHECK_NEAR(1e-3 / track.GetTimestep(), relativisticSpeed);
	}

	TEST_CASE("in a field along the beam every track runs straight to the stopping plane")
	{
		// The Field (0, 0, -100) V/m pushes electrons along +z. The plane sits half a Step past 100 Steps of
		// 0.1 mm, so every Track stops after 101 Steps.
		const double plane = 0.01005;

		Beam beam;
		SetUpElectronBeam(beam, 1000, {0, 0, 1}, 0.01, 100);
		beam.SetFieldFunction(UniformField({0, 0, -100}));
		beam.SetStoppingFunction([plane](double, double, double z) { return z >= plane; });

		beam.Run();

		for (const auto& track : beam.GetTracks())
		{
			const auto& points = track.GetPoints();
			REQUIRE(points.size() == 102);
			CHECK(points[points.size() - 2].z() < plane);
			CHECK(points.back().z() >= plane);
			CHECK_VEC_NEAR(Eigen::Vector2d(points.back().head<2>()), Eigen::Vector2d(points.front().head<2>()));
		}
	}

	// Known bug: https://github.com/srMemphis/space_traverse/issues/37
	// Now: the acceleration is the force over the rest mass, twice too large at the Lorentz factor 2. Correct: a
	// force across the velocity accelerates by force / (Lorentz factor * rest mass).
	TEST_CASE("a field across the beam bends the first step by the relativistic acceleration" *
			  doctest::expected_failures(1))
	{
		const double field = 1e6; // V/m along x
		const double stepLength = 1e-3;

		Beam beam;
		SetUpElectronBeam(beam, relativisticEnergy, {0, 0, 1}, 0.1, 100);
		beam.SetFieldFunction(UniformField({field, 0, 0}));
		beam.SetStoppingFunction(StopAfterFirstStep);

		beam.Run();

		// The first Step points along v + a * dt / 2 with dt = Step length / start speed, and the force
		// charge * field is across v, so a = charge * field / (2 * rest mass).
		const double acceleration = electronCharge * field / (2 * electronMass);
		const double across = acceleration * (stepLength / relativisticSpeed) / 2;
		const double expectedX = stepLength * across / std::hypot(across, relativisticSpeed);

		const auto& track = beam.GetTracks().front();
		CHECK_NEAR(track.GetLastPoint().x() - track.GetPoints().front().x(), expectedX);
	}

	TEST_CASE("resetting a beam returns every track to its start point")
	{
		Beam beam;
		SetUpElectronBeam(beam, 1000, {0, 0, 1}, 0.1, 100);
		beam.SetFieldFunction(UniformField(Eigen::Vector3d::Zero()));
		beam.SetStoppingFunction(StopAfterFirstStep);

		std::vector<Eigen::Vector3d> starts;
		for (const auto& track : beam.GetTracks())
			starts.push_back(track.GetLastPoint());

		beam.Run();
		beam.Reset();

		REQUIRE(beam.GetTracks().size() == starts.size());
		for (size_t i = 0; i < starts.size(); i++)
		{
			REQUIRE(beam.GetTracks()[i].GetPoints().size() == 1);
			CHECK_VEC_NEAR(beam.GetTracks()[i].GetLastPoint(), starts[i]);
		}
	}
}
