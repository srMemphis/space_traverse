#pragma once
#include "Mesh.h"
#include "Material.h"

#include <map>
#include <memory>

class Solver
{
public:

	Solver(std::shared_ptr<Mesh> mesh);
	~Solver();










	void AddMaterial(const std::string& name, const Material& mat);
	void DoStuff();

private:

	std::shared_ptr<Mesh> m_mesh;
	std::map<std::string, Material> m_materials;


};
