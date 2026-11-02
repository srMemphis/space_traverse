#include "particles/Track.h"
#include "support/approx.h"

#include <doctest/doctest.h>
#include <Eigen/Core>

#include <cmath>

TEST_SUITE("Track")
{
	TEST_CASE("every step advances the track by the step length along its velocity")
	{
		// A negative step length is taken by its magnitude.
		double stepLength = 0;
		SUBCASE("positive step length")
		{
			stepLength = 0.1;
		}
		SUBCASE("negative step length")
		{
			stepLength = -0.1;
		}

		Track track;
		track.SetStepSize(stepLength);
		track.SetStartingPosition({1, 2, 3});
		track.SetStartingVelocity({0, 3, 4});
		track.SetAcceleration({0, 0, 0});

		track.MakeStep();

		CHECK_VEC_NEAR(track.GetLastPoint(), Eigen::Vector3d(1, 2.06, 3.08));
	}

	TEST_CASE("the time of a step is the step length over the speed at its start")
	{
		const Eigen::Vector3d startVelocity{3, 4, 0};
		const Eigen::Vector3d acceleration{0, 50, 0};

		Track track;
		track.SetStepSize(0.1);
		track.SetStartingPosition({0, 0, 0});
		track.SetStartingVelocity(startVelocity);
		track.SetAcceleration(acceleration);

		// |(3, 4, 0)| = 5
		CHECK_NEAR(track.GetTimestep(), 0.1 / 5);

		track.MakeStep();
		CHECK_NEAR(track.GetTimestep(), 0.1 / 5);

		track.SetAcceleration(acceleration);
		track.CalcNewVelocity();
		track.MakeStep();

		// The velocity after the first step is (3, 4, 0) + (0, 50, 0) * 0.02 = (3, 5, 0).
		CHECK_NEAR(track.GetTimestep(), 0.1 / std::sqrt(34.0));
	}

	TEST_CASE("a track under constant acceleration follows the analytic trajectory")
	{
		const Eigen::Vector3d start{1, 2, 3};
		const Eigen::Vector3d startVelocity{1, 0, 0};
		const Eigen::Vector3d acceleration{-0.2, 0.3, 0};
		const int stepCount = 1000;

		Track track;
		track.SetStepSize(1e-3);
		track.SetStartingPosition(start);
		track.SetStartingVelocity(startVelocity);
		track.SetAcceleration(acceleration);

		double time = 0;
		for (int i = 0; i < stepCount; i++)
		{
			track.MakeStep();
			time += track.GetTimestep();
			track.SetAcceleration(acceleration);
			track.CalcNewVelocity();
		}

		const Eigen::Vector3d expected = start + startVelocity * time + acceleration * time * time / 2;

		// Loosened: the time of a step is the step length over the speed at its start, not over the mean speed
		// along the step, so the scheme is first order in the step length. Over these 1000 Steps of 1 mm the
		// position drifts from the analytic one by about 1e-4 of the displacement.
		CHECK_VEC_NEAR_REL(track.GetLastPoint(), expected, 1e-3);
	}

	// Known bug: https://github.com/srMemphis/space_traverse/issues/37
	// Now: velocity and acceleration of the previous step. Correct: those of the current step.
	TEST_CASE("a track reports the velocity and acceleration of its current step" * doctest::expected_failures(2))
	{
		const Eigen::Vector3d startVelocity{2, 0, 0};
		const Eigen::Vector3d startAcceleration{0, 1, 0};
		const Eigen::Vector3d nextAcceleration{0, 3, 0};

		Track track;
		track.SetStepSize(0.1);
		track.SetStartingPosition({0, 0, 0});
		track.SetStartingVelocity(startVelocity);
		track.SetAcceleration(startAcceleration);
		track.MakeStep();
		track.SetAcceleration(nextAcceleration);
		track.CalcNewVelocity();

		// Velocity Verlet with the step time 0.1 / |(2, 0, 0)| = 0.05:
		// (2, 0, 0) + 0.05 * ((0, 1, 0) + (0, 3, 0)) / 2 = (2, 0.1, 0).
		CHECK_VEC_NEAR(track.GetVelocity(), Eigen::Vector3d(2, 0.1, 0));
		CHECK_VEC_NEAR(track.GetAcceleration(), nextAcceleration);
	}

	TEST_CASE("resetting a track keeps only its start point")
	{
		Track track;
		track.SetStepSize(0.1);
		track.SetStartingPosition({1, 2, 3});
		track.SetStartingVelocity({1, 0, 0});
		track.SetAcceleration({0, 0, 0});
		track.MakeStep();
		track.MakeStep();

		track.ResetToStartingPosition();

		REQUIRE(track.GetPoints().size() == 1);
		CHECK_VEC_NEAR(track.GetLastPoint(), Eigen::Vector3d(1, 2, 3));
	}
}
