#include "utils/ToMatlab.h"

#include <iostream>
#include <vector>

ToMatlab::ToMatlab()
{
}

ToMatlab::~ToMatlab()
{
	if (m_file.is_open())
	{
		m_file.close();
	}
}

bool ToMatlab::OpenFile(std::string name)
{
	if (m_file.is_open())
	{
		std::cout << "Unable to open file : " << name
			<< ". File is already opened" << std::endl;
		return false;
	}

	m_file.open(name, std::ios::out | std::ios::trunc);

	if (!m_file.is_open())
	{
		std::cout << "Unable to open file : " << name << std::endl;
		return false;
	}

	return true;
}

bool ToMatlab::Write(Eigen::VectorXd vec, const std::string& name)
{
	if (!m_file.is_open())
	{
		std::cout << "Unable to write vector " << name << " to file.File is not opened." << std::endl;
		return false;
	}

	m_file << '\n' << name << " =  [ ";

	for (size_t i = 0; i < vec.size(); i++)
	{
		m_file << vec(i) << ' ';
	}

	m_file << "];\n";

	return true;
}

bool ToMatlab::Write(std::vector<double> vec, const std::string& name)
{
	if (!m_file.is_open())
	{
		std::cout << "Unable to write vector " << name << " to file.File is not opened." << std::endl;
		return false;
	}

	m_file << '\n' << name << " =  [ ";

	for (size_t i = 0; i < vec.size(); i++)
	{
		m_file << vec.at(i) << ' ';
	}

	m_file << "];\n";

	return true;
}

bool ToMatlab::Write(const Eigen::SparseMatrix<double>& matrix, const std::string& name)
{
	
	if (!m_file.is_open())
	{
		std::cout << "Unable to write matrix " << name <<  " to file.File is not opened." << std::endl;
		return false;
	}

	Eigen::VectorXd rows, cols, value;
	
	size_t size = matrix.nonZeros();
	rows.resize(size);
	cols.resize(size);
	value.resize(size);

	size_t index = 0;
	for (size_t i = 0; i < matrix.outerSize(); i++)
	{
		for (Eigen::SparseMatrix<double>::InnerIterator it(matrix, i); it; ++it)
		{
			rows(index) = it.row() + 1;
			cols(index) = it.col() + 1;
			value(index) = it.value();

			index++;
		}
	}

	Write(rows, std::string(name + "_rows"));
	Write(cols, std::string(name + "_cols"));
	Write(value, std::string(name + "_value"));

	m_file << '\n' << name << " = sparse(" << name << "_rows, " << name << "_cols, " << name << "_value, "
		<< matrix.rows() << ", " << matrix.cols() << ");\n";

	return true;
}
