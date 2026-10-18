#include "Beam.h"

#include "debug/Log.h"
#include "debug/Profiler.h"
#include "utils/PhysConstants.h"

#include <Eigen/Geometry>

#include <cmath>
#include <random>


Beam::Beam() : m_particle({1.60217633e-19, 0.511e6}), m_radius(1)
{
}

Beam::~Beam()
{
}

const std::vector<Track>& Beam::GetTracks() const
{
	return m_tracks;
}

void Beam::SetRadius(double rad)
{
	m_radius = rad;
}

void Beam::SetCenterPosition(double x, double y, double z)
{
	m_centerPosition(0) = x;
	m_centerPosition(1) = y;
	m_centerPosition(2) = z;
}

void Beam::SetDirection(double x, double y, double z)
{
	m_direction(0) = x;
	m_direction(1) = y;
	m_direction(2) = z;

	m_direction.normalize();
}

void Beam::GeneratePrimaryPosition(uint32_t param)
{
	PROFILE_FUNCTION();

	// for now this is the only option
	GenerateUniformDisk(param);
}

void Beam::SetParticle(Particle p)
{
	m_particle = p;
}

void Beam::SetEnergy(double ene)
{
	m_energy = ene;
}

void Beam::DetermineStepSize(double environmentSize, uint32_t desiredStepCount)
{
	m_stepSize = environmentSize / desiredStepCount;
}

void Beam::SetDivergence(double degrees)
{
	m_divergence = (degrees * 3.1415 / 180) / 2; // half of the angle in radians
}

void Beam::SetStoppingFunction(std::function<bool(double, double, double)> func)
{
	m_stoppingFunction = func;
}

void Beam::SetFieldFunction(std::function<Eigen::Vector3d(double x, double y, double z)> func)
{
	m_fieldFunction = func;
}

void Beam::Reset()
{
	for (auto& track : m_tracks)
	{
		track.ResetToStartingPosition();
	}
}

bool Beam::SaveTracksToFile(ToMatlab& file, uint32_t numPoints)
{
	file.StartCell("tracks", m_tracks.size());

	std::vector<double> coords;
	coords.reserve(numPoints);

	for (auto& track : m_tracks)
	{
		auto& points = track.GetPoints();
		auto pointsSize = points.size();
		auto coordsSize = pointsSize;

		double step = 1.0;
		if (pointsSize > numPoints)
		{
			step = (double)pointsSize / (double)(numPoints - 1);
			coordsSize = numPoints;
		}

		// reserve for x y and z
		coords.resize(coordsSize * 3);

		// writing column major matrix as vector

		// writing the first point
		coords.at(coordsSize * 0) = points.at(0).x();
		coords.at(coordsSize * 1) = points.at(0).y();
		coords.at(coordsSize * 2) = points.at(0).z();

		// writing all between the first and the second
		for (size_t i = 1; i < coordsSize - 1; i++)
		{
			coords.at(i + coordsSize * 0) = points.at(i * step).x();
			coords.at(i + coordsSize * 1) = points.at(i * step).y();
			coords.at(i + coordsSize * 2) = points.at(i * step).z();
		}

		// writing the last point
		coords.at(coordsSize - 1 + coordsSize * 0) = points.at(pointsSize - 1).x();
		coords.at(coordsSize - 1 + coordsSize * 1) = points.at(pointsSize - 1).y();
		coords.at(coordsSize - 1 + coordsSize * 2) = points.at(pointsSize - 1).z();

		file.WriteCellAsMatrix(coords, coordsSize, 3);

		coords.clear();
	}

	file.EndCell();

	return true;
}

bool Beam::SaveTracksToFilePresice(ToMatlab& file)
{
	file.StartCell("tracks", m_tracks.size());

	std::vector<double> coords;

	for (auto& track : m_tracks)
	{
		auto& points = track.GetPoints();
		auto size = points.size();

		// reserve for x y and z
		coords.resize(size * 3);

		// writing column major matrix as vector
		for (size_t i = 0; i < size; i++)
		{
			coords.at(i + size * 0) = points.at(i).x();
			coords.at(i + size * 1) = points.at(i).y();
			coords.at(i + size * 2) = points.at(i).z();
		}

		file.WriteCellAsMatrix(coords, size, 3);

		coords.clear();
	}

	file.EndCell();

	return true;
}

