#include "Beam.h"
#include <cmath>

#include <Eigen/Geometry>

#include "utils/PhysConstants.h"
#include "debug/Profiler.h"
#include "debug/Log.h"

Beam::Beam()
	:m_particle({1.60217633e-19, 9.1093837e-31}), m_radius(1)
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

void Beam::GeneratePrimaryPosition()
{
	PROFILE_FUNCTION();

	// for now this is the only option
	GenerateUniformDisk(4);
}

void Beam::SetParticle(Particle p)
{
	m_particle = p;
}

void Beam::SetEnergy(double ene)
{
	m_energy = ene;
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

bool Beam::SaveTracksToFile(ToMatlab& file)
{
	file.StartCell("tracks", m_tracks.size());

	std::vector<double> coords;

	for (auto& track : m_tracks)
	{
		auto& points = track.GetPoints();
		auto size = points.size();

		// reserve for x y and z
		coords.resize(size*3);
		
		// writing column major matrix as vector
		for (size_t i = 0; i < size; i++)
		{
			coords.at(i + size*0) = points.at(i).x();
			coords.at(i + size*1) = points.at(i).y();
			coords.at(i + size*2) = points.at(i).z();
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

	auto centermass = [](const double R, const double phi)
		{
			return 4.0 / 3.0 * R * std::sin(phi / 2) / phi;
		};

	auto area = [](const double R, const double phi)
		{
			return R * R * phi / 2;
		};

	std::vector<double> x;
	std::vector<double> y;
	std::vector<double> z;

	x.reserve(numtracks);
	y.reserve(numtracks);
	z.resize(numtracks);		// zero initialized

	for (int i = 1; i < n + 1; i++)
	{
		int N = std::round(phys::pi * dr * dr * (2 * i - 1) / A1);
		double dphi = 2 * phys::pi / N;

		double Rc = centermass(dr * i, dphi);
		double rc = centermass(dr * (i - 1), dphi);

		double AR = area(dr * i, dphi);
		double Ar = area(dr * (i - 1), dphi);

		double Rtotal = (AR * Rc - Ar * rc) / (AR - Ar);	// radius to centermass of a sector

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
	auto rotation = Eigen::Quaterniond::FromTwoVectors(Eigen::Vector3d{0,0,1}, m_direction);

	for (int i = 0; i < numtracks; i++)
	{
		Track t;
		t.SetStartingPosition(rotation * Eigen::Vector3d{ x.at(i), y.at(i), z.at(i) } + m_centerPosition);
		m_tracks.push_back(t);
	}

}

void Beam::Run()
{
	PROFILE_FUNCTION();

	// initial speed value
	double speed = phys::c * std::sqrt(1 - 1 / std::pow((m_energy + m_particle.E0) / m_particle.E0, 2));

	// using int based loop fop future openmp support

	// setting initial values
	for (int i = 0; i < m_tracks.size(); i++)
	{
		
		auto& track = m_tracks.at(i);

		// TODO: create api to set this from outside. 
		track.SetStepSize(0.001);

		track.SetStartingVelocity(m_direction * speed);
		auto field = m_fieldFunction(track.GetLastPoint().x(), track.GetLastPoint().y(), track.GetLastPoint().z());
		track.SetAcceleration(field * m_particle.charge);
	}


	#pragma omp parallel for num_threads(3)
	for (int i = 0; i < m_tracks.size(); i++)
	{
		PROFILE_SCOPE("ONE TRACK RUN");

		auto& track = m_tracks.at(i);
		while (true)
		{
			track.MakeStep();

			// if we should stop
			if (m_stoppingFunction(track.GetLastPoint().x(), track.GetLastPoint().y(), track.GetLastPoint().z()))
			{
				break;
			}
			
			auto field = m_fieldFunction(track.GetLastPoint().x(), track.GetLastPoint().y(), track.GetLastPoint().z());
			track.SetAcceleration(field * m_particle.charge);

			track.CalcNewVelocity();
		}
	}
}
