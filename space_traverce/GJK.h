#pragma once
#include "Box.h"
#include "ElementTypes.h"

#include <cmath>
#include <vector>
#include <array>

#include <iostream>
#include <fstream>

class GJK
{
private:
	struct Vec3
	{
		double x;
		double y;
		double z;

		void normalize()
		{
			double ivnLength = 1.0 / sqrt(x * x + y * y + z * z);
			x *= ivnLength;
			y *= ivnLength;
			z *= ivnLength;
		}

		double Dot(const Vec3& v) const
		{
			return x*v.x + y*v.y + z*v.z;
		}

		Vec3 Cross(const Vec3& v) const
		{
			return {y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x};
		}

		//binary -
		Vec3 operator-(Vec3 const& v) const
		{
			Vec3 res;
			res.x = this->x - v.x;
			res.y = this->y - v.y;
			res.z = this->z - v.z;
			return res;
		}

		// binary +
		Vec3 operator+(Vec3 const& v) const
		{
			Vec3 res;
			res.x = this->x + v.x;
			res.y = this->y + v.y;
			res.z = this->z + v.z;
			return res;
		}

		// unary -
		Vec3 operator-() const
		{
			Vec3 res;
			res.x = -this->x;
			res.y = -this->y;
			res.z = -this->z;
			return res;
		}

	};
	

public:
	// static class
	GJK() = delete;
	static const uint8_t MAX_ITER = 15;

	// return true if shapes intersect
	static bool Check(const Box& b, const ETetrahedron& t, const std::vector<Node>& nodes)
	{
		// prebuild data
		std::array<Vec3, 8> m_box;
		std::array<Vec3, 4> m_tet;
		
		m_box[0] = { b.xMin, b.yMin, b.zMin};
		m_box[1] = { b.xMax, b.yMin, b.zMin};
		m_box[2] = { b.xMax, b.yMax, b.zMin};
		m_box[3] = { b.xMin, b.yMax, b.zMin};

		m_box[4] = { b.xMin, b.yMin, b.zMax};
		m_box[5] = { b.xMax, b.yMin, b.zMax};
		m_box[6] = { b.xMax, b.yMax, b.zMax};
		m_box[7] = { b.xMin, b.yMax, b.zMax};

		m_tet[0] = { nodes.at(t.node1).x, nodes.at(t.node1).y, nodes.at(t.node1).z };
		m_tet[1] = { nodes.at(t.node2).x, nodes.at(t.node2).y, nodes.at(t.node2).z };
		m_tet[2] = { nodes.at(t.node3).x, nodes.at(t.node3).y, nodes.at(t.node3).z };
		m_tet[3] = { nodes.at(t.node4).x, nodes.at(t.node4).y, nodes.at(t.node4).z };

		// box center
		Vec3 centerB, centerT;
		centerB.x = (b.xMax + b.xMin) / 2.0;
		centerB.y = (b.yMax + b.yMin) / 2.0;
		centerB.z = (b.zMax + b.zMin) / 2.0;

		// tetrahedron center
		centerT.x = (nodes.at(t.node1).x + nodes.at(t.node2).x + nodes.at(t.node3).x + nodes.at(t.node4).x)/4.0;
		centerT.y = (nodes.at(t.node1).y + nodes.at(t.node2).y + nodes.at(t.node3).y + nodes.at(t.node4).y)/4.0;
		centerT.z = (nodes.at(t.node1).z + nodes.at(t.node2).z + nodes.at(t.node3).z + nodes.at(t.node4).z)/4.0;
		
		// center direction
		Vec3 dir = (centerT - centerB);
		dir.normalize();

		// find first support point
		std::vector<Vec3> m_simplex;
		m_simplex.reserve(4);

		m_simplex.push_back(Support(m_box, m_tet, dir));

		// new direction towards the origin
		dir = -m_simplex.at(0);


		int iter = 1;

		while (true)
		{
			// new support point
			Vec3 A = Support(m_box, m_tet, dir);
			if (A.Dot(dir) < 0) // no intersection
			{
				//std::cout << "No intersection" << std::endl;
				//std::cout << "GJK iterations " << iter << std::endl;


				// LOG
				LOG(b, m_tet, false);
				return false;
			}

			// add new point to a simplex
			m_simplex.push_back(A);
			if (HandleSimplex(m_simplex, dir))
			{
				//std::cout << "Intersection found!" << std::endl;
				//std::cout << "GJK iterations " << iter << std::endl;
				// LOG
				LOG(b, m_tet, true);
				return true;
			}

			iter++;

			if (iter > MAX_ITER)
			{
				std::cout << "GJK to many iterations: " << iter << std::endl;
				//LOG(b, m_tet);

				// most of the time there is no intersection
				return false; // FIXME: Consider returning true for reliability
			}
		}
	}


private:
	
