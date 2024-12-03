#include "mesh/TopologyTree.h"

#include "mesh/GJK.h"
#include "debug/Log.h"

TopologyTree::TopologyTree()
	: m_maxTetrahedronsAtNode(0)
{
}

TopologyTree::~TopologyTree()
{
}

bool TopologyTree::GenerateData(const std::map<int, Volume>& volumes, const std::vector<ETetrahedron>& elements, const std::vector<Node>& nodes)
{
	// if no geometry available
	if (elements.size() == 0)
	{
		return false;
	}

	// create root node
	// 
	// bounding box
	Box box;

	// update bounding box
	for (auto& pair : volumes)
	{
		auto& v = pair.second;

		box.xMax = box.xMax < v.maxX ? v.maxX : box.xMax;
		box.yMax = box.yMax < v.maxY ? v.maxY : box.yMax;
		box.zMax = box.zMax < v.maxZ ? v.maxZ : box.zMax;

		box.xMin = box.xMin > v.minX ? v.minX : box.xMin;
		box.yMin = box.yMin > v.minY ? v.minY : box.yMin;
		box.zMin = box.zMin > v.minZ ? v.minZ : box.zMin;
	}

	// add bounding box to the root of topology tree;
	m_topologyTree.clear();
	m_topologyTree.emplace_back();
	m_topologyTree.at(0).boundBox = box;

	// fill topology tree root with elements
	for (int i = 0; i < elements.size(); i++)
	{
		m_topologyTree.at(0).elements.push_back(i);
	}

	// make box slightly bigger to ensure that edges are inside
	//m_topologyTree.at(0).boundBox.xMax += 0.01;
	//m_topologyTree.at(0).boundBox.yMax += 0.01;
	//m_topologyTree.at(0).boundBox.zMax += 0.01;

	//m_topologyTree.at(0).boundBox.xMin -= 0.01;
	//m_topologyTree.at(0).boundBox.yMin -= 0.01;
	//m_topologyTree.at(0).boundBox.zMin -= 0.01;

	BuildTopologyRecursive(0, elements, nodes);

	// statistics
	m_maxTetrahedronsAtNode = 0;

	for (auto& node : m_topologyTree)
	{
		m_maxTetrahedronsAtNode = node.elements.size() > m_maxTetrahedronsAtNode ? node.elements.size() : m_maxTetrahedronsAtNode;
	}

	return true;
}

const std::vector<size_t>& TopologyTree::Find(double x, double y, double z) const
{
	
	// check if point is inside bounding box
	if (!m_topologyTree.at(0).boundBox.Contains(x, y, z))
	{
		// returns empty vector
		return std::vector<size_t>();
	}

	// find node that contains elements
	size_t currTreeNode = 0;
	while (m_topologyTree.at(currTreeNode).elements.empty())
	{
		auto left = m_topologyTree.at(currTreeNode).left;
		auto right = m_topologyTree.at(currTreeNode).right;

		if (m_topologyTree.at(left).boundBox.Contains(x, y, z))
		{
			// search left 
			currTreeNode = left;
		}
		else
		{
			// search right
			currTreeNode = right;
		}
	}

	// return list of elements
	return m_topologyTree.at(currTreeNode).elements;
}

const uint32_t TopologyTree::MaxTetraAtNode() const
{
	return m_maxTetrahedronsAtNode;
}

void TopologyTree::Clear()
{
	m_topologyTree.clear();
}

bool TopologyTree::IsEmpty() const
{
	return m_topologyTree.empty();
}

