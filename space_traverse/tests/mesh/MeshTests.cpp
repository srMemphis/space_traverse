#include "mesh/Mesh.h"
#include "support/approx.h"
#include "support/fixture.h"

#include <doctest/doctest.h>
#include <Eigen/Dense>

#include <cstdint>
#include <stdexcept>

namespace
{
	// tests/data/cube.msh: the cube [1, 2]^3 as one Volume "Cube" (tag 1) with the faces "Bottom" (z = 1, tag 2)
	// and "Top" (z = 2, tag 3). The counts below are read off the file.
	constexpr int kCubeTag = 1;
	constexpr int kBottomTag = 2;
	constexpr int kTopTag = 3;
	constexpr size_t kNodeCount = 14;
	constexpr size_t kElementCount = 24;
	constexpr size_t kTriangleCount = 8;
	constexpr size_t kSurfaceCount = 6;

	struct CubeMesh
	{
		Mesh mesh;

		CubeMesh()
		{
			REQUIRE(mesh.FromFile(FixturePath("cube.msh").string()));
			REQUIRE(mesh.BuildTopologyData());
		}
	};

	Eigen::Vector3d NodePosition(const Mesh& mesh, size_t index)
	{
		const auto& node = mesh.GetNode(index);
		return {node.x, node.y, node.z};
	}

	// Barycentric coordinates of p in the Element, from a linear solve independent of the lookup's face tests.
	Eigen::Vector4d Barycentric(const Mesh& mesh, size_t element, const Eigen::Vector3d& p)
	{
		const auto& tetra = mesh.GetTetrahedron(element);
		const Eigen::Vector3d p0 = NodePosition(mesh, tetra.node[0]);

		Eigen::Matrix3d edges;
		for (int i = 0; i < 3; i++)
			edges.col(i) = NodePosition(mesh, tetra.node[i + 1]) - p0;

		const Eigen::Vector3d l = edges.partialPivLu().solve(p - p0);
		return {1.0 - l.sum(), l.x(), l.y(), l.z()};
	}
} // namespace

