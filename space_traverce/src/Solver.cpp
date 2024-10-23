#include "Solver.h"
#include "utils/Timer.h"

#include <Eigen/Dense>

#include <iostream>


Solver::Solver(std::shared_ptr<Mesh> mesh)
	:m_mesh(mesh)
{
	size_t numNodes = m_mesh->GetNodes().size();
	size_t numTetra = m_mesh->GetBulkElements().size();
	
	m_chargeDensity.resize(numTetra);
	m_chargeDensity.setZero();

	m_u.resize(numNodes);
	m_u.setZero();

	m_uBoundary = Eigen::SparseVector<double>(numNodes);

	for (auto& key_volume : m_mesh->GetVolumes())
	{
		if (key_volume.second.numElements != 0)
			m_iIndex.AddInterval(key_volume.second.firstElementIndex, key_volume.second.firstElementIndex + key_volume.second.numElements-1);
	}

	for (auto& key_surface : m_mesh->GetSurfaces())
	{
		if (key_surface.second.numElements != 0)
		{
			m_jIndex.AddInterval(key_surface.second.firstElementIndex, key_surface.second.firstElementIndex + key_surface.second.numElements-1);
			m_iIndex.RemoveInterval(key_surface.second.firstElementIndex, key_surface.second.firstElementIndex + key_surface.second.numElements - 1);
		}
	}

}

Solver::~Solver()
{
}

void Solver::AddMaterial(const std::string& name, const Material& mat)
{
	m_materials.emplace(name, mat);
}

void Solver::AssignElementChargeDensity(size_t index, double value)
{
	m_chargeDensity(index) = value;
}

bool Solver::SetBoundaryValue(const std::string& name, double value)
{
	bool isFound = false;
	for (auto& key_surface : m_mesh->m_surfaces)
	{
		if (key_surface.second.phisicalName.name == name)
		{
			isFound = true;
			for (auto& triangle : key_surface.second.elements)
			{
				m_uBoundary.coeffRef(triangle.node[0]) = value;
			}
		}
	}
	return isFound;
}

void Solver::DoStuff()
{
	TIME_SCOPE();
	
	auto& surfElems = m_mesh->GetSurfElements();

	std::cout << "There are " << surfElems.size() << " triangles" << std::endl;

	auto& bulkElems = m_mesh->GetBulkElements();

	std::cout << "There are " << bulkElems.size() << " tetrahedrons" << std::endl;
	
	std::cout << "Max tetrahedrons at one node = " << m_mesh->MaxTetraAtNode() << std::endl;

}

