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
	const std::vector<Track>& GetTracks() const;

	//modifiers
	void SetRadius(double rad);
	void SetCenterPosition(double x, double y, double z);
	void SetDirection(double x, double y, double z);
	void SetParticle(Particle p);
	void SetStoppingFunction(std::function<bool(double, double, double)> func);
	void SetFieldFunction(std::function<Eigen::Vector3d(double x, double y, double z)> func);
	void Reset();		// resets particle positions to starting. clears tracks
	void Run();

private:
	double m_radius;
	Eigen::Vector3d m_direction;
	Eigen::Vector3d m_centerPosition;

	std::vector<Eigen::Vector3d> m_startPos;	// starting positions of particles
	std::vector<Track> m_tracks;				// particle tracks
	std::function<bool(double, double, double)> m_stoppingFunction;		// returns true when the particle should stop
	std::function<Eigen::Vector3d(double x, double y, double z)> m_fieldFunction;	// returns electic field at the position
	Particle m_particle;
};
