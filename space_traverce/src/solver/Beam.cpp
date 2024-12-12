#include "Beam.h"

#include "debug/Log.h"

Beam::Beam()
{
}

Beam::~Beam()
{
}

void Beam::SetParticle(Particle p)
{
}

void Beam::SetStoppingFunction(std::function<bool(double, double, double)> func)
{
	m_stoppingFunction = func;
}
