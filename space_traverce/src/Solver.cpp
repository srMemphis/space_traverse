#include "Solver.h"
#include <iostream>
#include "utils/Timer.h"

Solver::Solver(std::shared_ptr<Mesh> mesh)
	:m_mesh(mesh)
{
}

Solver::~Solver()
{
}

void Solver::AddMaterial(const std::string& name, const Material& mat)
{
	m_materials.emplace(name, mat);
}

void Solver::DoStuff()
{
	TIME_SCOPE();
	
	auto& surfElems = m_mesh->GetSurfElements();

	std::cout << "There are " << surfElems.size() << " triangles" << std::endl;

	auto& bulkElems = m_mesh->GetBulkElements();

	std::cout << "There are " << bulkElems.size() << " tetrahedrons" << std::endl;

	size_t maxSize = 0;

	// check if corner node are not replicated
	double d = 0.01;
	Box b(0.5 - d, 0.5 - d, 0.5 - d, 0.5 + d, 0.5 + d, 0.5 + d);
	int count = 0;
	for (auto& node : m_mesh->m_Nodes)
	{
		if (b.Contains(node.x, node.y, node.z))
			count++;
	}
	std::cout << "Count = " << count << std::endl;

}
