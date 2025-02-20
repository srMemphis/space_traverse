#include "SimuControl.h"

#include "debug/Log.h"
#include "debug/Profiler.h"
#include "utils/PhysConstants.h"
#include "utils/UnsignedRange.h"
#include "utils/yaml.h"

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

	if (!MeshFromFile(m_params.meshfile))
	{
		return false;
	}

	LOG_TRACE("Initializing simulation parameters...");

	// clearing out dir
	std::filesystem::remove_all(m_params.outputDir);
	std::filesystem::create_directory(m_params.outputDir);

	// saving geometry to dispaly at matlab
	ToMatlab matfile;
	matfile.OpenFile(m_params.outputDir + "/surfacegeo.mat");

	m_mesh->SaveNodesToFile(matfile);
	m_mesh->SaveSurfaceToFile(m_params.targetBoundary, matfile);
	GenerateSurfaceToVolumeIndexing(m_params.targetBoundary, m_params.targetName);

	matfile.Close();

	// assign material data
	m_solver->AddMaterial(m_params.worldName, m_params.worldMaterial);
	m_solver->AddMaterial("Detector", m_params.worldMaterial);
	m_solver->AddMaterial(m_params.targetName, m_params.targetMaterial);

	// set boundary conditions
	if (!m_solver->SetBoundaryValue(m_params.worldBoundary, 0))
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
	m_beam->SetEnergy(m_params.energy); // eV

	Eigen::Vector3d beamPos = m_params.pos;
	Eigen::Vector3d beamStare = m_params.stare;
	auto dir = (beamStare - beamPos).normalized();

	m_beam->SetCenterPosition(beamPos.x(), beamPos.y(), beamPos.z());
	m_beam->SetDirection(dir.x(), dir.y(), dir.z());
	m_beam->SetRadius(m_params.radius);
	m_beam->DetermineStepSize(0.25, m_params.numsteps);

	m_beam->GeneratePrimaryPosition(m_params.numtracks);		// 3*n^2 number of particles

	// all volumes exept Detector
	AddStoppingVolume(m_params.targetName);

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

