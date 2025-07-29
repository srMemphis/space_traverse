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
	void SetElementChargeDensity(size_t index, double value);
	void AddElementCharge(size_t index, double value);
	bool SetInitialValue(const std::string& volumeName, double value);
	bool SetBoundaryValue(const std::string& name, double value);
	bool SetBoundaryValues(const std::vector<std::string>& names, const std::vector<double> values);
	void SetFieldSmoothing(bool enable);

	void AssembleLeft();
	void AssembleRight();
	bool Solve();

	bool SaveSolutionToFile(ToMatlab& file);

	Eigen::Vector3d GetField(double x, double y, double z);
	double GetChargeDensAtElem(size_t elemIndex);

private:
	Eigen::Vector3d GetFieldCasual(double x, double y, double z);
	Eigen::Vector3d GetFieldSmooth(double x, double y, double z);

private:
	const double c_epsilon0 = 8.8541878128e-12;
	bool m_fieldSmoothing = 0;;

	std::shared_ptr<Mesh> m_mesh;					// finite element mesh
	std::map<std::string, Material> m_materials;	// material data
	Eigen::VectorXd m_chargeDensity;				// charge density in each tetrahedron
	Eigen::VectorXd m_u;							// electric potential (solution)
	Eigen::VectorXd m_uGuess;						// eigen iterative solution used as guess
	Eigen::SparseMatrix<double> m_Kii;				// part of K matrix created by non-boundary nodes
	Eigen::SparseMatrix<double> m_Kij;				// i - non-boundary node index, j - boundary node index
	Eigen::VectorXd m_fi;							// right hand side created by non-boundary nodes
	Eigen::VectorXd m_uBound;						// values of j-idexed nodes
	UnsignedRange m_iIndex;							// i - non - boundary node index
	UnsignedRange m_jIndex;							// j - boundary node index

	//std::vector<double> m_elemVolume;				// volumes of elements to calc charge density
	std::vector<Eigen::Vector3d> m_field;			// mean of electric field at nodes
};
