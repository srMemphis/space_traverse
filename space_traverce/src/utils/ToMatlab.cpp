#include "utils/ToMatlab.h"

#include "debug/Log.h"

#include "TinyMAT/tinymatwriter.h"

#include <vector>

ToMatlab::ToMatlab()
	:m_file(nullptr)
{
}

ToMatlab::~ToMatlab()
{
	if (m_file)
	{
		TinyMATWriter_close(m_file);
	}
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

bool ToMatlab::Write(Eigen::VectorXd vec, const std::string& name)
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

bool ToMatlab::Write(std::vector<double> vec, const std::string& name)
{
	if (!m_file)
	{
		LOG_ERROR("Unable to write vector {} to file.File is not opened.", name);

		return false;
	}

	TinyMATWriter_writeContainerAsColumn(m_file, name.c_str(), vec);

	return true;
}
