#pragma once
#include "ElementTypes.h"
#include "TopologyTree.h"

#include <string>
#include <vector>
#include <map>

class Mesh
{
public:
	Mesh();
	~Mesh();

	bool FromFile(std::string filepath);

	std::vector<ETriangle>& GetSurfElements();
	std::vector<ETetrahedron>& GetBulkElements();

	bool BuildTopologyData();
	void Clear();

	// find element by traversing topology tree
	// returns element index
	int64_t FindElementByCoords(double x, double y, double z) const;

private:
	std::map<std::pair<int, int>, PhisicalName> m_PhisicalNames;  // mapped by dimention+tag

	std::vector<Node> m_Nodes;                 // points
	std::vector<ETriangle> m_surfElements;     // triangles
	std::vector<ETetrahedron> m_bulkElements;  // tetrahedrons

	std::map<int, Surface> m_surfaces;         // mapped by entity tags
	std::map<int, Volume> m_volumes;

	std::pair<int, int> m_Version;

	TopologyTree m_topologyTree;

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

	friend class Solver;
};
