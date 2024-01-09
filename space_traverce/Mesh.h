#pragma once
#include "ElementTypes.h"
#include "Box.h"

#include <string>
#include <vector>
#include <map>


struct Entity
{
	int physicalTag;
	int dimention;
};

class Mesh
{

public:

	Mesh();
	~Mesh();

	bool FromFile(std::string filepath);

	std::vector<ETriangle>& GetSurfElements();
	std::vector<ETetrahedron>& GetBulkElements();
	void BuildTopologyData();

private:

	std::map<std::pair<int, int>, PhisicalName> m_PhisicalNames;
	std::pair<int, int> m_Version;
	std::vector<Node> m_Nodes;

	std::vector<ETriangle> m_surfElements;
	std::vector<ETetrahedron> m_bulkElements;
	std::map<int, Entity> m_Entities;

	// topology data
	struct treeNode
	{
		std::vector<size_t> elements;
		Box boundBox;
		size_t left = 0;
		size_t right = 0;
	};
	
	std::vector<treeNode> m_topologyTree;

private:
	bool ReadSection(std::ifstream& geofile, std::string section);
	bool ReadMeshFormat(std::ifstream& geofile);
	bool ReadPhysicalNames(std::ifstream& geofile);
	bool ReadEntities(std::ifstream& geofile);
	bool ReadPartitionedEntities(std::ifstream& geofile);
	bool ReadNodes(std::ifstream& geofile);
	bool ReadElements(std::ifstream& geofile);
	bool ReadPeriodic(std::ifstream& geofile);
	bool ReadGhostElements(std::ifstream& geofile);
	bool ReadParametrizations(std::ifstream& geofile);
	bool ReadNodeData(std::ifstream& geofile);
	bool ReadElementData(std::ifstream& geofile);
	bool ReadElementNodeData(std::ifstream& geofile);
	bool ReadInterpolationScheme(std::ifstream& geofile);

	void BuildTopologyRecursive(size_t parentID);
	friend class Solver;
};
