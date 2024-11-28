#include "SimuControl.h"

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
	//mesh->BuildTopologyData();

	m_solver->SetMesh(m_mesh);

	return true;
}

bool SimuControl::Prepare()
{
	// assign material data
	Material air, glass;
	air.epsilon = 1;
	glass.epsilon = 2;
	m_solver->AddMaterial("Air", air);
	m_solver->AddMaterial("Metall", glass);

	// set boundary conditions
	if (!m_solver->SetBoundaryValue("Air Boundary", 0))
	{
		std::cout << "surface not found" << std::endl;
		return false;
	}

	// initial charge value
	double charge = 1;
	double chargeDensity = charge / m_mesh->GetVolumeOfVolume("Metall");
	m_solver->SetInitialValue("Metall", chargeDensity);

	// assemble linear system
	m_solver->AssembleLeft();
}

int SimuControl::Run()
{
	
	m_solver->AssembleRight();

	// solve systems
	if (!m_solver->Solve())
	{
		std::cout << "Solving failed" << std::endl;
		return 2;
	}

	// save solution to file
	if (!m_solver->SaveSolutionToFile("solution.txt"))
	{
		std::cout << "failed to save solution to file" << std::endl;
		return 3;
	}

	m_solver->DoStuff();

	return 0;
}
