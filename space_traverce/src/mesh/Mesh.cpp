#include "mesh/Mesh.h"

#include "debug/Profiler.h"
#include "utils/Vec3.h"
#include "utils/Geometry.h"
#include "debug/Log.h"

#include <Eigen/Dense>

#include <fstream>
#include <iostream>
#include <cassert>
#include <algorithm>

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

bool Mesh::FromFile(std::string filepath)
{
	PROFILE_FUNCTION();

	std::ifstream geofile(filepath);
	if (!geofile.is_open())
	{
		LOG_ERROR("Mesh file {} is not found", filepath);

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

		LOG_TRACE("Reading Section {}", section);

		if (!ReadSection(geofile, section))
		{
			LOG_ERROR("Error reading section {}", section);
			return false;
		}
	}

	// fill datastructures
	FillEntitiesWithElements();
	GenerateGradientVectors();

	return true;
}

const std::vector<ETriangle>& Mesh::GetSurfElements() const
{
	return m_surfElements;
}

const std::vector<ETetrahedron>& Mesh::GetBulkElements() const
{
	return m_bulkElements;
}

const std::vector<Node>& Mesh::GetNodes() const
{
	return m_Nodes;
}

const std::map<int, Volume>& Mesh::GetVolumes() const
{
	return m_volumes;
}

const std::map<int, Surface>& Mesh::GetSurfaces() const
{
	return m_surfaces;
}

const ETriangle& Mesh::GetTriangle(size_t index) const
{
	return m_surfElements.at(index);
}

const ETetrahedron& Mesh::GetTetrahedron(size_t index) const
{
	return m_bulkElements.at(index);
}

const Node& Mesh::GetNode(size_t index) const
{
	return m_Nodes.at(index);
}

uint32_t Mesh::MaxTetraAtNode() const
{
	std::vector<uint16_t> histogram(m_Nodes.size(), 0);

	for (auto& tetra : m_bulkElements)
	{
		for (int i = 0; i < 4; i++)
		{
			histogram.at(tetra.node[i]) += 1;
		}
	}
	
	return *std::max_element(histogram.begin(), histogram.end());
}

double Mesh::GetVolumeOfTetrahedron(size_t index) const
{
	auto& tetra = m_bulkElements.at(index);
	Eigen::Matrix3d jacob(
		{
			{m_Nodes.at(tetra.node[1]).x - m_Nodes.at(tetra.node[0]).x, m_Nodes.at(tetra.node[1]).y - m_Nodes.at(tetra.node[0]).y, m_Nodes.at(tetra.node[1]).z - m_Nodes.at(tetra.node[0]).z},
			{m_Nodes.at(tetra.node[2]).x - m_Nodes.at(tetra.node[0]).x, m_Nodes.at(tetra.node[2]).y - m_Nodes.at(tetra.node[0]).y, m_Nodes.at(tetra.node[2]).z - m_Nodes.at(tetra.node[0]).z},
			{m_Nodes.at(tetra.node[3]).x - m_Nodes.at(tetra.node[0]).x, m_Nodes.at(tetra.node[3]).y - m_Nodes.at(tetra.node[0]).y, m_Nodes.at(tetra.node[3]).z - m_Nodes.at(tetra.node[0]).z}
		});
	double volume = jacob.determinant() / 6.0;
	return volume;
}

double Mesh::GetVolumeOfVolume(const std::string name) const
{
	double result = 0;

	int physTag = m_PhisicalNames.find(name)->second.tag;

	for (auto& key_volume : m_volumes) // for every volume
	{
		bool found = false;
		for (auto& tag : key_volume.second.physicalTags)
		{
			if (tag == physTag)
				found = true;
		}

		if (found) // if it has the same physical name
		{
			auto& volume = key_volume.second;

			// fore every tetrahedron in volume
			for (size_t i = volume.firstElementIndex; i < volume.firstElementIndex + volume.numElements; i++)
			{
				result += GetVolumeOfTetrahedron(i);
			}
		}
	}

	return result;
}

int Mesh::NameToTag(const std::string& name) const
{
	int tag = 0;

	tag = m_PhisicalNames.find(name)->second.tag;

	return tag;
}

const std::string& Mesh::TagToName(int tag) const
{
	for (auto& name : m_PhisicalNames)
	{
		if (name.second.tag == tag)
		{
			return name.second.name;
		}
	}

	return "";
}

std::vector<double> Mesh::GetNodesX() const
{
	std::vector<double> v;
	v.reserve(m_Nodes.size());

	for (auto& node : m_Nodes)
	{
		v.push_back(node.x);
	}

	return v;
}

std::vector<double> Mesh::GetNodesY() const
{
	std::vector<double> v;
	v.reserve(m_Nodes.size());

	for (auto& node : m_Nodes)
	{
		v.push_back(node.y);
	}

	return v;
}

std::vector<double> Mesh::GetNodesZ() const
{
	std::vector<double> v;
	v.reserve(m_Nodes.size());

	for (auto& node : m_Nodes)
	{
		v.push_back(node.z);
	}

	return v;
}

bool Mesh::IsInBounds(double x, double y, double z) const
{
	return m_topologyTree.IsInBounds(x,y,z);
}

const std::vector<int>& Mesh::GetPhisicalTagsOfTetrahedron(size_t index) const
{
	for (auto& key_vol : m_volumes)
	{
		if (index >= key_vol.second.firstElementIndex && index < key_vol.second.firstElementIndex + key_vol.second.numElements)
		{
			return key_vol.second.physicalTags;
		}
	}
}

