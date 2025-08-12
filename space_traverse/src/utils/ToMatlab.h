#pragma once
#include <string>
#include <string_view>
#include <filesystem>

#include <Eigen/Dense>
#include "debug/Log.h"
#include "TinyMAT/tinymatwriter.h"

struct TinyMATWriterFile;

class ToMatlab
{
public:

	ToMatlab();
	~ToMatlab();

	bool OpenFile(std::string name);
	bool OpenFile(std::filesystem::path name);

	bool Write(const Eigen::VectorXd& vec, const std::string& name);

	template<typename T>
	bool Write(const std::vector<T>& vec, const std::string& name);

	template<typename T>
	bool WriteAsMatrix(const std::vector<T>& vec, uint32_t rows, uint32_t cols, const std::string& name);

	void StartCell(const std::string& name, uint32_t size);
	void EndCell();
	bool WriteCell(const std::vector<double>& vec);
	bool WriteCellAsMatrix(const std::vector<double>& vec, uint32_t rows, uint32_t cols);

	template<typename T>
	bool WriteReal(T val, std::string_view name);

	void Close();

private:

	TinyMATWriterFile* m_file;
};

template<typename T>
bool ToMatlab::WriteReal(T val, std::string_view name)
{
	if (!m_file)
	{
		LOG_ERROR("Unable to write value {} to file.File is not opened.", name);
		return false;
	}

	TinyMATWriter_writeValue(m_file, name.data(), val);

	return true;
}

template<typename T>
inline bool ToMatlab::Write(const std::vector<T>& vec, const std::string& name)
{
	if (!m_file)
	{
		LOG_ERROR("Unable to write vector {} to file.File is not opened.", name);
		return false;
	}

	TinyMATWriter_writeContainerAsColumn(m_file, name.c_str(), vec);

	return true;
}

template<typename T>
inline bool ToMatlab::WriteAsMatrix(const std::vector<T>& vec, uint32_t rows, uint32_t cols, const std::string& name)
{
	if (!m_file)
	{
		LOG_ERROR("Unable to write matrix {} to file.File is not opened.", name);
		return false;
	}

	TinyMATWriter_writeMatrix2D_colmajor(m_file, name.c_str(), vec.data(), cols, rows);

	return true;
}
