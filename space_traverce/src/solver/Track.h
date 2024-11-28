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
	void SetStepSize(double step);
	void SetStartingPosition(Eigen::Vector3d point);
	void SetStartingVelocity(Eigen::Vector3d velo);
	void SetAcceleration(Eigen::Vector3d accel);
	void CalcNewVelocity();

	const Eigen::Vector3d& MakeStep();

private:
	double m_stepSize;		// fixed distance the particle will fly
	double m_dt;			// time in seconds the particle flew

	std::vector<Eigen::Vector3d> m_points;					// particle coordinates
	std::pair<Eigen::Vector3d, Eigen::Vector3d> m_velo;		// previous and current velocity
	std::pair<Eigen::Vector3d, Eigen::Vector3d> m_accel;	// previous and current acceleration
};
