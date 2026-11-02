#include "mesh/Mesh.h"
#include "solver/Solver.h"
#include "support/approx.h"
#include "support/fixture.h"
#include "utils/ToMatlab.h"

#include <doctest/doctest.h>
#include <Eigen/Dense>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
	// The analytic case on tests/data/cube.msh, the cube [1, 2]^3: no charge, epsilon = 1, 0 V on "Bottom" (z = 1)
	// and 1 V on "Top" (z = 2). The Potential is u = z - 1 and the Field is (0, 0, -1) V/m. Linear elements
	// reproduce a linear Potential exactly, and the zero-flux condition on the four side faces holds for it too.
	const Eigen::Vector3d kExactField(0.0, 0.0, -1.0);

	// Generic interior points: every point with a coordinate of 1.5 lies on an internal face of the fixture.
	const Eigen::Vector3d kInteriorPoints[] = {
		{1.23, 1.37, 1.61},
		{1.71, 1.18, 1.29},
		{1.42, 1.83, 1.77},
		{1.88, 1.62, 1.12},
	};

	struct CubeSolver
	{
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
		Solver solver;

		explicit CubeSolver(const std::string& materialName = "Cube")
		{
			REQUIRE(mesh->FromFile(FixturePath("cube.msh").string()));
			REQUIRE(mesh->BuildTopologyData());

			solver.SetMesh(mesh);
			solver.AddMaterial(materialName, Material{1.0});
			REQUIRE(solver.SetBoundaryValues({"Bottom", "Top"}, {0.0, 1.0}));
		}

		void SolveAnalyticCase()
		{
			solver.AssembleLeft();
			solver.AssembleRight();
			REQUIRE(solver.Solve());
		}
	};

	// Reads the real double array `name` from an uncompressed MAT 5 file, the format TinyMAT writes.
	std::vector<double> ReadMatArray(const std::filesystem::path& path, const std::string& name)
	{
		std::ifstream file(path, std::ios::binary);
		const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		const auto readU32 = [&](size_t at)
		{
			uint32_t value = 0;
			REQUIRE(at + sizeof(value) <= bytes.size());
			std::memcpy(&value, bytes.data() + at, sizeof(value));
			return value;
		};

		constexpr size_t kHeaderSize = 128;
		constexpr uint32_t kInt8 = 1, kDouble = 9, kMatrix = 14;

		for (size_t at = kHeaderSize; at + 8 <= bytes.size();)
		{
			const uint32_t type = readU32(at);
			const uint32_t size = readU32(at + 4);
			const size_t end = at + 8 + size;

			if (type == kMatrix)
			{
				std::string arrayName;
				std::vector<double> values;

				// Sub-elements: array flags, dimensions, name, real part. A small one (at most 4 bytes of data)
				// packs its size into the upper half of the type word and takes 8 bytes in all.
				for (size_t sub = at + 8; sub < end;)
				{
					uint32_t subType = readU32(sub);
					uint32_t subSize = 0;
					size_t data = 0;
					size_t next = 0;
					if (subType >> 16)
					{
						subSize = subType >> 16;
						subType &= 0xFFFF;
						data = sub + 4;
						next = sub + 8;
					}
					else
					{
						subSize = readU32(sub + 4);
						data = sub + 8;
						next = data + (subSize + 7) / 8 * 8;
					}

					if (subType == kInt8)
						arrayName.assign(bytes.data() + data, subSize);
					else if (subType == kDouble)
					{
						values.resize(subSize / sizeof(double));
						std::memcpy(values.data(), bytes.data() + data, subSize);
					}
					sub = next;
				}

				if (arrayName == name)
					return values;
			}

			at = end;
		}

		FAIL("no array named ", name, " in ", path.string());
		return {};
	}

	// The Potential at every node, read back from the file the Solver writes.
	Eigen::VectorXd SavedPotential(Solver& solver)
	{
		const auto path = std::filesystem::temp_directory_path() / "space_traverse_solver_tests.mat";
		{
			ToMatlab file;
			REQUIRE(file.OpenFile(path));
			REQUIRE(solver.SaveSolutionToFile(file));
		}

		const auto values = ReadMatArray(path, "u");
		std::filesystem::remove(path);
		return Eigen::Map<const Eigen::VectorXd>(values.data(), static_cast<Eigen::Index>(values.size()));
	}
} // namespace

TEST_SUITE("Solver")
{
	TEST_CASE_FIXTURE(CubeSolver, "the potential at every node rises linearly from bottom to top")
	{
		SolveAnalyticCase();

		const auto& nodes = mesh->GetNodes();
		Eigen::VectorXd expected(static_cast<Eigen::Index>(nodes.size()));
		for (size_t i = 0; i < nodes.size(); i++)
			expected(static_cast<Eigen::Index>(i)) = nodes[i].z - 1.0;

		CHECK_VEC_NEAR(SavedPotential(solver), expected);
	}

	TEST_CASE_FIXTURE(CubeSolver, "the field inside the cube is uniform and points from top to bottom")
	{
		SolveAnalyticCase();

		for (const auto& p : kInteriorPoints)
		{
			CAPTURE(p.transpose());
			CHECK_VEC_NEAR(solver.GetField(p.x(), p.y(), p.z()), kExactField);
		}
	}

	TEST_CASE_FIXTURE(CubeSolver, "the field outside the mesh is zero")
	{
		SolveAnalyticCase();

		// The Field inside is 1 V/m, so 1e-9 V/m is the default relative tolerance at that scale.
		CHECK_VEC_NEAR_ABS(solver.GetField(3.0, 3.0, 3.0), Eigen::Vector3d::Zero(), 1e-9);
	}

	TEST_CASE("a volume without a material stops the assembly with an error")
	{
		CubeSolver cube("Glass");

		CHECK_THROWS_AS(cube.solver.AssembleLeft(), std::runtime_error);
	}
}
