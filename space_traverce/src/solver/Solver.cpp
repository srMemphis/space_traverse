#include "solver/Solver.h"

#include "utils/Timer.h"
#include "debug/Log.h"
#include "utils/Geometry.h"

#include <Eigen/Dense>
#include <Eigen/IterativeLinearSolvers>

#include <iostream>
#include <fstream>

Solver::Solver()
{
}

Solver::~Solver()
{
}

void Solver::SetMesh(std::shared_ptr<Mesh> mesh)
{
	m_mesh = mesh;

	size_t numNodes = m_mesh->GetNodes().size();
	size_t numTetra = m_mesh->GetBulkElements().size();

	// initial conditions
	m_chargeDensity.resize(numTetra);
	m_chargeDensity.setZero();

	// solution vector
	m_u.resize(numNodes);
	m_u.setZero();

	// init iner node indices
	m_iIndex.AddInterval(0, m_mesh->m_Nodes.size() - 1);
}

void Solver::AddMaterial(const std::string& name, const Material& mat)
{
	m_materials.emplace(name, mat);
}

void Solver::AssignElementChargeDensity(size_t index, double value)
{
	m_chargeDensity(index) = value;
}

bool Solver::SetInitialValue(const std::string& volumeName, double value)
{
	TIME_FUNCTION();

	bool isFound = false;

	int physTag = m_mesh->NameToTag(volumeName);

	for (auto& key_volume : m_mesh->m_volumes)
	{
		auto& volume = key_volume.second;

		if (volume.HasTag(physTag))
		{
			isFound = true;

			for (size_t i = volume.firstElementIndex; i < volume.firstElementIndex + volume.numElements; i++)
			{
				AssignElementChargeDensity(i, value);
			}
		}
	}

	return isFound;
}

// TODO: This will work only for one boundary. Fix if needed
bool Solver::SetBoundaryValue(const std::string& name, double value)
{
	TIME_FUNCTION();

	auto physTag = m_mesh->NameToTag(name);
	bool found = false;

	for (auto& key_surface : m_mesh->m_surfaces)
	{
		if (key_surface.second.HasTag(physTag))
		{
			found = true;

			// separate boundary and inner indices
			for (auto& element : key_surface.second.elements)
			{
				m_jIndex.Add(element.node[0]);
				m_jIndex.Add(element.node[1]);
				m_jIndex.Add(element.node[2]);

				m_iIndex.RemoveInterval(element.node[0], element.node[0]);
				m_iIndex.RemoveInterval(element.node[1], element.node[1]);
				m_iIndex.RemoveInterval(element.node[2], element.node[2]);
			}
		}
	}

	// boundary value vector resize
	m_uBound.conservativeResize(m_jIndex.Size());

	for (auto& key_surface : m_mesh->m_surfaces)
	{
		if (key_surface.second.HasTag(physTag))
		{
			// write boundary coefficients
			for (auto& triangle : key_surface.second.elements)
			{
				m_uBound.coeffRef(m_jIndex.GetPositionOf(triangle.node[0])) = value;
				m_uBound.coeffRef(m_jIndex.GetPositionOf(triangle.node[1])) = value;
				m_uBound.coeffRef(m_jIndex.GetPositionOf(triangle.node[2])) = value;

				m_u.coeffRef(triangle.node[0]) = value;
				m_u.coeffRef(triangle.node[1]) = value;
				m_u.coeffRef(triangle.node[2]) = value;
			}
		}
	}

	return found;
}

bool Solver::SaveSolutionToFile(ToMatlab& file)
{
	TIME_FUNCTION();

	bool res = false;

	int N = 1000;
	std::vector<double> field;
	Eigen::ArrayXd x = Eigen::ArrayXd::LinSpaced(N, -0.45, 0.45);

	for (int i = 0; i < N; i++)
	{
		field.push_back(GetField2(x[i], 0, 0).norm());
	}

	file.Write(field, "field");
	file.Write(x, "xField");

	if(file.Write(m_u, "uPlus") &&
		file.Write(m_mesh->GetNodesX(), "xPlus") &&
		file.Write(m_mesh->GetNodesY(), "yPlus") &&
		file.Write(m_mesh->GetNodesZ(), "zPlus"))
	{
		res = true;
	}

	return res;
}

Eigen::Vector3d Solver::GetField(double x, double y, double z)
{
	auto elemIndex = m_mesh->FindElementByCoords(x, y, z);
	auto& element = m_mesh->m_bulkElements.at(elemIndex);
	auto& gradient = m_mesh->m_GradientVectors.at(elemIndex);
	
	// find field from weighted gradients
	Eigen::Vector3d field = - gradient.at(0) * m_u(element.node[0])
							- gradient.at(1) * m_u(element.node[1])
							- gradient.at(2) * m_u(element.node[2])
							- gradient.at(3) * m_u(element.node[3]);

	return field;
}

