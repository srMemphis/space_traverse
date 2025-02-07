#include "SimuControl.h"

#include "debug/Log.h"
#include "debug/Profiler.h"
#include "utils/PhysConstants.h"

#include <algorithm>
#include <string>

SimuControl::SimuControl()
{
	PROFILE_FUNCTION();

	m_mesh = std::make_shared<Mesh>();
	m_solver = std::make_shared<Solver>();
	m_beam = std::make_shared <Beam> ();

	m_matlab.OpenFile("solution.mat");
}

SimuControl::~SimuControl()
{
}

bool SimuControl::MeshFromFile(const std::string& filepath)
{
	PROFILE_FUNCTION();
	LOG_TRACE("Reading mesh...");

	bool result = m_mesh->FromFile(filepath);
	if (!result)
		return false;

	// build binary search tree
	LOG_TRACE("Building topology data...");
	m_mesh->BuildTopologyData();

	m_solver->SetMesh(m_mesh);

	m_detectorTag = m_mesh->NameToTag("Detector");

	// saving geometry to dispaly at matlab
	ToMatlab matfile;
	matfile.OpenFile("surfacegeo.mat");

	m_mesh->SaveSurfaceToFile("Plane Boundary", matfile);

	matfile.Close();

	return true;
}

bool SimuControl::Prepare()
{
	PROFILE_FUNCTION();
	LOG_TRACE("Initializing simulation parameters...");

	// assign material data
	Material air, glass;
	air.epsilon = 1;
	glass.epsilon = 2;
	m_solver->AddMaterial("World", air);
	m_solver->AddMaterial("Detector", air);
	m_solver->AddMaterial("Plane", glass);

	// set boundary conditions
	if (!m_solver->SetBoundaryValue("World Boundary", 0))
	{
		LOG_ERROR("boundary surface not found");
		return false;
	}

	// initial charge value
	//double charge = -1e-7;
	//double chargeDensity = charge / m_mesh->GetVolumeOfVolume("Metall");
	//m_solver->SetInitialValue("Metall", chargeDensity);

	// assemble linear system
	m_solver->AssembleLeft();

	// beam setup
	Particle electron;
	electron.charge = phys::qe;
	electron.E0 = 0.511e6;		//eV

	m_beam->SetParticle(electron);
	m_beam->SetEnergy(1e4); // eV

	Eigen::Vector3d beamPos = { 0, 0.05, 0.15 };
	Eigen::Vector3d beamStare = { 0, 0, -0.02 };
	auto dir = (beamStare - beamPos).normalized();

	m_beam->SetCenterPosition(beamPos.x(), beamPos.y(), beamPos.z());
	m_beam->SetDirection(dir.x(), dir.y(), dir.z());
	m_beam->SetRadius(0.01);
	m_beam->DetermineStepSize(0.5, 30000);

	m_beam->GeneratePrimaryPosition(6);		// n^2 growth of number of particles

	// all volumes exept Detector
	AddStoppingVolume("Plane");

	// set beam stopping and field functions
	m_beam->SetStoppingFunction(std::bind(&SimuControl::BeamShouldStop, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	m_beam->SetFieldFunction(std::bind(&Solver::GetField2, m_solver,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	// beam current and delta time
	m_deltaTime = 1e-9;		// seconds
	m_current = 100e-9;		// ampere

	return true;
}

bool SimuControl::AddStoppingVolume(const std::string& name)
{	
	auto physTag = m_mesh->NameToTag(name);

	m_stoppingVolumes.push_back(physTag);
	
	return true;
}

int SimuControl::Run()
{
	PROFILE_FUNCTION();
	LOG_TRACE("Running simulation...");
	
	LOG_TRACE("Assembling ...");
	m_solver->AssembleRight();

	// solve systems
	LOG_TRACE("Solving...");
	if (!m_solver->Solve())
	{
		LOG_ERROR("Solving failed");
		return 2;
	}

	// propagate particles in geometry
	// multithreaded inside
	LOG_TRACE("Propagating Tracks");
	m_beam->Run();

	// check where we hit and act accordingly
	RegisterHits();

	// save solution to file
	LOG_TRACE("Saving solution to file...");
	if (!m_solver->SaveSolutionToFile(m_matlab))
	{
		LOG_ERROR("failed to save solution to file");
		return 3;
	}

	if (!m_beam->SaveTracksToFile(m_matlab))
	{
		LOG_ERROR("failed to save tracks to file");
		return 4;
	}

	// TODO: save detector data to file 
	
	// closing matlab file
	// no more wrights after this
	m_matlab.Close();

	return 0;
}

// called in multithread
bool SimuControl::BeamShouldStop(double x, double y, double z) const
{
	// TODO: Consider narrow boundary checking to avoid infinite warnings
	// get element index
	auto elemIndex = m_mesh->FindElementByCoords(x, y, z);

	if (elemIndex < 0 )
	{
		// we are outside
		return true;
	}

	// get its physical tags
	auto tag = m_mesh->GetPhisicalTagOfTetrahedron(elemIndex);

	// check if should stop on this tags
	if (std::find(m_stoppingVolumes.begin(), m_stoppingVolumes.end(), tag) != m_stoppingVolumes.end())
	{
		return true;
	}
	
	return false;
}

void SimuControl::RegisterHits()
{
	m_detector.Clear();
	double charge = m_current * m_deltaTime / m_beam->GetTracks().size();	// charge per track
	for (auto& track : m_beam->GetTracks())
	{
		auto& lastPoint = track.GetLastPoint();
		auto elemIndex = m_mesh->FindElementByCoords(lastPoint.x(), lastPoint.y(), lastPoint.z());

		auto tag = m_mesh->GetPhisicalTagOfTetrahedron(elemIndex);

		// we hit detector
		if (tag == m_detectorTag)
		{
			// TODO: calc particle energy
			m_detector.hitsPositions.push_back(lastPoint);
			m_detector.current += (charge / m_deltaTime);
			m_detector.energy.push_back(0);
		}
		else if (std::find(m_stoppingVolumes.begin(), m_stoppingVolumes.end(), tag) != m_stoppingVolumes.end())
		{
			// TODO: generate secondary emission

			// we hit smth that needs to be charged
			m_solver->AddElementChargeDensity(elemIndex, charge);
		}
	}
}
