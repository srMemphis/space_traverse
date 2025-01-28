#pragma once
#include <vector>
#include <Eigen/Dense>

struct DetectorData
{
	std::vector<Eigen::Vector3d> hitsPositions;
	std::vector<double> energy;
	double current;

	void Clear()
	{
		hitsPositions.clear();
		energy.clear();
		current = 0;
	}
};
