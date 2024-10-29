#include "Solver.h"
#include "utils/Timer.h"

#include <Eigen/Dense>
#include<Eigen/IterativeLinearSolvers>

#include <iostream>
#include <fstream>


Solver::Solver(std::shared_ptr<Mesh> mesh)
	:m_mesh(mesh)
{
	TIME_SCOPE();

	size_t numNodes = m_mesh->GetNodes().size();
	size_t numTetra = m_mesh->GetBulkElements().size();
	
	// initial conditions
	m_chargeDensity.resize(numTetra);
	m_chargeDensity.setZero();

	// solution vector
	m_u.resize(numNodes);
	m_u.setZero();

	// separate boundary and volume node indices
	for (auto& key_volume : m_mesh->GetVolumes())
	{
		if (key_volume.second.numElements != 0)
		{
			// for every tetrahedron add its nodes to index array
			for (size_t i = key_volume.second.firstElementIndex; i < key_volume.second.firstElementIndex + key_volume.second.numElements; i++)
			{
				m_iIndex.Add(m_mesh->m_bulkElements.at(i).node[0]);
				m_iIndex.Add(m_mesh->m_bulkElements.at(i).node[1]);
				m_iIndex.Add(m_mesh->m_bulkElements.at(i).node[2]);
				m_iIndex.Add(m_mesh->m_bulkElements.at(i).node[3]);
			}
		}
	}

	for (auto& key_surface : m_mesh->GetSurfaces())
	{
		if (key_surface.second.numElements != 0)
		{
			for (size_t i = key_surface.second.firstElementIndex; i < key_surface.second.firstElementIndex + key_surface.second.numElements; i++)
			{
				m_jIndex.Add(m_mesh->m_surfElements.at(i).node[0]);
				m_jIndex.Add(m_mesh->m_surfElements.at(i).node[1]);
				m_jIndex.Add(m_mesh->m_surfElements.at(i).node[2]);

				m_iIndex.RemoveInterval(m_mesh->m_surfElements.at(i).node[0], m_mesh->m_surfElements.at(i).node[0]);
				m_iIndex.RemoveInterval(m_mesh->m_surfElements.at(i).node[1], m_mesh->m_surfElements.at(i).node[1]);
				m_iIndex.RemoveInterval(m_mesh->m_surfElements.at(i).node[2], m_mesh->m_surfElements.at(i).node[2]);
			}
		}
	}

	// boundary value vector
	m_uBound.resize(m_jIndex.Size());
	m_uBound.setZero();

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
				m_uBound.coeffRef(m_jIndex.GetPositionOf(triangle.node[0])) = value;
				m_u.coeffRef(triangle.node[0]) = value;
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

bool Solver::SaveSolutionToFile(std::string filepath)
{
	std::ofstream myfile;
	myfile.open(filepath, std::ios::out | std::ios::trunc);

	if (!myfile.is_open())
	{
		return false;
	}

	for (size_t i = 0; i < m_u.size(); i++)
	{
		myfile << m_u(i) << ' ';
	}

	myfile.close();

	return true;
}

void Solver::AssembleLeft()
{
	TIME_SCOPE();

	auto& nodes = m_mesh->GetNodes();
	size_t numNodes = nodes.size();

	// the chunky boys
	auto elemPerCol = m_mesh->MaxTetraAtNode();
	m_Kii.resize(m_iIndex.Size(), m_iIndex.Size());
	m_Kij.resize(m_iIndex.Size(), m_jIndex.Size());

	m_Kii.reserve(Eigen::VectorXi::Constant(m_iIndex.Size(), elemPerCol)); // reserve elements for i columns
	m_Kij.reserve(Eigen::VectorXi::Constant(m_jIndex.Size(), elemPerCol)); // reserve elements for j columns

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
				if (m_iIndex.Contains(tetra.node[i]))
				{	
					// ii
					if (m_iIndex.Contains(tetra.node[j]))
					{
						m_Kii.coeffRef(m_iIndex.GetPositionOf(tetra.node[i]), m_iIndex.GetPositionOf(tetra.node[j])) += Klocal(i, j);
					}
					// ij
					else
					{
						m_Kij.coeffRef(m_iIndex.GetPositionOf(tetra.node[i]), m_jIndex.GetPositionOf(tetra.node[j])) += Klocal(i, j);
					}
				}
			}
		}

	} // element loop

	//finalize
	m_Kii.finalize();
	m_Kij.finalize();
}

void Solver::AssembleRight()
{
	TIME_SCOPE();

	auto& nodes = m_mesh->GetNodes();

	// reserve space
	m_fi.resize(m_iIndex.Size());

	for (size_t tetraIndex = 0; tetraIndex < m_mesh->GetBulkElements().size(); tetraIndex++)
	{
		auto& tetra = m_mesh->GetBulkElements().at(tetraIndex);

		Eigen::Matrix3d jacob(
			{
				{nodes.at(tetra.node[1]).x - nodes.at(tetra.node[0]).x, nodes.at(tetra.node[1]).y - nodes.at(tetra.node[0]).y, nodes.at(tetra.node[1]).z - nodes.at(tetra.node[0]).z},
				{nodes.at(tetra.node[2]).x - nodes.at(tetra.node[0]).x, nodes.at(tetra.node[2]).y - nodes.at(tetra.node[0]).y, nodes.at(tetra.node[2]).z - nodes.at(tetra.node[0]).z},
				{nodes.at(tetra.node[3]).x - nodes.at(tetra.node[0]).x, nodes.at(tetra.node[3]).y - nodes.at(tetra.node[0]).y, nodes.at(tetra.node[3]).z - nodes.at(tetra.node[0]).z}
			});

		// assemble local f vector
		Eigen::Vector4d flocal = Eigen::Vector4d(1, 1, 1, 1) * (jacob.determinant() / 6.0) / (4 * c_epsilon0) * -m_chargeDensity(tetraIndex);

		// save only i-components to global f vector
		for (int i = 0; i < 4; i++)
		{
			if (m_iIndex.Contains(tetra.node[i]))
			{
				m_fi(m_iIndex.GetPositionOf(tetra.node[i])) += flocal(i);
			}
		}

	}

}

bool Solver::Solve()
{
	TIME_SCOPE();

	// iterative solver
	Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper> cg;

	// register right side
	cg.compute(m_Kii);

	// solve for left side
	Eigen::VectorXd uSolved = cg.solve(m_fi - m_Kij * m_uBound);

	if (cg.info() != Eigen::ComputationInfo::Success)
	{
		return false;
	}

	std::cout << "Iterations = " << cg.iterations() << std::endl;

	// assemble full u
	RangeIterator iIndex(m_iIndex);
	
	for (size_t i = iIndex.Get(); i < iIndex.GetMax(); iIndex.Advance())
	{
		i = iIndex.Get();
		m_u(i) = uSolved(m_iIndex.GetPositionOf(i));
	}

	return true;
}
