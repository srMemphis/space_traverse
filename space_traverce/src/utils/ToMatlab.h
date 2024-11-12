#pragma once
#include <fstream>
#include <string>
#include <Eigen/Dense>
#include <Eigen/SparseCore>

class ToMatlab
{
public:

	ToMatlab();
	~ToMatlab();


	bool OpenFile(std::string name);

	bool Write(Eigen::VectorXd vec, const std::string& name);

	bool Write(std::vector<double> vec, const std::string& name);

	bool Write(const Eigen::SparseMatrix<double>& matrix, const std::string& name);

private:

	std::ofstream m_file;

};
