#pragma once
#include "mesh/Box.h"
#include "mesh/ElementTypes.h"

#include <map>
#include <vector>


class TopologyTree
{
public:
	TopologyTree();
	~TopologyTree();

	// modifiers
	bool GenerateData(const std::map<int, Volume>& volumes, const std::vector<ETetrahedron>& elements,
					  const std::vector<Node>& nodes);

	// utils
	const std::vector<size_t>& Find(double x, double y, double z) const; // returns candidate vector;
	bool IsInBounds(double x, double y, double z) const;                 // check if point is inside bounding box

	// stats
	const uint32_t MaxTetraAtNode() const;

	void Clear();
	bool IsEmpty() const;

private:
	struct treeNode
	{
		std::vector<size_t> elements;
		Box boundBox;
		size_t left = 0;
		size_t right = 0;
	};

	std::vector<treeNode> m_topologyTree;
	void BuildTopologyRecursive(size_t parentID, const std::vector<ETetrahedron>& elements,
								const std::vector<Node>& nodes);

	// statistics
	size_t m_maxTetrahedronsAtNode;

	// utils
	std::vector<size_t> m_emptyVector; // empty vector for returning as const&
};