Eigen::Vector3d Solver::GetField2(double x, double y, double z)
{
	auto elemIndex = m_mesh->FindElementByCoords(x, y, z);
	auto& element = m_mesh->m_bulkElements.at(elemIndex);
	auto& nodes = m_mesh->GetNodes();

	// find barycentric weights
	/*Eigen::Matrix4d left;
	left << 1, 1, 1, 1,
		nodes.at(element.node[0]).x, nodes.at(element.node[1]).x, nodes.at(element.node[2]).x, nodes.at(element.node[3]).x,
		nodes.at(element.node[0]).y, nodes.at(element.node[1]).y, nodes.at(element.node[2]).y, nodes.at(element.node[3]).y,
		nodes.at(element.node[0]).z, nodes.at(element.node[1]).z, nodes.at(element.node[2]).z, nodes.at(element.node[3]).z;

	Eigen::Vector4d right{ 1, x, y, z };

	Eigen::Vector4d bary = left.ldlt().solve(right).normalized();*/

	double elemVolume = m_mesh->GetVolumeOfTetrahedron(elemIndex);
	Eigen::Vector4d bary;
	bary(0) = Geometry::TetrahedronVolume(nodes.at(element.node[1]), nodes.at(element.node[3]), nodes.at(element.node[2]), { x,y,z }) / elemVolume;
	bary(1) = Geometry::TetrahedronVolume(nodes.at(element.node[0]), nodes.at(element.node[2]), nodes.at(element.node[3]), { x,y,z }) / elemVolume;
	bary(2) = Geometry::TetrahedronVolume(nodes.at(element.node[0]), nodes.at(element.node[3]), nodes.at(element.node[1]), { x,y,z }) / elemVolume;
	bary(3) = 1 - bary(0) - bary(1) - bary(2);

	// find field from weighted gradients
	Eigen::Vector3d field = bary(0) * m_field.at(element.node[0])
		+ bary(1) * m_field.at(element.node[1])
		+ bary(2) * m_field.at(element.node[2])
		+ bary(3) * m_field.at(element.node[3]);

	return field;
}

void Solver::AssembleLeft()
{
	TIME_FUNCTION();

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
		auto& tags = m_mesh->GetPhisicalTagsOfTetrahedron(tetraIndex);
		auto& name = m_mesh->TagToName(tags.at(0));
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
	TIME_FUNCTION();

	auto& nodes = m_mesh->GetNodes();

	// reserve space
	m_fi.resize(m_iIndex.Size());
	m_fi.setZero();

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
		Eigen::Vector4d flocal = Eigen::Vector4d(1, 1, 1, 1) * (jacob.determinant() / 6.0) * -m_chargeDensity(tetraIndex) / (4 * c_epsilon0);

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
	TIME_FUNCTION();

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

	LOG_TRACE("Iterations = {}", cg.iterations());

	// assemble full u
	RangeIterator iIndex(m_iIndex);
	
	for (size_t i = iIndex.Get(); i < iIndex.GetMax(); iIndex.Advance())
	{
		i = iIndex.Get();

		m_u(i) = uSolved(m_iIndex.GetPositionOf(i));
	}

	RangeIterator jIndex(m_jIndex);

	for (size_t j = jIndex.Get(); j < jIndex.GetMax(); jIndex.Advance())
	{
		j = jIndex.Get();

		m_u(j) = m_uBound(m_jIndex.GetPositionOf(j));
	}

	// -------------------- when using avaraged field -------------------------------
	// calc mean of field at nodes
	//
	m_field.resize(m_u.size());

	std::vector<Eigen::Vector3d> elemField;
	size_t numElem = m_mesh->GetBulkElements().size();
	elemField.resize(numElem);

	// first calc field at elements
	for (size_t elemIndex = 0; elemIndex < numElem; elemIndex++)
	{
		auto& gradient = m_mesh->m_GradientVectors.at(elemIndex);
		auto& element = m_mesh->m_bulkElements.at(elemIndex);

		elemField.at(elemIndex) = -gradient.at(0) * m_u(element.node[0])
			- gradient.at(1) * m_u(element.node[1])
			- gradient.at(2) * m_u(element.node[2])
			- gradient.at(3) * m_u(element.node[3]);
	}

	// then calc mean
	size_t numNodes = m_mesh->GetNodes().size();
	for (size_t nodeindex = 0; nodeindex < numNodes; nodeindex++)
	{
		auto& node = m_mesh->GetNode(nodeindex);
		auto& candidates = m_mesh->m_topologyTree.Find(node.x, node.y, node.z);

		m_field.at(nodeindex).setZero();
		int count = 0;
		for (auto& candidateIndex : candidates)
		{
			auto& cand = m_mesh->GetTetrahedron(candidateIndex);

			if (cand.node[0] == nodeindex || cand.node[1] == nodeindex 
				|| cand.node[2] == nodeindex || cand.node[3] == nodeindex)
			{
				count++;
				m_field.at(nodeindex) += elemField.at(candidateIndex);
			}
		}

		if (count == 0)
		{
			LOG_ERROR("SKIBIDI");
		}

		m_field.at(nodeindex) = m_field.at(nodeindex).array() / count;
	}

	return true;
}
