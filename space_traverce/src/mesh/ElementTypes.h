#pragma once
#include <string>
#include <span>
#include <vector>

#include "utils/Types.h"

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

struct PhisicalName
{
	PhisicalName() {}
	PhisicalName(int Dim, int Tag, const std::string& Name)
		: dimention(Dim), tag(Tag), name(Name) {}

	int dimention = 0;
	int tag = 0;
	std::string name;
};

class Entity
{
public:
	Entity() {}
	virtual ~Entity() {}

	bool HasTag(int tag) const
	{
		for (auto& mytag : physicalTags)
		{
			if (mytag == tag)
				return true;
		}

		return false;
	}

	int entytyTag = 0;
	std::vector<int> physicalTags;

	double minX = 0;
	double minY = 0;
	double minZ = 0;
	double maxX = 0;
	double maxY = 0;
	double maxZ = 0;

	size_t firstElementIndex = 0;
	size_t numElements = 0;
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
