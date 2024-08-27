#pragma once
#include <string>

struct PhisicalName
{
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
	int physicalTag = 0;

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

};

class Volume : public Entity
{
public:



};

struct Node
{
	double x = 0;
	double y = 0;
	double z = 0;
};

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
