#include "SimuControl.h"

#include "debug/Log.h"
#include "debug/Profiler.h"
#include "utils/PhysConstants.h"
#include "utils/UnsignedRange.h"

#include <filesystem>
#include <algorithm>
#include <string>

SimuControl::SimuControl()
{
	PROFILE_FUNCTION();

	m_mesh = std::make_shared<Mesh>();
	m_solver = std::make_shared<Solver>();
	m_beam = std::make_shared <Beam> ();
}

SimuControl::~SimuControl()
{
}

bool SimuControl::MeshFromFile(const std::string& filepath)
{
	PROFILE_FUNCTION();
	LOG_TRACE("Reading mesh...");

	bool result = m_mesh->FromFile(filepath);
	if (!result)
		return false;

	// build binary search tree
	LOG_TRACE("Building topology data...");
	m_mesh->BuildTopologyData();

	m_solver->SetMesh(m_mesh);

	m_detectorTag = m_mesh->NameToTag("Detector");

	return true;
}

bool SimuControl::Prepare()
{
	PROFILE_FUNCTION();
	LOG_TRACE("Initializing simulation parameters...");

	// general
	m_iterations = 1000;
	m_outputDir = "out";
	m_saveInterval = 50;

	// beam current and delta time
	m_deltaTime = 1e-4;		// seconds
	m_current = -100e-9;	// ampere

	// clearing out dir
	std::filesystem::remove_all(m_outputDir);
	std::filesystem::create_directory(m_outputDir);

	// saving geometry to dispaly at matlab
	ToMatlab matfile;
	matfile.OpenFile(m_outputDir + "/surfacegeo.mat");

	m_mesh->SaveSurfaceToFile("Plane Boundary", matfile);
	GenerateSurfaceToVolumeIndexing("Plane Boundary", "Plane");

	matfile.Close();

	// assign material data
	Material air, glass;
	air.epsilon = 1;
	glass.epsilon = 2;
	m_solver->AddMaterial("World", air);
	m_solver->AddMaterial("Detector", air);
	m_solver->AddMaterial("Plane", glass);

	// set boundary conditions
	if (!m_solver->SetBoundaryValue("World Boundary", 0))
	{
		LOG_ERROR("boundary surface not found");
		return false;
	}

	// initial charge value
	//double charge = -1e-7;
	//double chargeDensity = charge / m_mesh->GetVolumeOfVolume("Metall");
	//m_solver->SetInitialValue("Metall", chargeDensity);

	// assemble linear system
	m_solver->AssembleLeft();

	// set field calc type
	m_solver->SetFieldSmoothing(false);

	// beam setup
	Particle electron;
	electron.charge = phys::qe;
	electron.E0 = 0.511e6;		//eV

	m_beam->SetParticle(electron);
	m_beam->SetEnergy(1e4); // eV

	Eigen::Vector3d beamPos = { 0, 0.05, 0.15 };
	Eigen::Vector3d beamStare = { 0, 0, 0.02 };
	auto dir = (beamStare - beamPos).normalized();

	m_beam->SetCenterPosition(beamPos.x(), beamPos.y(), beamPos.z());
	m_beam->SetDirection(dir.x(), dir.y(), dir.z());
	m_beam->SetRadius(0.0025);
	m_beam->DetermineStepSize(0.25, 30000);

	m_beam->GeneratePrimaryPosition(10);		// 3*n^2 number of particles

	// all volumes exept Detector
	AddStoppingVolume("Plane");

	// set beam stopping and field functions
	m_beam->SetStoppingFunction(std::bind(&SimuControl::BeamShouldStop, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	m_beam->SetFieldFunction(std::bind(&Solver::GetField, m_solver,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	return true;
}

bool SimuControl::AddStoppingVolume(const std::string& name)
{	
	auto physTag = m_mesh->NameToTag(name);

	m_stoppingVolumes.push_back(physTag);
	
	return true;
}

int SimuControl::Run()
{
	PROFILE_FUNCTION();
	LOG_TRACE("Running simulation...");

	// main loop
	for (size_t iter = 0; iter < m_iterations; iter++)
	{
		LOG_INFO("----------- Iteration {} -----------", iter);

		LOG_TRACE("Assembling ...");
		m_solver->AssembleRight();

		// solve systems
		LOG_TRACE("Solving...");
		if (!m_solver->Solve())
		{
			LOG_ERROR("Solving failed");
			return 2;
		}

		// propagate particles in geometry
		// multithreaded inside
		LOG_TRACE("Propagating tracks...");
		m_beam->Reset();
		m_beam->Run();

		// check where we hit and act accordingly
		LOG_TRACE("Registering hits...");
		RegisterHits();

		// save solution to file
		if (iter % m_saveInterval == 0 || iter == m_iterations - 1)
		{
			std::string outFile = m_outputDir + "/solution_" + std::to_string(iter) + ".mat";
			m_matlab.OpenFile(outFile);
			
			LOG_TRACE("Saving solution to file...");
			if (!m_solver->SaveSolutionToFile(m_matlab))
			{
				LOG_ERROR("failed to save solution to file");
				return 3;
			}

			if (!m_beam->SaveTracksToFile(m_matlab))
			{
				LOG_ERROR("failed to save tracks to file");
				return 3;
			}

			if (!SaveChargeData())
			{
				LOG_ERROR("failed to save charge to file");
				return 3;
			}

			// TODO: save detector data to file 

			// closing matlab file
			// no more wrights after this
			m_matlab.Close();
		}
	}

	return 0;
}

// called in multithread
bool SimuControl::BeamShouldStop(double x, double y, double z) const
{
	// narrow boundary checking to ensure that we definetly inside of the tetrahedron
	if (!m_mesh->IsInNarrowBounds(x, y, z))
	{
		return true;
	}

	// get element index
	auto elemIndex = m_mesh->FindElementByCoords(x, y, z);

	if (elemIndex < 0 )
	{
		LOG_ERROR("Narrow boundary check failed. Point was ({}, {}, {})", x, y, z);
		return true;
	}

	// get its physical tags
	auto tag = m_mesh->GetPhisicalTagOfTetrahedron(elemIndex);

	// check if should stop on this tags
	if (std::find(m_stoppingVolumes.begin(), m_stoppingVolumes.end(), tag) != m_stoppingVolumes.end() || tag == m_detectorTag)
	{
		return true;
	}
	
	return false;
}

void SimuControl::RegisterHits()
{
	m_detector.Clear();
	double charge = m_current * m_deltaTime / m_beam->GetTracks().size();	// charge per track
	for (auto& track : m_beam->GetTracks())
	{
		auto& lastPoint = track.GetLastPoint();
		auto elemIndex = m_mesh->FindElementByCoords(lastPoint.x(), lastPoint.y(), lastPoint.z());

		auto tag = m_mesh->GetPhisicalTagOfTetrahedron(elemIndex);

		// we hit detector
		if (tag == m_detectorTag)
		{
			// TODO: calc particle energy
			m_detector.hitsPositions.push_back(lastPoint);
			m_detector.current += (charge / m_deltaTime);
			m_detector.energy.push_back(0);
		}
		else if (std::find(m_stoppingVolumes.begin(), m_stoppingVolumes.end(), tag) != m_stoppingVolumes.end())
		{
			// TODO: generate secondary emission

			// we hit smth that needs to be charged
			m_solver->AddElementCharge(elemIndex, charge);
		}
	}
}

bool SimuControl::GenerateSurfaceToVolumeIndexing(const std::string& physSurface, const std::string& physVolume)
{
	// slow and naive
	PROFILE_FUNCTION();

	auto surfTag = m_mesh->NameToTag(physSurface);
	if (surfTag == 0)
	{
		LOG_CRITICAL("Cant find surface: {}", physSurface);
		return false;
	}

	auto volTag = m_mesh->NameToTag(physVolume);
	if (volTag == 0)
	{
		LOG_CRITICAL("Cant find volume: {}", physVolume);
		return false;
	}

	// get surface elemetns
	std::vector<ETriangle> triangles;
	for (const auto& key_surf : m_mesh->GetSurfaces())
	{
		if (key_surf.second.physicalTag == surfTag)
		{
			for (const auto& elem : key_surf.second.elements)
			{
				triangles.push_back(elem);
			}
		}
	}

	std::vector<size_t> indexForSaving;
	indexForSaving.resize(triangles.size());

	// true if tetrahedron contains the triangle
	auto hasTriang = [](const ETetrahedron& tet, const ETriangle& tri)
		{
			int count = 0;
			for (int i = 0; i < 3; i++)		// tri node
			{
				for (int j = 0; j < 4; j++)	// tet node
				{
					if (tri.node[i] == tet.node[j])
					{
						count++;
						break;	// jump to the next triangle node
					}
				}
			}

			if (count == 3)
				return true;

			return false;
		};

	// for every volume
	for (const auto& key_vol : m_mesh->GetVolumes())
	{
		// if volume has this physical tag
		if (key_vol.second.physicalTag == volTag)
		{
			// for every tet index in this volume
			for (size_t tetIndex = key_vol.second.firstElementIndex; tetIndex < key_vol.second.firstElementIndex + key_vol.second.numElements; tetIndex++)
			{
				const auto& tet = m_mesh->GetTetrahedron(tetIndex);
				
				// for every triangle
				for (size_t triIndex = 0; triIndex < triangles.size(); triIndex++)
				{
					// is tet has the triangle
					if (hasTriang(tet, triangles[triIndex]))
					{
						indexForSaving.at(triIndex) = tetIndex;
						break;	// jump to the next tetrahedron
					}
				}
			}
		}
	}

	m_chargeIndexToSave.emplace(physSurface, indexForSaving);

	return true;
}

bool SimuControl::SaveChargeData()
{
	std::vector<double> charge;

	for (auto& name_indices : m_chargeIndexToSave)
	{
		charge.reserve(name_indices.second.size());
		charge.clear();

		for (auto& index : name_indices.second)
		{
			charge.push_back(m_solver->GetChargeDensAtElem(index));
		}

		// removing spaces from name
		std::string newname = name_indices.first;
		auto newend = std::remove(newname.begin(), newname.end(), ' ');
		newname.erase(newend, newname.end());
		newname.append("_C");

		if (!m_matlab.Write(charge, newname))
		{
			return false;
		}
	}

	return true;
}
