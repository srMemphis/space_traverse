#pragma once
#include <string>

struct PhisicalName
{
	int dimention;
	int tag;
	std::string name;
};

struct Node
{
	double x;
	double y;
	double z;
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