	static void LOG(const Box& b, const std::array<Vec3, 4>& tet, bool result)
	{
		#define GJK_LOG
		#ifdef GJK_LOG
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


	static Vec3 Support(const std::array<Vec3, 8>& box, const std::array<Vec3, 4>& tet, const Vec3& dir)
	{
		// box furthest point
		double maxVal = box[0].Dot(dir);
		int maxIDbox = 0;
		for (int i = 1; i < 8; i++)
		{
			double curVal = box[i].Dot(dir);
			if (curVal > maxVal)
			{
				maxVal = curVal;
				maxIDbox = i;
			}
		}

		// tet furthest point
		maxVal = tet[0].Dot(-dir);
		int maxIDtet = 0;
		for (int i = 1; i < 4; i++)
		{
			double curVal = tet[i].Dot(-dir);
			if (curVal > maxVal)
			{
				maxVal = curVal;
				maxIDtet = i;
			}
		}

		return box[maxIDbox] - tet[maxIDtet];
	}
	
	static bool LineCase(std::vector<Vec3>& simp, Vec3& dir)
	{
		Vec3& A = simp[1];
		Vec3& B = simp[0];

		Vec3 AB = B - A;
		Vec3 AO = -A;

		if (AB.Dot(AO) > 0)
		{
			// origin is in the AB region
			// perp to a AB towards the origin
			Vec3 ABperp = AB.Cross(AO).Cross(AB);
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

	static bool TriangleCase(std::vector<Vec3>& simp, Vec3& dir)
	{
		Vec3 AB = simp[1] - simp[2];  // B - A
		Vec3 AC = simp[0] - simp[2];  // C - A
		Vec3 AO = -simp[2];  // -A

		// perp of ABC triangle
		Vec3 ABC = AB.Cross(AC);

		// Check AC
		if (ABC.Cross(AC).Dot(AO) > 0)
		{
			if (AC.Dot(AO) > 0)
			{
				// detelete B point
				//and set direction from AC towards the origin
				simp.erase(std::next(simp.begin(), 1));
				dir = AC.Cross(AO).Cross(AC);
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
			if (AB.Cross(ABC).Dot(AO) > 0)
			{
				// detelete C point
				//and set direction from AB towards the origin
				simp.erase(simp.begin());
				return LineCase(simp, dir);
			}
			else
			{
				// triangle contains the origin
				if (ABC.Dot(AO) > 0)
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

	static bool TetrahedronCase(std::vector<Vec3>& simp, Vec3& dir)
	{
		Vec3 AB = simp[2] - simp[3]; // B - A
		Vec3 AC = simp[1] - simp[3]; // C - A
		Vec3 AD = simp[0] - simp[3]; // D - A
		Vec3 AO = -simp[3]; // -A

		// perp facing ouside of the tetrahedron
		Vec3 ABC = AB.Cross(AC);
		Vec3 ACD = AC.Cross(AD);
		Vec3 ADB = AD.Cross(AB);

		// check if origin is outside of the tetrahedron
		if (ABC.Dot(AO) > 0)
		{
			// Delete D point
			simp.erase(simp.begin());

			return TriangleCase(simp, dir);
		}
		if (ACD.Dot(AO) > 0)
		{
			// Delete B point
			simp.erase(std::next(simp.begin(), 2));

			return TriangleCase(simp, dir);
		}
		if (ADB.Dot(AO) > 0)
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

	static bool HandleSimplex(std::vector<Vec3>& simp, Vec3& dir)
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
