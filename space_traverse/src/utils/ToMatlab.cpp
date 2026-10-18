#include "utils/ToMatlab.h"

#include "debug/Log.h"

#include "TinyMAT/tinymatwriter.h"

#include <vector>

ToMatlab::ToMatlab() : m_file(nullptr)
{
}

ToMatlab::~ToMatlab()
{
	Close();
}

bool ToMatlab::OpenFile(std::string name)
{
	if (m_file)
	{
		LOG_ERROR("Unable to open file :{}. File is already opened", name);

		return false;
	}

	m_file = TinyMATWriter_open(name.c_str());

	if (!m_file)
	{
		LOG_ERROR("Unable to open file :{}.", name);

		return false;
	}

	return true;
}

bool ToMatlab::OpenFile(std::filesystem::path name)
{
	return OpenFile(name.string());
}

bool ToMatlab::Write(const Eigen::VectorXd& vec, const std::string& name)
{
	if (!m_file)
	{
		LOG_ERROR("Unable to write vector {} to file.File is not opened.", name);

		return false;
	}

	std::vector<double> v(&vec[0], vec.data() + vec.cols() * vec.rows());

	if (vec.IsRowMajor())
	{
		TinyMATWriter_writeContainerAsRow(m_file, name.c_str(), v);
	}
	else
	{
		TinyMATWriter_writeContainerAsColumn(m_file, name.c_str(), v);
	}

	return true;
}

void ToMatlab::StartCell(const std::string& name, uint32_t size)
{
	TinyMATWriter_startCellVectorAsCol(m_file, name.c_str(), size);
}

void ToMatlab::EndCell()
{
	TinyMATWriter_endCellArray(m_file);
}

bool ToMatlab::WriteCell(const std::vector<double>& vec)
{
	if (!m_file)
	{
		LOG_ERROR("Unable to write cell to file.File is not opened.");

		return false;
	}

	TinyMATWriter_writeContainerAsColumn(m_file, "", vec);

	return true;
}

bool ToMatlab::WriteCellAsMatrix(const std::vector<double>& vec, uint32_t rows, uint32_t cols)
{
	if (!m_file)
	{
		LOG_ERROR("Unable to write cell to file.File is not opened.");

		return false;
	}

	TinyMATWriter_writeMatrix2D_colmajor(m_file, "", vec.data(), cols, rows);

	return true;
}

void ToMatlab::Close()
{
	if (m_file)
	{
		TinyMATWriter_close(m_file);
		m_file = nullptr;
	}
}