TEST_SUITE("Mesh")
{
	TEST_CASE_FIXTURE(CubeMesh, "the mesh holds every node, Element and face of the file")
	{
		CHECK(mesh.GetNodes().size() == kNodeCount);
		CHECK(mesh.GetBulkElements().size() == kElementCount);
		CHECK(mesh.GetSurfElements().size() == kTriangleCount);
		CHECK(mesh.GetVolumes().size() == 1);
		CHECK(mesh.GetSurfaces().size() == kSurfaceCount);
	}

	TEST_CASE_FIXTURE(CubeMesh, "each name in the mesh maps to its tag and back")
	{
		CHECK(mesh.NameToTag("Cube") == kCubeTag);
		CHECK(mesh.NameToTag("Bottom") == kBottomTag);
		CHECK(mesh.NameToTag("Top") == kTopTag);

		CHECK(mesh.TagToName(kCubeTag) == "Cube");
		CHECK(mesh.TagToName(kBottomTag) == "Bottom");
		CHECK(mesh.TagToName(kTopTag) == "Top");
	}

	TEST_CASE_FIXTURE(CubeMesh, "a name missing from the mesh has tag 0")
	{
		CHECK(mesh.NameToTag("Target") == 0);
	}

	TEST_CASE_FIXTURE(CubeMesh, "a tag missing from the mesh has an empty name")
	{
		CHECK(mesh.TagToName(99).empty());
	}

	TEST_CASE_FIXTURE(CubeMesh, "the cube Volume measures one cubic metre")
	{
		CHECK_NEAR(mesh.GetVolumeOfVolume("Cube"), 1.0);
	}

	TEST_CASE_FIXTURE(CubeMesh, "the Elements of the cube are positively oriented and add up to one cubic metre")
	{
		double total = 0;
		for (size_t i = 0; i < mesh.GetBulkElements().size(); i++)
		{
			INFO("Element ", i);
			const double size = mesh.GetVolumeOfTetrahedron(i);
			CHECK(size > 0);
			total += size;
		}

		CHECK_NEAR(total, 1.0);
	}

	TEST_CASE_FIXTURE(CubeMesh, "measuring a Volume missing from the mesh is an error")
	{
		CHECK_THROWS_AS(mesh.GetVolumeOfVolume("Target"), std::runtime_error);
	}

	TEST_CASE_FIXTURE(CubeMesh, "every Element of the cube carries the cube's tag")
	{
		for (size_t i = 0; i < mesh.GetBulkElements().size(); i++)
		{
			INFO("Element ", i);
			CHECK(mesh.GetPhysicalTagOfTetrahedron(i) == kCubeTag);
		}
	}

	TEST_CASE_FIXTURE(CubeMesh, "an index past every Volume has no tag")
	{
		CHECK(mesh.GetPhysicalTagOfTetrahedron(kElementCount) == -1);
	}

	TEST_CASE_FIXTURE(CubeMesh, "a point inside the cube is found in an Element that contains it")
	{
		const Eigen::Vector3d p{1.23, 1.37, 1.61};

		const int64_t element = mesh.FindElementByCoords(p.x(), p.y(), p.z());

		REQUIRE(element >= 0);
		REQUIRE(element < static_cast<int64_t>(kElementCount));
		CHECK(Barycentric(mesh, static_cast<size_t>(element), p).minCoeff() > 0);
		CHECK(mesh.GetPhysicalTagOfTetrahedron(static_cast<size_t>(element)) == kCubeTag);
	}

	TEST_CASE_FIXTURE(CubeMesh, "a point outside the cube is not found")
	{
		Eigen::Vector3d p;
		SUBCASE("beyond the far corner")
		{
			p = {3.0, 3.0, 3.0};
		}
		SUBCASE("above the top")
		{
			p = {1.5, 1.5, 2.5};
		}
		SUBCASE("at the origin")
		{
			p = {0.0, 0.0, 0.0};
		}

		CHECK(mesh.FindElementByCoords(p.x(), p.y(), p.z()) < 0);
	}

	TEST_CASE_FIXTURE(CubeMesh, "a cleared mesh knows no names")
	{
		mesh.Clear();

		CHECK(mesh.NameToTag("Cube") == 0);
		CHECK(mesh.TagToName(kCubeTag).empty());
	}

	TEST_CASE_FIXTURE(CubeMesh, "loading the same file again gives the same mesh")
	{
		REQUIRE(mesh.FromFile(FixturePath("cube.msh").string()));

		CHECK(mesh.GetNodes().size() == kNodeCount);
		CHECK(mesh.GetBulkElements().size() == kElementCount);
		CHECK(mesh.GetSurfElements().size() == kTriangleCount);
		CHECK(mesh.GetVolumes().size() == 1);
		CHECK(mesh.GetSurfaces().size() == kSurfaceCount);
		CHECK(mesh.NameToTag("Cube") == kCubeTag);
		CHECK_NEAR(mesh.GetVolumeOfVolume("Cube"), 1.0);
	}

	// Known bug: https://github.com/srMemphis/space_traverse/issues/36
	// Now: the mesh bounds grow from the origin over the Gmsh entity boxes, [0, 2.0000001]^3. Correct: [1, 2]^3.
	TEST_CASE_FIXTURE(CubeMesh, "the mesh bounds are the cube" * doctest::expected_failures(2))
	{
		const Box box = mesh.GetBoundingBox();

		CHECK_VEC_NEAR(Eigen::Vector3d(box.xMin, box.yMin, box.zMin), Eigen::Vector3d(1.0, 1.0, 1.0));
		CHECK_VEC_NEAR(Eigen::Vector3d(box.xMax, box.yMax, box.zMax), Eigen::Vector3d(2.0, 2.0, 2.0));
	}

	TEST_CASE_FIXTURE(CubeMesh, "the origin lies outside the mesh bounds")
	{
		CHECK_FALSE(mesh.IsInBounds(0.0, 0.0, 0.0));
	}

	TEST_CASE_FIXTURE(CubeMesh, "the corners of the cube lie in the mesh bounds")
	{
		CHECK(mesh.IsInBounds(1.0, 1.0, 1.0));
		CHECK(mesh.IsInBounds(2.0, 2.0, 2.0));
	}

	// The narrow bounds of the cube are [1.0005, 1.9995]^3: the cube shrunk by 0.1% about its centre.
	TEST_CASE_FIXTURE(CubeMesh, "the narrow bounds keep a thin margin inside every face of the cube")
	{
		Eigen::Vector3d p;
		SUBCASE("near the low corner")
		{
			p = {1.0001, 1.0001, 1.0001};
		}
		SUBCASE("near the high corner")
		{
			p = {1.9999, 1.9999, 1.9999};
		}
		SUBCASE("outside the cube")
		{
			p = {0.999, 1.5, 1.5};
		}

		CHECK_FALSE(mesh.IsInNarrowBounds(p.x(), p.y(), p.z()));
	}

	TEST_CASE_FIXTURE(CubeMesh, "the narrow bounds cover the cube except that margin")
	{
		Eigen::Vector3d p;
		SUBCASE("the centre")
		{
			p = {1.5, 1.5, 1.5};
		}
		SUBCASE("near the low corner")
		{
			p = {1.001, 1.001, 1.001};
		}
		SUBCASE("near the high corner")
		{
			p = {1.999, 1.999, 1.999};
		}

		CHECK(mesh.IsInNarrowBounds(p.x(), p.y(), p.z()));
	}

	// Known bug: https://github.com/srMemphis/space_traverse/issues/36
	// Now: a point on a face shared by two Elements is in neither (-2). Correct: it is found in one of them.
	TEST_CASE_FIXTURE(CubeMesh, "a point on a face shared by two Elements is found" * doctest::expected_failures(1))
	{
		// The triangle (1.5, 1, 1.5), (1.5, 2, 1.5), (1.5, 1.5, 1) in the plane x = 1.5 is a face of two Elements;
		// every coordinate is exact in binary, so the point lies exactly on it.
		CHECK(mesh.FindElementByCoords(1.5, 1.5, 1.25) >= 0);
	}
}
