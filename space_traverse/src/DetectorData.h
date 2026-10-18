#pragma once
#include "utils/ToMatlab.h"

#include <Eigen/Dense>

#include <vector>

struct DetectorData
{
	std::vector<double> hitsPositionsX;
	std::vector<double> hitsPositionsY;
	std::vector<double> hitsPositionsZ;
	std::vector<double> energy;
	double current;

	void Clear()
	{
		hitsPositionsX.clear();
		hitsPositionsY.clear();
		hitsPositionsZ.clear();
		energy.clear();
		current = 0;
	}

	bool ToFile(ToMatlab& file) const
	{
		bool res = file.Write(hitsPositionsX, "hitsX") && file.Write(hitsPositionsY, "hitsY") &&
				   file.Write(hitsPositionsZ, "hitsZ") && file.WriteReal(current, "current") &&
				   file.Write(energy, "energy");

		return res;
	}
};
