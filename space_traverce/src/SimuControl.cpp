#include "SimuControl.h"

#include "debug/Log.h"

#include <algorithm>
#include <string>

SimuControl::SimuControl()
{
	m_mesh = std::make_shared<Mesh>();
	m_solver = std::make_shared<Solver>();
	m_beam = std::make_shared <Beam> ();
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
	// set beam stopping function
	m_beam->SetStoppingFunction(std::bind(&SimuControl::BeamShouldStop, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

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
	
	m_solver->AssembleRight();

	// solve systems
	if (!m_solver->Solve())
	{
		LOG_ERROR("Solving failed");
		return 2;
	}

	// save solution to file
	if (!m_solver->SaveSolutionToFile("solution.mat"))
	{
		LOG_ERROR("failed to save solution to file");
		return 3;
	}

	return 0;
}

bool SimuControl::BeamShouldStop(double x, double y, double z) const
{
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
