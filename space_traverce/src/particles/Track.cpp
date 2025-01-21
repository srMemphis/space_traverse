#include "Track.h"

#include "debug/Log.h"

#include <cmath>
#include <algorithm>

Track::Track()
	:m_stepSize(1), m_dt(0)
{
}

Track::~Track()
{
}

const std::vector<Eigen::Vector3d>& Track::GetPoints() const
{
	return m_points;
}

const Eigen::Vector3d& Track::GetLastPoint() const
{
	return m_points.back();
}

const Eigen::Vector3d& Track::GetVelocity() const
{
	return m_velo.second;
}

const Eigen::Vector3d& Track::GetAcceleration() const
{
	return m_accel.second;
}

double Track::GetTimestep() const
{
	return m_dt;
}

void Track::SetStepSize(double step)
{
	m_stepSize = std::abs(step);
}

const Eigen::Vector3d& Track::MakeStep()
{
	// Verlet velocity algorythm

	// TODO: fix verlet algoritm. velosity and acceleration are incorrect
	// get direction of particle
	Eigen::Vector3d dir = m_velo.first + 0.5 * m_accel.first;
	
	// normalize it to step size
	dir.normalize();
	dir *= m_stepSize;

	// get new point
	auto& oldpoint = GetLastPoint();
	m_points.push_back(oldpoint + dir);

	// calc dt
	m_dt = m_stepSize / m_velo.first.norm();

	return m_points.back();
}

void Track::SetStartingPosition(Eigen::Vector3d point)
{
	if (m_points.size() == 0)
	{
		m_points.resize(1);
	}

	m_points.at(0) = point;
}

void Track::SetStartingVelocity(Eigen::Vector3d velo)
{
	m_velo.first = velo;
}

void Track::CalcNewVelocity()
{
	m_velo.second = m_velo.first + 0.5 * m_dt * (m_accel.first + m_accel.second);

	std::swap(m_velo.first, m_velo.second);
}

void Track::ResetToStartingPosition()
{
	m_points.resize(1);
}

void Track::SetAcceleration(Eigen::Vector3d accel)
{
	m_accel.second = accel;

	std::swap(m_accel.first, m_accel.second);
}
