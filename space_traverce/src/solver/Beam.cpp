#include "Beam.h"

#include "debug/Log.h"

Beam::Beam()
	:m_particle({1.60217633e-19, 9.1093837e-31}), m_radius(1)
{
}

Beam::~Beam()
{
}

const std::vector<Track>& Beam::GetTracks() const
{
	return m_tracks;
}

void Beam::SetRadius(double rad)
{
	m_radius = rad;
}

void Beam::SetCenterPosition(double x, double y, double z)
{
	m_centerPosition(0) = x;
	m_centerPosition(1) = y;
	m_centerPosition(2) = z;
}

void Beam::SetDirection(double x, double y, double z)
{
	m_direction(0) = x;
	m_direction(1) = y;
	m_direction(2) = z;
}

void Beam::SetParticle(Particle p)
{
	m_particle = p;
}

void Beam::SetStoppingFunction(std::function<bool(double, double, double)> func)
{
	m_stoppingFunction = func;
}

void Beam::SetFieldFunction(std::function<Eigen::Vector3d(double x, double y, double z)> func)
{
	m_fieldFunction = func;
}

void Beam::Reset()
{
	// TODO
}

void Beam::Run()
{
	//TODO
}
