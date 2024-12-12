#pragma once
#include <Eigen/Dense>

#include <vector>
#include <functional>

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
	void SetStoppingFunction(std::function<bool(double, double, double)> func);

private:

	std::vector<Eigen::Vector3d> m_startPos;	// starting positions of particles
	std::vector<Track> m_tracks;				// particle tracks
	std::function<bool(double, double, double)> m_stoppingFunction;		// returns true when the particle should stop

	Particle m_particle;
};
