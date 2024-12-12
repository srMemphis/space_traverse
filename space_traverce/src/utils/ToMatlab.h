#pragma once
#include <string>

#include <Eigen/Dense>

struct TinyMATWriterFile;

class ToMatlab
{
public:

	ToMatlab();
	~ToMatlab();

	bool OpenFile(std::string name);

	bool Write(Eigen::VectorXd vec, const std::string& name);

	bool Write(std::vector<double> vec, const std::string& name);

private:

	TinyMATWriterFile* m_file;
};