void Solver::AssembleLeft()
{
	TIME_SCOPE();

	auto& nodes = m_mesh->GetNodes();
	size_t numNodes = nodes.size();

	// the chunky boy
	Eigen::SparseMatrix<double> K(numNodes, numNodes);
	auto elemPerCol = m_mesh->MaxTetraAtNode();
	K.reserve(Eigen::VectorXi::Constant(numNodes, elemPerCol));

	for (size_t tetraIndex = 0; tetraIndex < m_mesh->GetBulkElements().size(); tetraIndex++)
	{
		auto& tetra = m_mesh->GetBulkElements().at(tetraIndex);

		// assemble local matrix from tetrahedron coordinates
		Eigen::Matrix3d jacob(
		{
			{nodes.at(tetra.node[1]).x - nodes.at(tetra.node[0]).x, nodes.at(tetra.node[1]).y - nodes.at(tetra.node[0]).y, nodes.at(tetra.node[1]).z - nodes.at(tetra.node[0]).z},
			{nodes.at(tetra.node[2]).x - nodes.at(tetra.node[0]).x, nodes.at(tetra.node[2]).y - nodes.at(tetra.node[0]).y, nodes.at(tetra.node[2]).z - nodes.at(tetra.node[0]).z},
			{nodes.at(tetra.node[3]).x - nodes.at(tetra.node[0]).x, nodes.at(tetra.node[3]).y - nodes.at(tetra.node[0]).y, nodes.at(tetra.node[3]).z - nodes.at(tetra.node[0]).z}
		});

		Eigen::Matrix<double,3,4> dN;

		dN.col(0) = jacob.inverse() * Eigen::Vector3d(-1, -1, -1).matrix();
		dN.col(1) = jacob.inverse() * Eigen::Vector3d(1, 0, 0).matrix();
		dN.col(2) = jacob.inverse() * Eigen::Vector3d(0, 1, 0).matrix();
		dN.col(3) = jacob.inverse() * Eigen::Vector3d(0, 0, 1).matrix();

		Eigen::Matrix4d Klocal;

		for (int i = 0; i < 4; i++)
		{ 
			for (int j = 0; j < 4; j++)
			{
				Klocal(i, j) = (dN.col(i).array() * dN.col(j).array()).sum() * jacob.determinant() / -6.0;
			}
		}

		// multiply local matrix by epsilon of material
		auto name = m_mesh->GetPhisicalNameOfTetrahedron(tetraIndex);
		auto& material = m_materials.find(name)->second;

		Klocal *= material.epsilon;

		// write local to global matrix
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				// split matrix
				K.coeffRef(tetra.node[i], tetra.node[j]) += Klocal(i,j);
			}
		}

	} // element loop

	// TODO: this is not working. FIX
	// divide matrix into parts
	//m_Kii.reserve(Eigen::VectorXi::Constant(m_iIndex.Size(), elemPerCol));
	//m_Kij.reserve(Eigen::VectorXi::Constant(m_jIndex.Size(), elemPerCol));

	//RangeIterator iIter(m_iIndex);
	//RangeIterator iiIter(m_iIndex);
	//RangeIterator jIter(m_jIndex);

	//for (auto i = iIter.Get(); i < iIter.GetMax(); iIter.Advance())
	//{
	//	i = iIter.Get();
	//	for (auto j = jIter.Get(); j < jIter.GetMax(); jIter.Advance())
	//	{
	//		j = jIter.Get();
	//		m_Kij.coeffRef(i, j) = K.coeff(i, j);
	//	}

	//	for (auto ii = iiIter.Get(); ii < iiIter.GetMax(); iiIter.Advance())
	//	{
	//		m_Kii.coeffRef(i, ii) = K.coeff(i, ii);
	//	}
	//}

}

void Solver::AssembleRight()
{
	TIME_SCOPE();

	auto& nodes = m_mesh->GetNodes();
	size_t numNodes = nodes.size();

	// right hand side
	Eigen::VectorXd f(numNodes);

	for (size_t tetraIndex = 0; tetraIndex < m_mesh->GetBulkElements().size(); tetraIndex++)
	{
		auto& tetra = m_mesh->GetBulkElements().at(tetraIndex);

		Eigen::Matrix3d jacob(
			{
				{nodes.at(tetra.node[1]).x - nodes.at(tetra.node[0]).x, nodes.at(tetra.node[1]).y - nodes.at(tetra.node[0]).y, nodes.at(tetra.node[1]).z - nodes.at(tetra.node[0]).z},
				{nodes.at(tetra.node[2]).x - nodes.at(tetra.node[0]).x, nodes.at(tetra.node[2]).y - nodes.at(tetra.node[0]).y, nodes.at(tetra.node[2]).z - nodes.at(tetra.node[0]).z},
				{nodes.at(tetra.node[3]).x - nodes.at(tetra.node[0]).x, nodes.at(tetra.node[3]).y - nodes.at(tetra.node[0]).y, nodes.at(tetra.node[3]).z - nodes.at(tetra.node[0]).z}
			});

		// assemble right hand side
		Eigen::Vector4d flocal = Eigen::Vector4d(1, 1, 1, 1) * (jacob.determinant() / 6.0) / (4 * c_epsilon0) * -m_chargeDensity(tetraIndex);


		// TODO FIX
		// save only i-components
		/*for (int i = 0; i < 4; i++)
		{
			if (m_iIndex.IsInRange(tetra.node[i]))
			{
				m_fi(tetra.node[i]) += flocal(i);
			}
		}*/

	}

}
