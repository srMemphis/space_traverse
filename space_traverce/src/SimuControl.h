#pragma once
#include <memory>

#include "mesh/Mesh.h"
#include "solver/Solver.h"
#include "solver/Beam.h"

class SimuControl
{
public:
	SimuControl();
	~SimuControl();

	bool MeshFromFile(const std::string& filepath);
	bool Prepare();


	int Run();

private:
	std::shared_ptr<Mesh> m_mesh;
	std::shared_ptr<Solver> m_solver;
	std::shared_ptr<Beam> m_beam;
};
