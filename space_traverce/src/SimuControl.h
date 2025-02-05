#pragma once
#include <memory>

#include "mesh/Mesh.h"
#include "solver/Solver.h"
#include "particles/Beam.h"
#include "DetectorData.h"

class SimuControl
{
public:
	SimuControl();
	~SimuControl();

	bool MeshFromFile(const std::string& filepath);
	bool AddStoppingVolume(const std::string& name);

	bool Prepare();

	int Run();

private:
	bool BeamShouldStop(double x, double y, double z) const;
	void RegisterHits();

	std::shared_ptr<Mesh> m_mesh;
	std::shared_ptr<Solver> m_solver;
	std::shared_ptr<Beam> m_beam;

	std::vector<int> m_stoppingVolumes;		// tags of phys names electrons should stop in
	int m_detectorTag = 0;

	// simu params
	double m_deltaTime = 0;
	double m_current = 0;

	// per iteration data 
	DetectorData m_detector;

	ToMatlab m_matlab;			// matlab file IO
};