bool Mesh::BuildTopologyData()
{
	PROFILE_FUNCTION();
	LOG_TRACE("Building topology...");
	m_topologyTree.Clear();

	if (!m_topologyTree.GenerateData(m_volumes, m_bulkElements, m_Nodes))
	{
		LOG_ERROR("Error during topology generation; Missing geometry data!");
		return false;
	}

	return true;
}

void Mesh::Clear()
{
	m_Version = std::pair<int,int>{ 0, 0 };
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
		LOG_ERROR("Point is outside of the geometry: ({},{},{})", x, y, z);
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
	// may happen if the point is inside of the bounding box but outside of the edge tetrahedrons
	// not crititcal if the algorythm is doing OK. Critical otherwise
	LOG_ERROR("Can not find tetrahedron that contains the point: ({},{},{})", x,y,z);
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
		LOG_WARN("Unknown section in geo file: {}", s);
		return false;
	}

	return false;
}

bool Mesh::ReadMeshFormat(std::ifstream& geofile)
{
	geofile >> m_Version.first;
	geofile.get();    // get . symbol
	geofile >> m_Version.second;

	LOG_TRACE("MESH version {}.{}", m_Version.first, m_Version.second);

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

		m_PhisicalNames.emplace(pname.name, pname);

		LOG_TRACE("dim: {}, tag: {}, name: {}", pname.dimention, pname.tag, pname.name);
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

	// points
	for (int i = 0; i < numPoints; i++)
	{
		int pointTag;
		double X, Y, Z;

		geofile >> pointTag >> X >> Y >> Z;

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
		
		tmp.physicalTags.reserve(numPhysicalTags);

		for (int j = 0; j < numPhysicalTags; j++)
		{	
			int tmpPhysTag = 0;
			geofile >> tmpPhysTag;
			tmp.physicalTags.push_back(tmpPhysTag);
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

		size_t numPhysicalTags;
		geofile >> numPhysicalTags;

		tmp.physicalTags.reserve(numPhysicalTags);
		for (int j = 0; j < numPhysicalTags; j++)
		{
			int tmpPhysTag = 0;
			geofile >> tmpPhysTag;
			tmp.physicalTags.push_back(tmpPhysTag);
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

	for (int i = 0; i < numEntityBlocks; i++)
	{
		int entityDim;
		int entityTag;
		int elementType;
		size_t numElementsInBlock;

		geofile >> entityDim >> entityTag >> elementType >> numElementsInBlock;

		size_t elementTag;

		switch (entityDim)
		{
		case 2: // surface
		{
			if (elementType != 2) // 3-node Triangle
			{
				LOG_ERROR("ERROR element type {} is not supported!", elementType);
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
				LOG_ERROR("ERROR element type {} is not supported!", elementType);
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

void Mesh::FillEntitiesWithElements()
{
	for (auto& surf : m_surfaces)
	{
		// assign a span of elements to the entity
		surf.second.elements = { m_surfElements.begin() + surf.second.firstElementIndex,
								m_surfElements.begin() + surf.second.firstElementIndex + surf.second.numElements };
	}

	for (auto& vol : m_volumes)
	{
		// assign a span of elements to the entity
		vol.second.elements = { m_bulkElements.begin() + vol.second.firstElementIndex,
								m_bulkElements.begin() + vol.second.firstElementIndex + vol.second.numElements };
	}
}

void Mesh::GenerateGradientVectors()
{
	m_GradientVectors.reserve(m_bulkElements.size());

	//#pragma omp parallel for
	for (size_t i = 0; i < m_bulkElements.size(); i++ )
	{
		auto& element = m_bulkElements.at(i);

		Eigen::Vector3d p1 {m_Nodes.at(element.node[0]).x, m_Nodes.at(element.node[0]).y, m_Nodes.at(element.node[0]).z};
		Eigen::Vector3d p2 {m_Nodes.at(element.node[1]).x, m_Nodes.at(element.node[1]).y, m_Nodes.at(element.node[1]).z};
		Eigen::Vector3d p3 {m_Nodes.at(element.node[2]).x, m_Nodes.at(element.node[2]).y, m_Nodes.at(element.node[2]).z};
		Eigen::Vector3d p4 {m_Nodes.at(element.node[3]).x, m_Nodes.at(element.node[3]).y, m_Nodes.at(element.node[3]).z};

		auto grad1 = Geometry::BarycentricGradient(p1, p2, p3, p4);
		auto grad2 = Geometry::BarycentricGradient(p2, p1, p3, p4);
		auto grad3 = Geometry::BarycentricGradient(p3, p2, p1, p4);
		auto grad4 = Geometry::BarycentricGradient(p4, p2, p3, p1);

		m_GradientVectors.emplace_back();

		m_GradientVectors.at(i).at(0) = grad1;
		m_GradientVectors.at(i).at(1) = grad2;
		m_GradientVectors.at(i).at(2) = grad3;
		m_GradientVectors.at(i).at(3) = grad4;
	}
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

	if (minNodeTag != 1)
	{
		LOG_CRITICAL("Ubnormal node indexing in mesh!");
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

			if (nodeTag - prevNodeTag != 1)
			{
				LOG_ERROR("Non consecutive node numbering: {} -> {}", prevNodeTag, nodeTag);
			}
			prevNodeTag = nodeTag;
		}
		for (int j = 0; j < numNodesInBlock; j++)
		{
			Node node;
			geofile >> node.x;
			geofile >> node.y;
			geofile >> node.z;

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
