#include "SimuControl.h"
#include "debug/Log.h"
#include "debug/Profiler.h"

#include <string>

int main()
{
	Log::Init("simulog.log");
	PROFILE_BEGIN_SESSION("profile", "profile.json");

	SimuControl simulation;

	//std::string filepath = "msh/sphere_coarse.msh";
	//std::string filepath = "msh/sphere.msh";
	std::string filepath = "msh/plane.msh";

	if (!simulation.MeshFromFile(filepath))
		return -1;

	if (!simulation.Prepare())
		return 1;
	
	auto res = simulation.Run();

	PROFILE_END_SESSION();

	return res;
}