// TODO: rewrite this SHIT
void TopologyTree::BuildTopologyRecursive(size_t parentID, const std::vector<ETetrahedron>& elements, const std::vector<Node>& nodes)
{
	// create new tree branches
	m_topologyTree.emplace_back();
	size_t leftID = m_topologyTree.size() - 1;
	m_topologyTree.at(parentID).left = leftID;

	m_topologyTree.emplace_back();
	size_t rightID = m_topologyTree.size() - 1;
	m_topologyTree.at(parentID).right = rightID;

	// create local reffs after expanding tree vector
	treeNode& parent = m_topologyTree.at(parentID);
	treeNode& left = m_topologyTree.at(leftID);
	treeNode& right = m_topologyTree.at(rightID);

	// split parent bounding box into two
	parent.boundBox.Split(left.boundBox, right.boundBox);

	// search for elements in new boxes
	std::vector<size_t> nodesLeft, nodesRight; // unique nodes in the box

	for (auto index : parent.elements)
	{
		auto& element = elements.at(index);

		bool contains = false;
		// left box.  check every tetrahedron node
		if (left.boundBox.Contains(nodes.at(element.node[0]).x, nodes.at(element.node[0]).y, nodes.at(element.node[0]).z))
		{
			if (std::find(nodesLeft.begin(), nodesLeft.end(), element.node[0]) == nodesLeft.end()) // if unique
				nodesLeft.push_back(element.node[0]);
			contains = true;
		}
		if (left.boundBox.Contains(nodes.at(element.node[1]).x, nodes.at(element.node[1]).y, nodes.at(element.node[1]).z))
		{
			if (std::find(nodesLeft.begin(), nodesLeft.end(), element.node[1]) == nodesLeft.end()) // if unique
				nodesLeft.push_back(element.node[1]);
			contains = true;
		}
		if (left.boundBox.Contains(nodes.at(element.node[2]).x, nodes.at(element.node[2]).y, nodes.at(element.node[2]).z))
		{
			if (std::find(nodesLeft.begin(), nodesLeft.end(), element.node[2]) == nodesLeft.end()) // if unique
				nodesLeft.push_back(element.node[2]);
			contains = true;
		}
		if (left.boundBox.Contains(nodes.at(element.node[3]).x, nodes.at(element.node[3]).y, nodes.at(element.node[3]).z))
		{
			if (std::find(nodesLeft.begin(), nodesLeft.end(), element.node[3]) == nodesLeft.end()) // if unique
				nodesLeft.push_back(element.node[3]);
			contains = true;
		}

		if (!contains) // if nodes are not in the box perform GJK test
			contains = GJK::Check(left.boundBox, element, nodes);

		if (contains)
			left.elements.push_back(index);


		// right box. check every tetrahedron node
		contains = false;

		if (right.boundBox.Contains(nodes.at(element.node[0]).x, nodes.at(element.node[0]).y, nodes.at(element.node[0]).z))
		{
			if (std::find(nodesRight.begin(), nodesRight.end(), element.node[0]) == nodesRight.end()) // if unique
				nodesRight.push_back(element.node[0]);
			contains = true;
		}
		if (right.boundBox.Contains(nodes.at(element.node[1]).x, nodes.at(element.node[1]).y, nodes.at(element.node[1]).z))
		{
			if (std::find(nodesRight.begin(), nodesRight.end(), element.node[1]) == nodesRight.end()) // if unique
				nodesRight.push_back(element.node[1]);
			contains = true;
		}
		if (right.boundBox.Contains(nodes.at(element.node[2]).x, nodes.at(element.node[2]).y, nodes.at(element.node[2]).z))
		{
			if (std::find(nodesRight.begin(), nodesRight.end(), element.node[2]) == nodesRight.end()) // if unique
				nodesRight.push_back(element.node[2]);
			contains = true;
		}
		if (right.boundBox.Contains(nodes.at(element.node[3]).x, nodes.at(element.node[3]).y, nodes.at(element.node[3]).z))
		{
			if (std::find(nodesRight.begin(), nodesRight.end(), element.node[3]) == nodesRight.end()) // if unique
				nodesRight.push_back(element.node[3]);
			contains = true;
		}

		if (!contains) // if nodes are not in the box perform GJK test
			contains = GJK::Check(right.boundBox, element, nodes);

		if (contains)
			right.elements.push_back(index);
	}

	// clear parent element array
	{
		std::vector<size_t> tmp;
		parent.elements.swap(tmp);
		// tmp deletes here
	}

	// propagate split if there is more than 1 node in the box
	if (nodesLeft.size() > 1)
	{
		{
			std::vector<size_t> tmp;
			nodesLeft.swap(tmp);
		}
		BuildTopologyRecursive(leftID, elements , nodes);
	}
	if (nodesRight.size() > 1)
	{
		{
			std::vector<size_t> tmp;
			nodesRight.swap(tmp);
		}
		BuildTopologyRecursive(rightID, elements, nodes);
	}
}
