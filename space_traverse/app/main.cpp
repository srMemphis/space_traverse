#include "debug/Log.h"
#include "debug/Profiler.h"
#include "SimuControl.h"

#include <string>

int main()
{
	Log::Init("simulog.log");
	PROFILE_BEGIN_SESSION("profile", "profile.json");

	SimuControl simulation;

	std::string configfile = "config.yaml";

	if (!simulation.ConfigFromFile(configfile))
		return 1;

	if (!simulation.Prepare())
		return 2;

	auto res = simulation.Run();

	PROFILE_END_SESSION();

	return res;
}
