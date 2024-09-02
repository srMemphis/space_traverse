#pragma once
#include <cmath>

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
		return x * v.x + y * v.y + z * v.z;
	}

	Vec3 Cross(const Vec3& v) const
	{
		return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x };
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
