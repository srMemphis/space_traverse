#include "Mesh.h"
#include <fstream>
#include <iostream>

#include "GJK.h"
#include "utils/Timer.h"
#include <cassert>

Mesh::Mesh()
{
	m_topologyTree.emplace_back();
}

Mesh::~Mesh()
{
}

bool Mesh::FromFile(std::string filepath)
{

	TIME_SCOPE();

	std::ifstream geofile(filepath);
	if (!geofile.is_open())
	{
		std::cout << "File not found!" << std::endl;
		return false;
	}

	while (geofile.good())
	{
		std::string section;
		std::getline(geofile, section, '$');
		std::getline(geofile, section);

		if (section == "")
			break;

		std::cout << "Reading Section " << section << std::endl;
		if (!ReadSection(geofile, section))
		{
			std::cout << "Error reading section" << std::endl;
			return false;
		}
	}

	return true;
}

std::vector<ETriangle>& Mesh::GetSurfElements()
{
	return m_surfElements;
}

std::vector<ETetrahedron>& Mesh::GetBulkElements()
{
	return m_bulkElements;
}

void Mesh::BuildTopologyData()
{
	// govno
	if (m_topologyTree.size() == 0)
	{
		std::cout << "Error during topology generation; Missing root node!" << std::endl;
		return;
	}
	else if (m_topologyTree.size() > 1)
	{
		m_topologyTree.erase(++m_topologyTree.begin(), m_topologyTree.end());
	}

	// fill topology tree root
	for (int i = 0; i < m_bulkElements.size(); i++)
	{
		m_topologyTree.at(0).elements.push_back(i);
	}


	//m_topologyTree.at(0).boundBox.xMax += 0.01;
	//m_topologyTree.at(0).boundBox.yMax += 0.01;
	//m_topologyTree.at(0).boundBox.zMax += 0.01;

	//m_topologyTree.at(0).boundBox.xMin -= 0.01;
	//m_topologyTree.at(0).boundBox.yMin -= 0.01;
	//m_topologyTree.at(0).boundBox.zMin -= 0.01;

	BuildTopologyRecursive(0);
}

void Mesh::BuildTopologyRecursive(size_t parentID)
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
		auto& element = m_bulkElements.at(index);
		
		bool contains = false;
		// left box.  check every tetrahedron node
		if (left.boundBox.Contains(m_Nodes.at(element.node1).x, m_Nodes.at(element.node1).y, m_Nodes.at(element.node1).z))
		{
			if (std::find(nodesLeft.begin(), nodesLeft.end(), element.node1) == nodesLeft.end()) // if unique
				nodesLeft.push_back(element.node1);
			contains = true;	
		}
		if (left.boundBox.Contains(m_Nodes.at(element.node2).x, m_Nodes.at(element.node2).y, m_Nodes.at(element.node2).z))
		{
			if (std::find(nodesLeft.begin(), nodesLeft.end(), element.node2) == nodesLeft.end()) // if unique
				nodesLeft.push_back(element.node2);
			contains = true;
		}
		if (left.boundBox.Contains(m_Nodes.at(element.node3).x, m_Nodes.at(element.node3).y, m_Nodes.at(element.node3).z))
		{
			if (std::find(nodesLeft.begin(), nodesLeft.end(), element.node3) == nodesLeft.end()) // if unique
				nodesLeft.push_back(element.node3);
			contains = true;
		}
		if (left.boundBox.Contains(m_Nodes.at(element.node4).x, m_Nodes.at(element.node4).y, m_Nodes.at(element.node4).z))
		{
			if (std::find(nodesLeft.begin(), nodesLeft.end(), element.node4) == nodesLeft.end()) // if unique
				nodesLeft.push_back(element.node4);
			contains = true;
		}

		if (!contains) // if nodes are not in the box perform GJK test
			contains = GJK::Check(left.boundBox, element, m_Nodes);

		if(contains)
			left.elements.push_back(index);


		// right box. check every tetrahedron node
		contains = false;

		if (right.boundBox.Contains(m_Nodes.at(element.node1).x, m_Nodes.at(element.node1).y, m_Nodes.at(element.node1).z))
		{
			if (std::find(nodesRight.begin(), nodesRight.end(), element.node1) == nodesRight.end()) // if unique
				nodesRight.push_back(element.node1);
			contains = true;
		}
		if (right.boundBox.Contains(m_Nodes.at(element.node2).x, m_Nodes.at(element.node2).y, m_Nodes.at(element.node2).z))
		{
			if (std::find(nodesRight.begin(), nodesRight.end(), element.node2) == nodesRight.end()) // if unique
				nodesRight.push_back(element.node2);
			contains = true;
		}
		if (right.boundBox.Contains(m_Nodes.at(element.node3).x, m_Nodes.at(element.node3).y, m_Nodes.at(element.node3).z))
		{
			if (std::find(nodesRight.begin(), nodesRight.end(), element.node3) == nodesRight.end()) // if unique
				nodesRight.push_back(element.node3);
			contains = true;
		}
		if (right.boundBox.Contains(m_Nodes.at(element.node4).x, m_Nodes.at(element.node4).y, m_Nodes.at(element.node4).z))
		{
			if (std::find(nodesRight.begin(), nodesRight.end(), element.node4) == nodesRight.end()) // if unique
				nodesRight.push_back(element.node4);
			contains = true;
		}

		if (!contains) // if nodes are not in the box perform GJK test
			contains = GJK::Check(right.boundBox, element, m_Nodes);
		 
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
		BuildTopologyRecursive(leftID);
	}
	if (nodesRight.size() > 1)
	{
		{
			std::vector<size_t> tmp;
			nodesRight.swap(tmp);
		}
		BuildTopologyRecursive(rightID);
	}
}

