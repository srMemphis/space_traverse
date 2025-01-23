#pragma once
#include "mesh/Box.h"
#include "mesh/ElementTypes.h"
#include "utils/Vec3.h"

#include <Eigen/Dense>

#include "debug/Log.h"

#include <cmath>
#include <vector>
#include <array>

#include <iostream>
#include <fstream>

#define GJK_LOG 0
class GJK
{
public:
	// static class
	GJK() = delete;
	static const uint8_t MAX_ITER = 15;

	// return true if shapes intersect
	static bool Check(const Box& b, const ETetrahedron& t, const std::vector<Node>& nodes)
	{
		// prebuild data
		std::array<Eigen::Vector3d, 8> m_box;
		std::array<Eigen::Vector3d, 4> m_tet;

		m_box[0] = { b.xMin, b.yMin, b.zMin };
		m_box[1] = { b.xMax, b.yMin, b.zMin };
		m_box[2] = { b.xMax, b.yMax, b.zMin };
		m_box[3] = { b.xMin, b.yMax, b.zMin };

		m_box[4] = { b.xMin, b.yMin, b.zMax };
		m_box[5] = { b.xMax, b.yMin, b.zMax };
		m_box[6] = { b.xMax, b.yMax, b.zMax };
		m_box[7] = { b.xMin, b.yMax, b.zMax };

		m_tet[0] = { nodes.at(t.node[0]).x, nodes.at(t.node[0]).y, nodes.at(t.node[0]).z };
		m_tet[1] = { nodes.at(t.node[1]).x, nodes.at(t.node[1]).y, nodes.at(t.node[1]).z };
		m_tet[2] = { nodes.at(t.node[2]).x, nodes.at(t.node[2]).y, nodes.at(t.node[2]).z };
		m_tet[3] = { nodes.at(t.node[3]).x, nodes.at(t.node[3]).y, nodes.at(t.node[3]).z };

		// box center
		Eigen::Vector3d centerB, centerT;
		centerB.x() = (b.xMax + b.xMin) / 2.0;
		centerB.y() = (b.yMax + b.yMin) / 2.0;
		centerB.z() = (b.zMax + b.zMin) / 2.0;

		// tetrahedron center
		centerT.x() = (nodes.at(t.node[0]).x + nodes.at(t.node[1]).x + nodes.at(t.node[2]).x + nodes.at(t.node[3]).x) / 4.0;
		centerT.y() = (nodes.at(t.node[0]).y + nodes.at(t.node[1]).y + nodes.at(t.node[2]).y + nodes.at(t.node[3]).y) / 4.0;
		centerT.z() = (nodes.at(t.node[0]).z + nodes.at(t.node[1]).z + nodes.at(t.node[2]).z + nodes.at(t.node[3]).z) / 4.0;

		// center direction
		Eigen::Vector3d dir = centerT - centerB;
		dir.normalize();

		// find first support point
		std::vector<Eigen::Vector3d> m_simplex;
		m_simplex.reserve(4);

		m_simplex.push_back(Support(m_box, m_tet, dir));

		// new direction towards the origin
		dir = -m_simplex.at(0);


		int iter = 1;

		while (true)
		{
			// new support point
			Eigen::Vector3d A = Support(m_box, m_tet, dir);
			if (A.dot(dir) < 0) // no intersection
			{
				//std::cout << "No intersection" << std::endl;
				//std::cout << "GJK iterations " << iter << std::endl;


				// LOG
				//LOG(b, m_tet, false);
				return false;
			}

			// add new point to a simplex
			m_simplex.push_back(A);
			if (HandleSimplex(m_simplex, dir))
			{
				//std::cout << "Intersection found!" << std::endl;
				//std::cout << "GJK iterations " << iter << std::endl;
				// LOG
				//LOG(b, m_tet, true);
				return true;
			}

			iter++;

			if (iter > MAX_ITER)
			{
				//LOG_TRACE("GJK to many iterations: {}", iter);
				//LOG(b, m_tet);

				// most of the time there is no intersection
				return false; // FIXME: Consider returning true for reliability
			}
		}
	}

private:
	
	static void LOG(const Box& b, const std::array<Eigen::Vector3d, 4>& tet, bool result)
	{
		#if (GJK_LOG == 1)
			std::ofstream myfile;
			myfile.open("GJKlog.txt", std::ios::app);
			
			myfile << "box = [" << b.xMin << ", " << b.yMin << ", " << b.zMin << ", " << b.xMax << ", " << b.yMax << ", " << b.zMax << "]\n";
			myfile << "tet = [" << tet[0].x << ", " << tet[0].y << ", " << tet[0].z << '\n';
			myfile << tet[1].x << ", " << tet[1].y << ", " << tet[1].z << '\n';
			myfile << tet[2].x << ", " << tet[2].y << ", " << tet[2].z << '\n';
			myfile << tet[3].x << ", " << tet[3].y << ", " << tet[3].z << "]\n";
			if (result)
			{
				myfile << "HIT\n";
			}
			else
			{
				myfile << "MISS\n";
			}
			myfile << '\n';
			
			myfile.close();
		#endif
	}


