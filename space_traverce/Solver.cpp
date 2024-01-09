#include "Solver.h"
#include <iostream>

Solver::Solver(std::shared_ptr<Mesh> mesh)
	:m_Mesh(mesh)
{
}

Solver::~Solver()
{
}

void Solver::DoStuff()
{

	auto& surfElems = m_Mesh->GetSurfElements();

	std::cout << "There are " << surfElems.size() << " triangles" << std::endl;

	auto& bulkElems = m_Mesh->GetBulkElements();

	std::cout << "There are " << bulkElems.size() << " tetrahedrons" << std::endl;


	size_t maxSize = 0;

	for (auto node : m_Mesh->m_topologyTree)
	{
		maxSize = maxSize < node.elements.size() ? node.elements.size() : maxSize;
	}

	std::cout << " Max size = " << maxSize << std::endl;

	double d = 0.01;
	Box b(0.5 - d, 0.5 - d, 0.5 - d, 0.5 + d, 0.5 + d, 0.5 + d);
	int count = 0;
	for (auto& node : m_Mesh->m_Nodes)
	{
		if (b.Contains(node.x, node.y, node.z))
			count++;
	}
	std::cout << "Count = " << count << std::endl;

}