bool Mesh::ReadSection(std::ifstream& geofile, std::string s)
{

	// not critical path
	// no need to inmplement switch with string to int mapping
	if (s == "MeshFormat")
	{
		return ReadMeshFormat(geofile);
	}
	else if (s == "PhysicalNames")
	{
		return ReadPhysicalNames(geofile);
	}
	else if (s == "Entities")
	{
		return ReadEntities(geofile);
	}
	else if (s == "PartitionedEntities")
	{
		return ReadPartitionedEntities(geofile);
	}
	else if (s == "Nodes")
	{
		return ReadNodes(geofile);
	}
	else if (s == "Elements")
	{
		return ReadElements(geofile);
	}
	else if (s == "Periodic")
	{
		return ReadPeriodic(geofile);
	}
	else if (s == "GhostElements")
	{
		return ReadGhostElements(geofile);
	}
	else if (s == "Parametrizations")
	{
		return ReadParametrizations(geofile);
	}
	else if (s == "NodeData")
	{
		return ReadNodeData(geofile);
	}
	else if (s == "ElementData")
	{
		return ReadElementData(geofile);
	}
	else if (s == "ElementNodeData")
	{
		return ReadElementNodeData(geofile);
	}
	else if (s == "InterpolationScheme")
	{
		return ReadInterpolationScheme(geofile);
	}
	else
	{
		std::cout << "Unlnown section in geo file: " << s <<  std::endl;
		return false;
	}

	return false;
}

bool Mesh::ReadMeshFormat(std::ifstream& geofile)
{
	geofile >> m_Version.first;
	geofile.get();    // get . symbol
	geofile >> m_Version.second;

	std::cout << "MESH version " << m_Version.first << '.' << m_Version.second << std::endl;

	int type;
	int size;

	geofile >> type >> size;

	std::string end;
	geofile.get(); // get \n
	std::getline(geofile, end);
	if (end == "$EndMeshFormat")
		return true;

	return false;
}

bool Mesh::ReadPhysicalNames(std::ifstream& geofile)
{
	int numNames;
	geofile >> numNames;

	for (int i = 0; i < numNames; i++)
	{
		PhisicalName pname;

		geofile >> pname.dimention;
		geofile >> pname.tag;
		std::getline(geofile, pname.name);

		m_PhisicalNames.emplace(std::pair<int, int>( pname.dimention , pname.tag ), pname);

		std::cout << pname.dimention << ' ' << pname.tag << ' ' << pname.name << std::endl;
	}

	std::string end;
	std::getline(geofile, end);
	if (end == "$EndPhysicalNames")
		return true;

	return false;
}

