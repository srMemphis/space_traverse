#pragma once
#include "Eigen/Dense"
#include "utils/Types.h"

#include <cmath>


class Geometry
{
public:
	// return A, B, C, D coefficients of a plane
	static Eigen::Vector4d ThreePointPlane(const Eigen::Vector3d& p1, const Eigen::Vector3d& p2,
										   const Eigen::Vector3d& p3)
	{
		auto v12 = p2 - p1;
		auto v13 = p3 - p1;

		auto n = (v12.cross(v13)).normalized();

		double d = -n(0) * p1(0) - n(1) * p1(1) - n(2) * p1(2);

		return Eigen::Vector4d{n(0), n(1), n(2), d};
	}

	// returns gradient vector from point 1 to plane 234
	static Eigen::Vector3d BarycentricGradient(const Eigen::Vector3d& p1, const Eigen::Vector3d& p2,
											   const Eigen::Vector3d& p3, const Eigen::Vector3d& p4)
	{
		auto plane = ThreePointPlane(p2, p3, p4);

		double t = -(plane(0) * p1(0) + plane(1) * p1(1) + plane(2) * p1(2) + plane(3)) /
				   (plane(0) * plane(0) + plane(1) * plane(1) + plane(2) * plane(2));

		Eigen::Vector3d intersection = {p1(0) + t * plane(0), p1(1) + t * plane(1), p1(2) + t * plane(2)};

		Eigen::Vector3d vector = p1 - intersection;

		vector = vector.array() / (vector(0) * vector(0) + vector(1) * vector(1) + vector(2) * vector(2));

		return vector;
	}

	// return area of a triangle
	static double TriangleArea(const Eigen::Vector3d& p1, const Eigen::Vector3d& p2, const Eigen::Vector3d& p3)
	{
		return ((p2 - p1).cross(p3 - p1)).norm() / 2;
	}

	// return volume of tetrahedron
	static double TetrahedronVolume(const Eigen::Vector3d& p1, const Eigen::Vector3d& p2, const Eigen::Vector3d& p3,
									const Eigen::Vector3d& p4)
	{
		Eigen::Matrix3d jacob({{p2(0) - p1(0), p2(1) - p1(1), p2(2) - p1(2)},
							   {p3(0) - p1(0), p3(1) - p1(1), p3(2) - p1(2)},
							   {p4(0) - p1(0), p4(1) - p1(1), p4(2) - p1(2)}});
		double volume = jacob.determinant() / 6.0;
		return volume;
	}

	static double TetrahedronVolume(const Node& p1, const Node& p2, const Node& p3, const Node& p4)
	{
		Eigen::Matrix3d jacob({{p2.x - p1.x, p2.y - p1.y, p2.z - p1.z},
							   {p3.x - p1.x, p3.y - p1.y, p3.z - p1.z},
							   {p4.x - p1.x, p4.y - p1.y, p4.z - p1.z}});
		double volume = jacob.determinant() / 6.0;
		return volume;
	}

private:
	Geometry() = delete;
};
