#pragma once
#include "mesh/Mesh.h"
#include "solver/Material.h"
#include "utils/UnsignedRange.h"
#include "utils/ToMatlab.h"
 	
#include <Eigen/SparseCore>

#include <map>
#include <memory>

class Solver
{
public:

	Solver();
	~Solver();
	
	void SetMesh(std::shared_ptr<Mesh> mesh);
	void AddMaterial(const std::string& name, const Material& mat);
	void AssignElementChargeDensity(size_t index, double value);
	bool SetInitialValue(const std::string& volumeName, double value);
	bool SetBoundaryValue(const std::string& name, double value);
	
	void AssembleLeft();
	void AssembleRight();
	bool Solve();

	void DoStuff();
	bool SaveSolutionToFile(const std::string& filepath);

	Eigen::Vector3d GetField(double x, double y, double z);

private:
	const double c_epsilon0 = 8.8541878128e-12;

	std::shared_ptr<Mesh> m_mesh;					// finite element mesh
	std::map<std::string, Material> m_materials;	// material data
	Eigen::VectorXd m_chargeDensity;				// charge density in each tetrahedron
	Eigen::VectorXd m_u;							// electric potential (solution)
	Eigen::SparseMatrix<double> m_Kii;				// part of K matrix created by non-boundary nodes
	Eigen::SparseMatrix<double> m_Kij;				// i - non-boundary node index, j - boundary node index
	Eigen::VectorXd m_fi;							// right hand side created by non-boundary nodes
	Eigen::VectorXd m_uBound;						// values of j-idexed nodes
	UnsignedRange m_iIndex;							// i - non - boundary node index
	UnsignedRange m_jIndex;							// j - boundary node index

	ToMatlab m_matlab;
};
