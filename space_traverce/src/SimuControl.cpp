#include "SimuControl.h"

#include "debug/Log.h"
#include "utils/Timer.h"
#include "utils/PhysConstants.h"

#include <algorithm>
#include <string>

SimuControl::SimuControl()
{
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
	bool result = m_mesh->FromFile(filepath);
	if (!result)
		return false;

	// build binary search tree
	m_mesh->BuildTopologyData();

	m_solver->SetMesh(m_mesh);

	return true;
}

bool SimuControl::Prepare()
{
	TIME_FUNCTION();
	LOG_TRACE("Initializing simulation parameters...");

	// assign material data
	Material air, glass;
	air.epsilon = 1;
	glass.epsilon = 2;
	m_solver->AddMaterial("Air", air);
	m_solver->AddMaterial("Metall", glass);

	// set boundary conditions
	if (!m_solver->SetBoundaryValue("Air Boundary", 0))
	{
		LOG_ERROR("surface not found");
		return false;
	}

	// initial charge value
	double charge = 1;
	double chargeDensity = charge / m_mesh->GetVolumeOfVolume("Metall");
	m_solver->SetInitialValue("Metall", chargeDensity);

	// assemble linear system
	m_solver->AssembleLeft();

	// beam setup
	Particle electron;
	electron.charge = phys::qe;
	electron.E0 = 0.511e6;		//eV

	m_beam->SetParticle(electron);
	m_beam->SetEnergy(10000); // eV
	m_beam->SetCenterPosition(0, 0, 0);
	m_beam->SetDirection(0,0,1);
	m_beam->SetRadius(0.1);

	m_beam->GeneratePrimaryPosition();

	// set beam stopping and field functions
	m_beam->SetStoppingFunction(std::bind(&SimuControl::BeamShouldStop, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	m_beam->SetFieldFunction(std::bind(&Solver::GetField2, m_solver,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

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
	TIME_FUNCTION();
	LOG_TRACE("Running simulation...");
	
	m_solver->AssembleRight();

	// solve systems
	if (!m_solver->Solve())
	{
		LOG_ERROR("Solving failed");
		return 2;
	}

	// save solution to file
	if (!m_solver->SaveSolutionToFile(m_matlab))
	{
		LOG_ERROR("failed to save solution to file");
		return 3;
	}

	// propagate particles in geometry
	m_beam->Run();
	
	if (!m_beam->SaveTracksToFile(m_matlab))
	{
		LOG_ERROR("failed to save tracks to file");
		return 4;
	}
	
	// closing matlab file
	// no more wrights after this
	m_matlab.Close();

	return 0;
}

bool SimuControl::BeamShouldStop(double x, double y, double z) const
{
	// out of bounds check
	if (!m_mesh->IsInBounds(x, y, z))
	{
		return false;
	}

	// get element index
	auto elemIndex = m_mesh->FindElementByCoords(x, y, z);
	// get its physical tags
	auto& tags = m_mesh->GetPhisicalTagsOfTetrahedron(elemIndex);

	bool contains = false;

	// check if should stop on this tags
	for (auto tag : tags)
	{
		if (std::find(m_stoppingVolumes.begin(), m_stoppingVolumes.end(), tag) != m_stoppingVolumes.end())
		{
			contains = true;
		}
	}
	
	return contains;
}
