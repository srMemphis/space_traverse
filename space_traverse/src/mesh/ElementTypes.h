#pragma once
#include "utils/Types.h"

#include <span>
#include <string>
#include <vector>

// 3-node triangle element
struct ETriangle
{
	size_t node[3];
};

// 4-node tetrahedron element
struct ETetrahedron
{
	size_t node[4];
};

struct PhysicalName
{
	PhysicalName() {}
	PhysicalName(int Dim, int Tag, const std::string& Name) : dimention(Dim), tag(Tag), name(Name) {}

	int dimention = 0;
	int tag = 0;
	std::string name;
};

class Entity
{
public:
	Entity() {}
	virtual ~Entity() {}

	int entytyTag = 0;
	int physicalTag = -1;

	double minX = 0;
	double minY = 0;
	double minZ = 0;
	double maxX = 0;
	double maxY = 0;
	double maxZ = 0;

	size_t firstElementIndex = 0;
	size_t numElements = 0;

	bool HasTag(int tag) const
	{
		if (physicalTag == tag)
			return true;

		return false;
	}
};

class Surface : public Entity
{
public:
	std::span<ETriangle> elements;
	static const int dimention = 2;
};

class Volume : public Entity
{
public:
	std::span<ETetrahedron> elements;
	static const int dimention = 3;
};
