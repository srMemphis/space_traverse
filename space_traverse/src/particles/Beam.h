#pragma once
#include "particles/Track.h"
#include "utils/ToMatlab.h"

#include <Eigen/Dense>

#include <functional>
#include <vector>

struct Particle
{
	double charge = 0; // C
	double E0 = 0;     // eV
};

class Beam
{
public:
	Beam();
	~Beam();

	// accessors
	const std::vector<Track>& GetTracks() const;

	// modifiers

	// set this before generating
	void SetRadius(double rad);
	void SetCenterPosition(double x, double y, double z);
	void SetDirection(double x, double y, double z);
	void SetParticle(Particle p);
	void SetEnergy(double ene);
	void DetermineStepSize(double environmentSize, uint32_t desiredStepCount);
	void SetDivergence(double degrees);

	// generates initial positions based on parameters set above
	void GeneratePrimaryPosition(uint32_t param);

	// set functions to determine particle behavior in space
	void SetStoppingFunction(std::function<bool(double, double, double)> func);
	void SetFieldFunction(std::function<Eigen::Vector3d(double x, double y, double z)> func);

	// run vasya run
	void Run();

	// clears tracks exept their first point
	void Reset();

	// matlab file output
	bool SaveTracksToFile(ToMatlab& file, uint32_t numPoints = 200);
	bool SaveTracksToFilePresice(ToMatlab& file);

private:
	void GenerateUniformDisk(uint32_t circles);

private:
	double m_divergence;
	double m_radius;
	double m_energy;
	double m_stepSize;
	Eigen::Vector3d m_direction;
	Eigen::Vector3d m_centerPosition;

	std::vector<Eigen::Vector3d> m_startPos;                        // starting positions of particles
	std::vector<Track> m_tracks;                                    // particle tracks
	std::function<bool(double, double, double)> m_stoppingFunction; // returns true when the particle should stop
	std::function<Eigen::Vector3d(double x, double y, double z)>
		m_fieldFunction; // returns electic field at the position
	Particle m_particle;
};
