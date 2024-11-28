#pragma once
#include <Eigen/Dense>

#include <vector>

#include "solver/Track.h"

struct Particle
{
	double m_charge;
	double m_mass;
};

class Beam
{
public:

	Beam();
	~Beam();

	// accessors



	//modifiers
	void SetParticle(Particle p);
	

private:

	std::vector<Eigen::Vector3d> m_startPos;	// starting positions of particles
	std::vector<Track> m_tracks;				// particle tracks

	Particle m_particle;
};
