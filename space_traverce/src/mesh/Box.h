#pragma once
#include <iostream>

class Box
{
public:

	Box(double xmin, double ymin, double zmin, double xmax, double ymax, double zmax)
		: xMin(xmin), yMin(ymin), zMin(zmin), xMax(xmax), yMax(ymax), zMax(zmax) {}

	Box()
		: xMin(0), yMin(0), zMin(0), xMax(0), yMax(0), zMax(0) {}

	double xMin;
	double yMin;
	double zMin;

	double xMax;
	double yMax;
	double zMax;

	bool Contains(double x, double y, double z) const
	{
		return x >= xMin && y >= yMin && z >= zMin && x <= xMax && y <= yMax && z <= zMax;
	}

	bool Split(Box& out1, Box& out2) const
	{
		double lenX = std::abs(xMax - xMin);
		double lenY = std::abs(yMax - yMin);
		double lenZ = std::abs(zMax - zMin);

		out1 = *this;
		out2 = *this;

		if (lenX >= lenY && lenX >= lenZ)
		{
			// split perpendicular to X
			// Y and Z coordinates remain the same
			
			double newX = (xMax + xMin) / 2;

			// lower box
			out1.xMax = newX;

			// upper box 
			out2.xMin = newX;
		}
		else if (lenY >= lenX && lenY >= lenZ)
		{
			// split perpendicular to Y
			// X and Z coordinates remain the same

			double newY = (yMax + yMin) / 2;

			// lower box
			out1.yMax = newY;

			// upper box 
			out2.yMin = newY;
		}
		else if (lenZ >= lenY && lenZ >= lenX)
		{
			// split perpendicular to Z
			// X and Y coordinates remain the same

			double newZ = (zMax + zMin) / 2;

			// lower box
			out1.zMax = newZ;

			// upper box 
			out2.zMin = newZ;
		}
		else
		{
			return false;
		}
		
		return true;
	}
};