	static Eigen::Vector3d Support(const std::array<Eigen::Vector3d, 8>& box, const std::array<Eigen::Vector3d, 4>& tet, const Eigen::Vector3d& dir)
	{
		// box furthest point
		double maxVal = box[0].dot(dir);
		int maxIDbox = 0;
		for (int i = 1; i < 8; i++)
		{
			double curVal = box[i].dot(dir);
			if (curVal > maxVal)
			{
				maxVal = curVal;
				maxIDbox = i;
			}
		}

		// tet furthest point
		maxVal = tet[0].dot(-dir);
		int maxIDtet = 0;
		for (int i = 1; i < 4; i++)
		{
			double curVal = tet[i].dot(-dir);
			if (curVal > maxVal)
			{
				maxVal = curVal;
				maxIDtet = i;
			}
		}

		return box[maxIDbox] - tet[maxIDtet];
	}
	
	static bool LineCase(std::vector<Eigen::Vector3d>& simp, Eigen::Vector3d& dir)
	{
		Eigen::Vector3d& A = simp[1];
		Eigen::Vector3d& B = simp[0];

		Eigen::Vector3d AB = B - A;
		Eigen::Vector3d AO = -A;

		if (AB.dot(AO) > 0)
		{
			// origin is in the AB region
			// perp to a AB towards the origin
			Eigen::Vector3d ABperp = AB.cross(AO).cross(AB);
			ABperp.normalize();
			dir = ABperp;
		}
		else
		{
			// origin is not between first and second point
			// delete first point and set direction from A to the origin;
			simp.erase(simp.begin());
			dir = AO;
		}

		// return false -> Add new point to the simplex
		return false;
	}

	static bool TriangleCase(std::vector<Eigen::Vector3d>& simp, Eigen::Vector3d& dir)
	{
		Eigen::Vector3d AB = simp[1] - simp[2];  // B - A
		Eigen::Vector3d AC = simp[0] - simp[2];  // C - A
		Eigen::Vector3d AO = -simp[2];  // -A

		// perp of ABC triangle
		Eigen::Vector3d ABC = AB.cross(AC);

		// Check AC
		if (ABC.cross(AC).dot(AO) > 0)
		{
			if (AC.dot(AO) > 0)
			{
				// detelete B point
				//and set direction from AC towards the origin
				simp.erase(std::next(simp.begin(), 1));
				dir = AC.cross(AO).cross(AC);
			}
			else
			{
				// delete C point
				// AB line case
				simp.erase(simp.begin());
				return LineCase(simp, dir);
			}
		}
		// Check AB
		else
		{
			if (AB.cross(ABC).dot(AO) > 0)
			{
				// detelete C point
				//and set direction from AB towards the origin
				simp.erase(simp.begin());
				return LineCase(simp, dir);
			}
			else
			{
				// triangle contains the origin
				if (ABC.dot(AO) > 0)
				{
					dir = ABC;
				}
				else
				{
					// swap B and C points
					std::swap(simp[0], simp[1]);
					dir = -ABC;
				}
			}
		}

		// return false -> Add new point to the simplex
		return false;
	}

	static bool TetrahedronCase(std::vector<Eigen::Vector3d>& simp, Eigen::Vector3d& dir)
	{
		Eigen::Vector3d AB = simp[2] - simp[3]; // B - A
		Eigen::Vector3d AC = simp[1] - simp[3]; // C - A
		Eigen::Vector3d AD = simp[0] - simp[3]; // D - A
		Eigen::Vector3d AO = -simp[3]; // -A

		// perp facing ouside of the tetrahedron
		Eigen::Vector3d ABC = AB.cross(AC);
		Eigen::Vector3d ACD = AC.cross(AD);
		Eigen::Vector3d ADB = AD.cross(AB);

		// check if origin is outside of the tetrahedron
		if (ABC.dot(AO) > 0)
		{
			// Delete D point
			simp.erase(simp.begin());

			return TriangleCase(simp, dir);
		}
		if (ACD.dot(AO) > 0)
		{
			// Delete B point
			simp.erase(std::next(simp.begin(), 2));

			return TriangleCase(simp, dir);
		}
		if (ADB.dot(AO) > 0)
		{
			//  Delete C point
			simp.erase(std::next(simp.begin(), 1));

			// swap B and D points
			std::swap(simp[0], simp[1]);

			return TriangleCase(simp, dir);
		}

		// origin is inside the tetrahedron
		// intersection found
		return true;
	}

	static bool HandleSimplex(std::vector<Eigen::Vector3d>& simp, Eigen::Vector3d& dir)
	{
		switch (simp.size())
		{
		case 2: // line simplex
		{
			return LineCase(simp, dir);
		}
		case 3: // triangle simplex
		{
			return TriangleCase(simp, dir);
		}
		case 4: // tetrahedron simplex
		{
			return TetrahedronCase(simp, dir);
		}
		default:
			std::cout << "[ERROR] GJK::Unknows Simplex size!" << std::endl;
			return false;
		}
	}
};
