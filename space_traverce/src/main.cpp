#include <iostream>
#include <fstream>
#include <string>

#include "Mesh.h"
#include "Solver.h"
#include "GJK.h"

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

	//Box box(-0.21875, 0.09375, -0.03125, -0.1875, 0.125, 0);

	//ETetrahedron tet {0,1,2,3};
	//std::vector<Node> tetCoords;

	//tetCoords.push_back({ -0.234116, 0.0995518, -0.00945692 });
	//tetCoords.push_back({ -0.235809, 0.0802185, 0.0079708 });
	//tetCoords.push_back({ -0.241861, 0.0748158, -0.0261597 });
	//tetCoords.push_back({ -0.21729, 0.0804512, -0.0116034 });


	//bool result = GJK::Check(box, tet, tetCoords);

	//std::cout << result << std::endl;
	return 0;
}
