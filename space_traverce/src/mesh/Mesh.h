#pragma once
#include "mesh/ElementTypes.h"
#include "mesh/TopologyTree.h"
#include "utils/ToMatlab.h"

#include <string>
#include <vector>
#include <map>

#include <Eigen/Dense>

class Mesh
{
public:
	Mesh();
	~Mesh();

	// modifiers
	bool FromFile(std::string filepath); 	// initialize mesh data from file
	bool BuildTopologyData();
	void Clear();

	// getters
	const std::vector<ETriangle>& GetSurfElements() const;
	const std::vector<ETetrahedron>& GetBulkElements() const;
	const std::vector<Node>& GetNodes() const;
	const std::map<int, Volume>& GetVolumes() const;
	const std::map<int, Surface>& GetSurfaces() const;
	int GetPhisicalTagOfTetrahedron(size_t index) const;

	const ETriangle& GetTriangle(size_t index) const;
	const ETetrahedron& GetTetrahedron(size_t index) const;
	const Node& GetNode(size_t index) const;

	// stats
	uint32_t MaxTetraAtNode() const;
	double GetVolumeOfTetrahedron(size_t index) const;
	double GetVolumeOfVolume(const std::string name) const;
	
	// utils
	int NameToTag(const std::string& name) const;	// physical name to tag
	const std::string& TagToName(int tag) const;	// physical tag to name
	std::vector<double> GetNodesX() const;			// creates vector of node coordinates
	std::vector<double> GetNodesY() const;
	std::vector<double> GetNodesZ() const;
	bool IsInBounds(double x, double y, double z) const;						// checks if point is inside biggest bounding box 
	

	// find element by traversing topology tree
	// returns element index
	int64_t FindElementByCoords(double x, double y, double z) const;

	void SaveSurfaceToFile(const std::string& physname, ToMatlab& file);

private:
	std::map<std::string, PhisicalName> m_PhisicalNames;  // mapped by name

	std::vector<Node> m_Nodes;                 // points
	std::vector<ETriangle> m_surfElements;     // triangles
	std::vector<ETetrahedron> m_bulkElements;  // tetrahedrons

	std::map<int, Surface> m_surfaces;         // mapped by entity tags
	std::map<int, Volume> m_volumes;

	std::pair<int, int> m_Version;				// gmsh file version

	TopologyTree m_topologyTree;				// binary search tree
	std::vector<std::array<Eigen::Vector3d, 4>> m_GradientVectors;        // four gradient vectors per element

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

	// after file is read
	void FillEntitiesWithElements();
	void GenerateGradientVectors();

	friend class Solver;
};
