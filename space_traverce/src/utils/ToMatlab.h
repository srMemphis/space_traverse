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

	bool Write(const Eigen::VectorXd& vec, const std::string& name);

	bool Write(const std::vector<double>& vec, const std::string& name);

	void StartCell(const std::string& name, uint32_t size);
	void EndCell();

	bool WriteCell(const std::vector<double>& vec);
	bool WriteCellAsMatrix(const std::vector<double>& vec, uint32_t rows, uint32_t cols);



	void Close();

private:

	TinyMATWriterFile* m_file;
};
