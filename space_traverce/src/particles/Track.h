#pragma once
#include <vector>
#include <Eigen/Dense>

class Track
{
public:

	Track();
	~Track();

	// accessors
	const std::vector<Eigen::Vector3d>& GetPoints() const;
	const Eigen::Vector3d& GetLastPoint() const;
	const Eigen::Vector3d& GetVelocity() const;
	const Eigen::Vector3d& GetAcceleration() const;
	double GetTimestep() const;

	// modifiers

	// set this as initial values before macking step
	void SetStepSize(double step);							// step can be changed after each MakeStep() call
	void SetStartingPosition(Eigen::Vector3d point);
	void SetStartingVelocity(Eigen::Vector3d velo);
	void SetAcceleration(Eigen::Vector3d accel);			// use this to set starting value and to set new values after each MakeStep() call

	// make step
	const Eigen::Vector3d& MakeStep();

	// calc new acceleration and set it before calc new velocity
	void CalcNewVelocity();

	// clear all points exept first
	void ResetToStartingPosition();

private:
	double m_stepSize;		// fixed distance the particle will fly
	double m_dt;			// time in seconds the particle flew

	std::vector<Eigen::Vector3d> m_points;					// particle coordinates
	std::pair<Eigen::Vector3d, Eigen::Vector3d> m_velo;		// previous and current velocity
	std::pair<Eigen::Vector3d, Eigen::Vector3d> m_accel;	// previous and current acceleration
};