bool SimuControl::ConfigFromFile(std::string_view configfile)
{
	auto node_res = yaml::FromFile(configfile);

	if (!node_res.second)
	{
		LOG_CRITICAL("Failed to open config file {}!", configfile);
		return false;
	}

	auto& inconfig = node_res.first;

	// ----------------------------------- general ----------------------------------- 
	if (!inconfig.contains("General"))
	{
		LOG_CRITICAL("Config file does not have 'General' section");
		return false;
	}

	// outdir
	if (!inconfig["General"].contains("outdir"))
	{
		LOG_CRITICAL("Config file does not have 'General:outdir' section");
		return false;
	}
	else if (!yaml::NodeToString(inconfig["General"]["outdir"], m_params.outputDir))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "General:outdir", "string");
		return false;
	}

	// iterations
	if (!inconfig["General"].contains("iterations"))
	{
		LOG_CRITICAL("Config file does not have 'General:iterations' section");
		return false;
	}
	else if (!yaml::NodeToInt(inconfig["General"]["iterations"], m_params.iterations))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "General:iterations", "integer");
		return false;
	}

	// deltaTime
	if (!inconfig["General"].contains("deltaTime"))
	{
		LOG_CRITICAL("Config file does not have 'General:deltaTime' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["General"]["deltaTime"], m_params.deltaTime))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "General:deltaTime", "floating");
		return false;
	}

	// saveInterval
	if (!inconfig["General"].contains("saveInterval"))
	{
		LOG_CRITICAL("Config file does not have 'General:saveInterval' section");
		return false;
	}
	else if (!yaml::NodeToInt(inconfig["General"]["saveInterval"], m_params.saveInterval))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "General:saveInterval", "integer");
		return false;
	}
	
	// ----------------------------------- mesh ----------------------------------- 
	if (!inconfig.contains("Mesh"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh' section");
		return false;
	}

	// meshfile
	if (!inconfig["Mesh"].contains("filename"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh:filename' section");
		return false;
	}
	else if (!yaml::NodeToString(inconfig["Mesh"]["filename"], m_params.meshfile))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Mesh:filename", "string");
		return false;
	}

	// target
	if (!inconfig["Mesh"].contains("Target"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh:Target' section");
		return false;
	}

	// targetName
	if (!inconfig["Mesh"]["Target"].contains("name"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh:Target:name' section");
		return false;
	}
	else if (!yaml::NodeToString(inconfig["Mesh"]["Target"]["name"], m_params.targetName))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Mesh:Target:name", "string");
		return false;
	}

	// targetMaterial
	if (!inconfig["Mesh"]["Target"].contains("material"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh:Target:material' section");
		return false;
	}
	else if (!yaml::NodeToString(inconfig["Mesh"]["Target"]["material"], m_params.targetMaterialName))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Mesh:Target:material", "string");
		return false;
	}

	// targetBoundary
	if (!inconfig["Mesh"]["Target"].contains("boundary"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh:Target:boundary' section");
		return false;
	}
	else if (!yaml::NodeToString(inconfig["Mesh"]["Target"]["boundary"], m_params.targetBoundary))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Mesh:Target:boundary", "string");
		return false;
	}

	// world
	if (!inconfig["Mesh"].contains("World"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh:World' section");
		return false;
	}

	// worldName
	if (!inconfig["Mesh"]["World"].contains("name"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh:World:name' section");
		return false;
	}
	else if (!yaml::NodeToString(inconfig["Mesh"]["World"]["name"], m_params.worldName))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Mesh:World:name", "string");
		return false;
	}

	// worldMaterial
	if (!inconfig["Mesh"]["World"].contains("material"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh:World:material' section");
		return false;
	}
	else if (!yaml::NodeToString(inconfig["Mesh"]["World"]["material"], m_params.worldMaterialName))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Mesh:World:material", "string");
		return false;
	}

	// worldBoundary
	if (!inconfig["Mesh"]["World"].contains("boundary"))
	{
		LOG_CRITICAL("Config file does not have 'Mesh:World:boundary' section");
		return false;
	}
	else if (!yaml::NodeToString(inconfig["Mesh"]["World"]["boundary"], m_params.worldBoundary))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Mesh:World:boundary", "string");
		return false;
	}

	// ----------------------------------- beam ----------------------------------- 
	if (!inconfig.contains("Beam"))
	{
		LOG_CRITICAL("Config file does not have 'Beam' section");
		return false;
	}

	// current
	if (!inconfig["Beam"].contains("current"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:current' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Beam"]["current"], m_params.current))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:current", "floating");
		return false;
	}

	// energy
	if (!inconfig["Beam"].contains("energy"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:energy' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Beam"]["energy"], m_params.energy))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:energy", "floating");
		return false;
	}

	// radius
	if (!inconfig["Beam"].contains("radius"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:radius' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Beam"]["radius"], m_params.radius))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:radius", "floating");
		return false;
	}

	// numtracks
	if (!inconfig["Beam"].contains("numtracks"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:numtracks' section");
		return false;
	}
	else if (!yaml::NodeToInt(inconfig["Beam"]["numtracks"], m_params.numtracks))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:numtracks", "integer");
		return false;
	}

	// numsteps
	if (!inconfig["Beam"].contains("numsteps"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:numsteps' section");
		return false;
	}
	else if (!yaml::NodeToInt(inconfig["Beam"]["numsteps"], m_params.numsteps))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:numsteps", "integer");
		return false;
	}

	// Pos
	if (!inconfig["Beam"].contains("Pos"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:Pos' section");
		return false;
	}

	// Pos.x
	if (!inconfig["Beam"]["Pos"].contains("x"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:Pos:x' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Beam"]["Pos"]["x"], m_params.pos.x()))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:Pos:x", "floating");
		return false;
	}

	// Pos.y
	if (!inconfig["Beam"]["Pos"].contains("y"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:Pos:y' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Beam"]["Pos"]["y"], m_params.pos.y()))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:Pos:y", "floating");
		return false;
	}

	// Pos.z
	if (!inconfig["Beam"]["Pos"].contains("z"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:Pos:z' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Beam"]["Pos"]["z"], m_params.pos.z()))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:Pos:z", "floating");
		return false;
	}

	// Stare
	if (!inconfig["Beam"].contains("Stare"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:Stare' section");
		return false;
	}

	// Stare.x
	if (!inconfig["Beam"]["Stare"].contains("x"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:Stare:x' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Beam"]["Stare"]["x"], m_params.stare.x()))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:Stare:x", "floating");
		return false;
	}

	// Stare.y
	if (!inconfig["Beam"]["Stare"].contains("y"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:Stare:y' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Beam"]["Stare"]["y"], m_params.stare.y()))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:Stare:y", "floating");
		return false;
	}

	// Stare.z
	if (!inconfig["Beam"]["Stare"].contains("z"))
	{
		LOG_CRITICAL("Config file does not have 'Beam:Stare:z' section");
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Beam"]["Stare"]["z"], m_params.stare.z()))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Beam:Stare:z", "floating");
		return false;
	}

	// ----------------------------------- solver ----------------------------------- 
	if (!inconfig.contains("Solver"))
	{
		LOG_CRITICAL("Config file does not have 'Solver' section");
		return false;
	}

	// smooth
	if (!inconfig["Solver"].contains("smooth"))
	{
		LOG_CRITICAL("Config file does not have 'Solver:smooth' section");
		return false;
	}
	else if (!yaml::NodeToBool(inconfig["Solver"]["smooth"], m_params.smooth))
	{
		LOG_CRITICAL("Config file section '{}' has wrong type. Expected {}", "Solver:smooth", "boolean");
		return false;
	}

	// Materials
	if (!inconfig["Solver"].contains("Materials"))
	{
		LOG_CRITICAL("Config file does not have 'Solver:Materials' section");
		return false;
	}

	// TargetMaterial
	if (!inconfig["Solver"]["Materials"].contains(m_params.targetMaterialName))
	{
		LOG_CRITICAL("Config file does not have 'Solver:Materials:{}' section", m_params.targetMaterialName);
		return false;
	}

	// target epsilon
	if (!inconfig["Solver"]["Materials"][m_params.targetMaterialName].contains("epsilon"))
	{
		LOG_CRITICAL("Config file does not have 'Solver:Materials:{}epsilon' section", m_params.targetMaterialName);
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Solver"]["Materials"][m_params.targetMaterialName]["epsilon"], m_params.targetMaterial.epsilon))
	{
		LOG_CRITICAL("Config file section 'Solver:Materials:{}epsilon' has wrong type. Expected {}", m_params.targetMaterialName, "double");
		return false;
	}

	// WorldMaterial
	if (!inconfig["Solver"]["Materials"].contains(m_params.worldMaterialName))
	{
		LOG_CRITICAL("Config file does not have 'Solver:Materials:{}' section", m_params.worldMaterialName);
		return false;
	}

	// target epsilon
	if (!inconfig["Solver"]["Materials"][m_params.worldMaterialName].contains("epsilon"))
	{
		LOG_CRITICAL("Config file does not have 'Solver:Materials:{}epsilon' section", m_params.worldMaterialName);
		return false;
	}
	else if (!yaml::NodeToDouble(inconfig["Solver"]["Materials"][m_params.worldMaterialName]["epsilon"], m_params.worldMaterial.epsilon))
	{
		LOG_CRITICAL("Config file section 'Solver:Materials:{}epsilon' has wrong type. Expected {}", m_params.worldMaterialName, "double");
		return false;
	}

	return true;
}

int SimuControl::Run()
{
	PROFILE_FUNCTION();
	LOG_TRACE("Running simulation...");

	// main loop
	for (size_t iter = 0; iter < m_params.iterations; iter++)
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
		if (iter % m_params.saveInterval == 0 || iter == m_params.iterations - 1)
		{
			std::string outFile = m_params.outputDir + "/solution_" + std::to_string(iter) + ".mat";
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

			if (!m_detector.ToFile(m_matlab))
			{
				LOG_ERROR("Failed to save detector data to file");
				return 3;
			}

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
	double charge = m_params.current * m_params.deltaTime / m_beam->GetTracks().size();	// charge per track

	for (auto& track : m_beam->GetTracks())
	{
		auto& lastPoint = track.GetLastPoint();
		auto elemIndex = m_mesh->FindElementByCoords(lastPoint.x(), lastPoint.y(), lastPoint.z());

		auto tag = m_mesh->GetPhisicalTagOfTetrahedron(elemIndex);

		// we hit detector
		if (tag == m_detectorTag)
		{
			double ene = phys::me * std::pow(track.GetVelocity().norm(), 2) / (2 * 1.602e-19);
			m_detector.energy.push_back(ene);
			m_detector.hitsPositionsX.push_back(lastPoint.x());
			m_detector.hitsPositionsY.push_back(lastPoint.y());
			m_detector.hitsPositionsZ.push_back(lastPoint.z());
			m_detector.current += (charge / m_params.deltaTime);
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