void Beam::GenerateUniformDisk(uint32_t circles)
{
	// Isocell method
	int N1 = 3;
	int n = circles;

	int numtracks = N1 * n * n;

	double Rfacke = 2 * n * m_radius / (2 * n - 1);
	double dr = Rfacke / n;
	double A1 = phys::pi * dr * dr / N1;

	auto centermass = [](const double R, const double phi) { return 4.0 / 3.0 * R * std::sin(phi / 2) / phi; };

	auto area = [](const double R, const double phi) { return R * R * phi / 2; };

	std::vector<double> x;
	std::vector<double> y;
	std::vector<double> z;

	x.reserve(numtracks);
	y.reserve(numtracks);
	z.resize(numtracks); // zero initialized

	for (int i = 1; i < n + 1; i++)
	{
		int N = std::round(phys::pi * dr * dr * (2 * i - 1) / A1);
		double dphi = 2 * phys::pi / N;

		double Rc = centermass(dr * i, dphi);
		double rc = centermass(dr * (i - 1), dphi);

		double AR = area(dr * i, dphi);
		double Ar = area(dr * (i - 1), dphi);

		double Rtotal = (AR * Rc - Ar * rc) / (AR - Ar); // radius to centermass of a sector

		for (int Ni = 1; Ni < N + 1; Ni++)
		{
			x.push_back(Rtotal * std::cos(dphi * Ni));
			y.push_back(Rtotal * std::sin(dphi * Ni));
		}
	}

	// populating tracks with initial values
	m_tracks.clear();
	m_tracks.reserve(numtracks);

	// rotate point to designated direction
	auto rotation = Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d{0, 0, 1}, m_direction);

	for (int i = 0; i < numtracks; i++)
	{
		Track t;
		t.SetStartingPosition(rotation * Eigen::Vector3d{x.at(i), y.at(i), z.at(i)} + m_centerPosition);
		m_tracks.push_back(t);
	}
}

void Beam::Run()
{
	PROFILE_FUNCTION();

	// initial speed value
	double speed = phys::c * std::sqrt(1 - std::pow(m_particle.E0 / (m_energy + m_particle.E0), 2));

	std::random_device rd;  // Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<> div(0, m_divergence);
	std::uniform_real_distribution<> fi(0, 2 * 3.1415);

	// perpendicular to direction
	Eigen::Vector3d randomVector = Eigen::Vector3d::Random();
	Eigen::Vector3d perp = m_direction.cross(randomVector).normalized();

	// setting initial values
	for (int i = 0; i < m_tracks.size(); i++)
	{
		auto& track = m_tracks.at(i);
		track.SetStepSize(m_stepSize);

		// generate divergence from direction
		Eigen::Vector3d dirdiv = Eigen::AngleAxis<double>(fi(gen), m_direction) * perp;
		dirdiv = dirdiv * std::tan(div(gen));
		Eigen::Vector3d newdir = (m_direction + dirdiv).normalized();
		track.SetStartingVelocity(newdir * speed);

		auto field = m_fieldFunction(track.GetLastPoint().x(), track.GetLastPoint().y(), track.GetLastPoint().z());
		track.SetAcceleration(field * m_particle.charge / phys::me);
	}

#pragma omp parallel for schedule(dynamic)
	for (int i = 0; i < m_tracks.size(); i++)
	{
		PROFILE_SCOPE("ONE TRACK RUN");

		auto& track = m_tracks.at(i);
		while (true)
		{
			auto& lastPoint = track.MakeStep();

			// if we should stop
			if (m_stoppingFunction(lastPoint.x(), lastPoint.y(), lastPoint.z()))
			{
				break;
			}

			auto field = m_fieldFunction(lastPoint.x(), lastPoint.y(), lastPoint.z());
			track.SetAcceleration(field * m_particle.charge / phys::me);

			track.CalcNewVelocity();
		}
	}
}
