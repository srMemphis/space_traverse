#include <iostream>
#include <fstream>
#include <string>

#include "mesh/Mesh.h"
#include "solver/Solver.h"

#include <vector>
#include <random>

int main()
{
	//std::string filepath = "msh/sphere_coarse.msh";
	std::string filepath = "msh/sphere.msh";
	
	std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
	bool result = mesh->FromFile(filepath);
	if (!result)
		return -1;
	
	// build binary search tree
	//mesh->BuildTopologyData();

	// assign material data
	Solver slv(mesh);
	Material air, glass;
	air.epsilon = 1;
	glass.epsilon = 2;
	slv.AddMaterial("Air", air);
	slv.AddMaterial("Metall", glass);

	// set boundary conditions
	if (!slv.SetBoundaryValue("Air Boundary", 0))
	{
		std::cout << "surface not found" << std::endl;
		return 1;
	}

	// initial charge value
	double charge = 1;
	double chargeDensity = charge / mesh->GetVolumeOfVolume("Metall");
	slv.SetInitialValue("Metall", chargeDensity);

	// assemble linear system
	slv.AssembleLeft();
	slv.AssembleRight();

	// solve systems
	if (!slv.Solve())
	{
		std::cout << "Solving failed" << std::endl;
		return 2;
	}

	// save solution to file
	if (!slv.SaveSolutionToFile("solution.txt"))
	{
		std::cout << "failed to save solution to file" << std::endl;
		return 3;
	}

	slv.DoStuff();

	return 0;
}
