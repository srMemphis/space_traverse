#pragma once
#include "Mesh.h"
#include <memory>

class Solver
{
public:

	Solver(std::shared_ptr<Mesh> mesh);
	~Solver();



	void DoStuff();




private:

	std::shared_ptr<Mesh> m_Mesh;
};