bool Mesh::ReadEntities(std::ifstream& geofile)
{
	size_t numPoints;
	size_t numCurves;
	size_t numSurfaces;
	size_t numVolumes;

	geofile >> numPoints >> numCurves >> numSurfaces >> numVolumes;
	//std::cout << numPoints << ' ' << numCurves << ' ' << numSurfaces << ' ' << numVolumes << std::endl;

	// points
	for (int i = 0; i < numPoints; i++)
	{
		int pointTag;
		double X, Y, Z;

		geofile >> pointTag >> X >> Y >> Z;
		//std::cout << pointTag << ' ' << X << ' ' << Y << ' ' << Z << std::endl;

		size_t numPhysicalTags;
		geofile >> numPhysicalTags;

		for (int j = 0; j < numPhysicalTags; j++)
		{
			int physicalTag;
			geofile >> physicalTag;

		}
	}


	// curves
	for (int i = 0; i < numCurves; i++)
	{
		int curveTag;
		double minX, minY, minZ;
		double maxX, maxY, maxZ;

		geofile >> curveTag >> minX >> minY >> minZ >> maxX >> maxY >> maxZ;

		size_t numPhysicalTags;
		geofile >> numPhysicalTags;

		for (int j = 0; j < numPhysicalTags; j++)
		{
			int physicalTag;
			geofile >> physicalTag;
		}

		size_t numBoundingPoints;
		geofile >> numBoundingPoints;
		for (int j = 0; j < numBoundingPoints; j++)
		{
			int pointTag;
			geofile >> pointTag;
		}
	}
	
	// surfaces
	for (int i = 0; i < numSurfaces; i++)
	{
		int surfaceTag;
		double minX, minY, minZ;
		double maxX, maxY, maxZ;

		geofile >> surfaceTag >> minX >> minY >> minZ >> maxX >> maxY >> maxZ;

		size_t numPhysicalTags;
		geofile >> numPhysicalTags;

		int physicalTag = 0;
		for (int j = 0; j < numPhysicalTags; j++)
		{
			
			geofile >> physicalTag;
		}

		size_t numBoundingCurves;
		geofile >> numBoundingCurves;
		for (int j = 0; j < numBoundingCurves; j++)
		{
			int curveTag;
			geofile >> curveTag;
		}

		Entity tmp;
		tmp.physicalTag = physicalTag;
		tmp.dimention = 2;
		m_Entities.emplace(surfaceTag, tmp);
	}


	// bounding box
	Box box;

	// volumes
	for (int i = 0; i < numVolumes; i++)
	{
		
		int volumeTag;
		double minX, minY, minZ;
		double maxX, maxY, maxZ;

		geofile >> volumeTag >> minX >> minY >> minZ >> maxX >> maxY >> maxZ;
		//std::cout << "Volume tag : " << volumeTag << std::endl;

		// update bounding box
		box.xMax = box.xMax < maxX ? maxX : box.xMax;
		box.yMax = box.yMax < maxY ? maxY : box.yMax;
		box.zMax = box.zMax < maxZ ? maxZ : box.zMax;

		box.xMin = box.xMin > minX ? minX : box.xMin;
		box.yMin = box.yMin > minY ? minY : box.yMin;
		box.zMin = box.zMin > minZ ? minZ : box.zMin;
		//

		size_t numPhysicalTags;
		geofile >> numPhysicalTags;

		int physicalTag = 0;
		for (int j = 0; j < numPhysicalTags; j++)
		{
			geofile >> physicalTag;
		}

		size_t numBoundngSurfaces;
		geofile >> numBoundngSurfaces;
		for (int j = 0; j < numBoundngSurfaces; j++)
		{
			int surfaceTag;
			geofile >> surfaceTag;
		}

		Entity tmp;
		tmp.physicalTag = physicalTag;
		tmp.dimention = 3;
		m_Entities.emplace(volumeTag, tmp);


	}

	// add bounding box to the root of topology tree;
	m_topologyTree.at(0).boundBox = box;


	std::string end;
	geofile.get(); // get \n
	std::getline(geofile, end);
	std::getline(geofile, end);
	if (end == "$EndEntities")
		return true;

	return false;
}

bool Mesh::ReadPartitionedEntities(std::ifstream& geofile)
{
	std::string end;
	std::getline(geofile, end, '$');
	std::getline(geofile, end);
	if (end == "EndPartitionedEntities")
		return true;

	return false;
}

bool Mesh::ReadElements(std::ifstream& geofile)
{

	size_t numEntityBlocks;
	size_t numElements;
	size_t minElementTag;
	size_t maxElementTag;

	geofile >> numEntityBlocks >> numElements >> minElementTag >> maxElementTag;
	//std::cout << numEntityBlocks << ' ' << numElements << ' ' << minElementTag << ' ' << maxElementTag << std::endl;

	size_t prevElementTag = 0;

	for (int i = 0; i < numEntityBlocks; i++)
	{
		int entityDim;
		int entityTag;
		int elementType;
		size_t numElementsInBlock;

		geofile >> entityDim >> entityTag >> elementType >> numElementsInBlock;
		//std::cout << entityDim << ' ' << entityTag << ' ' << elementType << ' ' << numElementsInBlock << std::endl;

		

		for (int j = 0; j < numElementsInBlock; j++)
		{	
			size_t elementTag;

			if (elementType == 2) // 3-node Triangle
			{
				ETriangle element;

				geofile >> elementTag >> element.node1 >> element.node2 >> element.node3;
				//std::cout << elementTag << ' ' << element.node1 << ' ' << element.node2 << ' ' << element.node3 << std::endl;

				m_surfElements.push_back(element);
			}
			else if (elementType == 4) // 4-node Tetrahedron
			{

				ETetrahedron element;

				geofile >> elementTag >> element.node1 >> element.node2 >> element.node3 >> element.node4;
				//std::cout << elementTag << ' ' << element.node1 << ' ' << element.node2
				//		  << ' ' << element.node3 << ' ' << element.node4 << std::endl;

				// gmesh indexing starts from 1
				element.node1 -= 1;
				element.node2 -= 1;
				element.node3 -= 1;
				element.node4 -= 1;

				m_bulkElements.push_back(element);
			}else
			{
				std::cout << "ERROR element type " << elementType << " is not supported!" << std::endl;
				return false;
			}

			if (elementTag - prevElementTag != 1)
			{
				std::cout << "!WARNING! non cousecutive numbering of elements" << std::endl;
				//std::cout << prevElementTag << ' ' << elementTag << std::endl;
			}

			prevElementTag = elementTag;
		}
	}


	std::string end;
	geofile.get(); // get \n
	std::getline(geofile, end);
	std::getline(geofile, end);
	if (end == "$EndElements")
		return true;

	return false;
}

