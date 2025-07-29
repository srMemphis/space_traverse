#pragma once
#include <memory>
#include <string_view>

#include "mesh/Mesh.h"
#include "solver/Solver.h"
#include "particles/Beam.h"
#include "DetectorData.h"

struct SimParameters
{
	// general
	double deltaTime = 0;
	size_t iterations = 0;
	size_t saveInterval = 0;
	std::string outputDir = "out";
	
	// beam
	double current = 0;
	double energy = 0;
	Eigen::Vector3d pos = {0,0,0};
	Eigen::Vector3d stare = { 0,0,0 };
	double radius = 0;
	double divergence = 0;
	uint32_t numsteps = 0;
	uint32_t numtracks = 0;

	// mesh
	std::string meshfile;

	std::string targetName;
	std::string targetMaterialName;
	std::string targetBoundary;

	std::string worldName;
	std::string worldMaterialName;
	std::string worldBoundary;

	std::string maskName;
	std::string maskMaterialName;
	std::string maskBoundary;

	// solver
	Material targetMaterial;
	Material worldMaterial;
	Material maskMaterial;

	bool smooth = false;
};

class SimuControl
{
public:
	SimuControl();
	~SimuControl();

	bool ConfigFromFile(std::string_view configfile);

	bool Prepare();

	int Run();

private:
	bool AddStoppingVolume(const std::string& name);
	bool MeshFromFile(const std::string& filepath);
	bool BeamShouldStop(double x, double y, double z) const;
	void RegisterHits();

	std::shared_ptr<Mesh> m_mesh;
	std::shared_ptr<Solver> m_solver;
	std::shared_ptr<Beam> m_beam;

	std::vector<int> m_stoppingVolumes;		// tags of phys names electrons should stop in
	int m_detectorTag = 0;

	// simu params
	SimParameters m_params;

	// serialization
	bool GenerateSurfaceToVolumeIndexing(const std::string& physSurface, const std::string& physVolume);
	bool SaveChargeData();
	std::map<std::string, std::vector<size_t>> m_chargeIndexToSave;	// maped by phys surface name

	// per iteration data 
	DetectorData m_detector;

	ToMatlab m_matlab;			// matlab file IO
};
