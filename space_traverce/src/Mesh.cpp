#include "Mesh.h"

#include "utils/Timer.h"
#include "utils/Vec3.h"

#include <fstream>
#include <iostream>
#include <cassert>


Mesh::Mesh()
{
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

	// clear existing data
	Clear();

	// read file by sections
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

	// fill datastructures
	m_PhisicalNames.emplace(std::pair<int, int>{2, 0}, PhisicalName(2, 0, "No Name"));
	m_PhisicalNames.emplace(std::pair<int, int>{3, 0}, PhisicalName(3, 0, "No Name"));

	for (auto& surf : m_surfaces)
	{
		// write full phisical name data
		surf.second.phisicalName = m_PhisicalNames.find({ 2, surf.second.phisicalName.tag})->second;

		// assign a span of elements to the entity
		surf.second.elements = {m_surfElements.begin() + surf.second.firstElementIndex,
								m_surfElements.begin() + surf.second.firstElementIndex + surf.second.numElements};
	}

	for (auto& vol : m_volumes)
	{
		// write full phisical name data
		vol.second.phisicalName = m_PhisicalNames.find({ 3, vol.second.phisicalName.tag })->second;

		// assign a span of elements to the entity
		vol.second.elements = { m_bulkElements.begin() + vol.second.firstElementIndex,
								m_bulkElements.begin() + vol.second.firstElementIndex + vol.second.numElements };
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

bool Mesh::BuildTopologyData()
{
	TIME_SCOPE();
	m_topologyTree.Clear();

	if (!m_topologyTree.GenerateData(m_volumes, m_bulkElements, m_Nodes))
	{
		std::cerr << "Error during topology generation; Missing geometry data!" << std::endl;
		return false;
	}

	return true;
}

void Mesh::Clear()
{
	m_Version = { 0, 0 };
	m_Nodes.clear();
	m_surfElements.clear();
	m_bulkElements.clear();
	m_surfaces.clear();
	m_volumes.clear();
	m_topologyTree.Clear();
}

int64_t Mesh::FindElementByCoords(double x, double y, double z) const 
{
	//convect coords to vector
	Vec3 point = { x, y, z };

	auto& candidates = m_topologyTree.Find(x, y, z);

	if (candidates.empty())
	{
		// point is outside of the geometry
		std::cerr << "[WARNING] [Mesh::FindElementByCoords] Point is outside of the geometry" << std::endl;
		return -1;
	}

	// check every tetrahedron in the candidates list
	for (auto index : candidates)
	{
		auto& elem = m_bulkElements.at(index);

		// check if point is indide tetrahedron by 
		// checking if it is to the right side of each
		// tetrahedrons face

		Vec3 node0 = { m_Nodes.at(elem.node[0]).x, m_Nodes.at(elem.node[0]).y, m_Nodes.at(elem.node[0]).z };
		Vec3 node1 = { m_Nodes.at(elem.node[1]).x, m_Nodes.at(elem.node[1]).y, m_Nodes.at(elem.node[1]).z };
		Vec3 node2 = { m_Nodes.at(elem.node[2]).x, m_Nodes.at(elem.node[2]).y, m_Nodes.at(elem.node[2]).z };
		Vec3 node3 = { m_Nodes.at(elem.node[3]).x, m_Nodes.at(elem.node[3]).y, m_Nodes.at(elem.node[3]).z };
		
		Vec3 vec01 = node1 - node0;
		Vec3 vec02 = node2 - node0;
		Vec3 vec03 = node3 - node0;
		Vec3 vec21 = node1 - node2;
		Vec3 vec23 = node3 - node2;

		Vec3 vec0p = point - node0;

		double s0 = (vec01.Cross(vec02)).Dot(vec0p);
		double s1 = (vec03.Cross(vec01)).Dot(vec0p);
		double s2 = (vec02.Cross(vec03)).Dot(vec0p);
		double s3 = (vec21.Cross(vec23)).Dot(point - node2);

		if (s0 > 0 && s1 > 0 && s2 > 0 && s3 > 0)
		{
			return index;
		}
	}

	// failed to find tetrahedron in the candidate list
	// should never happen
	// critical error
	std::cerr << "[ERROR] [Mesh::FindElementByCoords] Can not find tetrahedron that contains the point" << std::endl;
	return -2;
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

		// remove quotes and space characters
		pname.name.erase(0, 2);                  // erase the first two characters
		pname.name.erase(pname.name.size() - 1); // erase the last character

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
		// buffer
		Surface tmp;

		// read to buffer
		geofile >> tmp.entytyTag >> tmp.minX >> tmp.minY >> tmp.minZ >> tmp.maxX >> tmp.maxY >> tmp.maxZ;

		size_t numPhysicalTags;
		geofile >> numPhysicalTags;
		
		for (int j = 0; j < numPhysicalTags; j++)
		{
			geofile >> tmp.phisicalName.tag;
			//std::cout << "Surface " << tmp.entytyTag << " has " << tmp.phisicalName.tag << " Phisical tag" << std::endl;
		}

		size_t numBoundingCurves;
		geofile >> numBoundingCurves;
		for (int j = 0; j < numBoundingCurves; j++)
		{
			int curveTag;
			geofile >> curveTag;
		}

		// save
		m_surfaces.emplace(tmp.entytyTag, tmp);
	}

	// volumes
	for (int i = 0; i < numVolumes; i++)
	{
		// buffer
		Volume tmp;

		// read from file to buffer
		geofile >> tmp.entytyTag >> tmp.minX >> tmp.minY >> tmp.minZ >> tmp.maxX >> tmp.maxY >> tmp.maxZ;
		//std::cout << "Volume tag : " << volumeTag << std::endl;

		size_t numPhysicalTags;
		geofile >> numPhysicalTags;

		for (int j = 0; j < numPhysicalTags; j++)
		{
			geofile >> tmp.phisicalName.tag;
		}

		size_t numBoundngSurfaces;
		geofile >> numBoundngSurfaces;
		for (int j = 0; j < numBoundngSurfaces; j++)
		{
			int surfaceTag;
			geofile >> surfaceTag;
		}

		//save
		m_volumes.emplace(tmp.entytyTag, tmp);
	}

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

	for (int i = 0; i < numEntityBlocks; i++)
	{
		int entityDim;
		int entityTag;
		int elementType;
		size_t numElementsInBlock;

		geofile >> entityDim >> entityTag >> elementType >> numElementsInBlock;
		//std::cout << entityDim << ' ' << entityTag << ' ' << elementType << ' ' << numElementsInBlock << std::endl;

		size_t elementTag;

		switch (entityDim)
		{
		case 2: // surface
		{
			if (elementType != 2) // 3-node Triangle
			{
				std::cerr << "ERROR element type " << elementType << " is not supported!" << std::endl;
				return false;
			}

			// fill entity data
			auto& surface = m_surfaces.find(entityTag)->second;
			surface.numElements = numElementsInBlock;
			surface.firstElementIndex = m_surfElements.size();

			// read elements
			for (int j = 0; j < numElementsInBlock; j++)
			{
				ETriangle element;

				geofile >> elementTag >> element.node[0] >> element.node[1] >> element.node[2];
				//std::cout << elementTag << ' ' << element.node1 << ' ' << element.node2 << ' ' << element.node3 << std::endl;

				// gmsh indexing starts from 1
				element.node[0] -= 1;
				element.node[1] -= 1;
				element.node[2] -= 1;

				m_surfElements.push_back(element);
			}
			break;
		}
		case 3: // volume
		{
			if (elementType != 4) // 4-node Tetrahedron
			{
				std::cerr << "ERROR element type " << elementType << " is not supported!" << std::endl;
				return false;
			}

			// fill entity data
			auto& volume = m_volumes.find(entityTag)->second;
			volume.numElements = numElementsInBlock;
			volume.firstElementIndex = m_bulkElements.size();

			for (int j = 0; j < numElementsInBlock; j++)
			{
				ETetrahedron element;

				geofile >> elementTag >> element.node[0] >> element.node[1] >> element.node[2] >> element.node[3];
				//std::cout << elementTag << ' ' << element.node1 << ' ' << element.node2
				//		  << ' ' << element.node3 << ' ' << element.node4 << std::endl;

				// gmsh indexing starts from 1
				element.node[0] -= 1;
				element.node[1] -= 1;
				element.node[2] -= 1;
				element.node[3] -= 1;

				m_bulkElements.push_back(element);
			}
			break;
		}
		default:break;
		} // switch

	} // entity block


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