bool Mesh::ReadPeriodic(std::ifstream& geofile)
{
	std::string end;
	std::getline(geofile, end, '$');
	std::getline(geofile, end);
	if (end == "EndPeriodic")
		return true;

	return false;
}

bool Mesh::ReadGhostElements(std::ifstream& geofile)
{
	std::string end;
	std::getline(geofile, end, '$');
	std::getline(geofile, end);
	if (end == "EndGhostElements")
		return true;

	return false;
}

bool Mesh::ReadParametrizations(std::ifstream& geofile)
{
	std::string end;
	std::getline(geofile, end, '$');
	std::getline(geofile, end);
	if (end == "EndParametrizations")
		return true;

	return false;
}

bool Mesh::ReadNodeData(std::ifstream& geofile)
{
	std::string end;
	std::getline(geofile, end, '$');
	std::getline(geofile, end);
	if (end == "EndNodeData")
		return true;

	return false;
}

bool Mesh::ReadElementData(std::ifstream& geofile)
{
	std::string end;
	std::getline(geofile, end, '$');
	std::getline(geofile, end);
	if (end == "EndElementData")
		return true;
	
	return false;
}

bool Mesh::ReadElementNodeData(std::ifstream& geofile)
{
	std::string end;
	std::getline(geofile, end, '$');
	std::getline(geofile, end);
	if (end == "EndElementNodeData")
		return true;

	return false;
}

bool Mesh::ReadInterpolationScheme(std::ifstream& geofile)
{
	std::string end;
	std::getline(geofile, end, '$');
	std::getline(geofile, end);
	if (end == "EndInterpolationScheme")
		return true;

	return false;
}



bool Mesh::ReadNodes(std::ifstream& geofile)
{

	size_t numEntityBlocks; 
	size_t numNodes;
	size_t minNodeTag;
	size_t maxNodeTag;
	int entityDim;
	int entityTag;
	int parametric;
	size_t numNodesInBlock;

	geofile >> numEntityBlocks;
	geofile >> numNodes;
	geofile >> minNodeTag;
	geofile >> maxNodeTag;

	//std::cout << numEntityBlocks << ' ' << numNodes << ' ' << minNodeTag << ' ' << maxNodeTag << std::endl;

	if (minNodeTag != 1)
	{
		std::cout << "ÒÛ ÏÎÆÐÀË ÃÎÂÍÀ" << std::endl;
		assert(false);
	}


	m_Nodes.reserve(numNodes);
	size_t prevNodeTag = 0;

	for (int i = 0; i < numEntityBlocks; i++)
	{
		geofile >> entityDim;
		geofile >> entityTag;
		geofile >> parametric;
		geofile >> numNodesInBlock;

		
		size_t nodeTag;
		for (int j = 0; j < numNodesInBlock; j++)
		{
			
			geofile >> nodeTag;
			
			//std::cout << nodeTag << std::endl;

			if (nodeTag - prevNodeTag != 1)
			{
				std::cout << "!WARNING! non consecutive node numbering" << std::endl;
				std::cout << prevNodeTag << ' ' << nodeTag << std::endl;
			}
			prevNodeTag = nodeTag;
		}
		for (int j = 0; j < numNodesInBlock; j++)
		{
			Node node;
			geofile >> node.x;
			geofile >> node.y;
			geofile >> node.z;

			//std::cout << node.x << ' ' << node.y << ' ' << node.z << std::endl;

			m_Nodes.push_back(node);
		}


	}


	std::string end;
	geofile.get(); // get \n

	std::getline(geofile, end);
	if (end == "$EndNodes")
		return true;

	return false;
}
