#include "SimuControl.h"
#include "debug/Log.h"

#include <string>

int main()
{
	Log::Init("simulog.log");

	SimuControl simulation;

	//std::string filepath = "msh/sphere_coarse.msh";
	std::string filepath = "msh/sphere.msh";

	if (!simulation.MeshFromFile(filepath))
		return -1;

	if (!simulation.Prepare())
		return 1;
	
	auto res = simulation.Run();

	return res;
}
