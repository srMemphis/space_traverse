#include "Log.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

std::shared_ptr<spdlog::logger> Log::s_Logger;

void Log::Init(const std::string& filename)
{
	std::vector<spdlog::sink_ptr> logSinks;
	logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true));

	logSinks.at(0)->set_pattern("%^[%H:%M:%S:%e] [T-%t] [%l]: %v%$");
	logSinks.at(1)->set_pattern("%^[%H:%M:%S:%e] [T-%t-%s:%#] [%l]: %v%$");

	s_Logger = std::make_shared<spdlog::logger>("name", begin(logSinks), end(logSinks));
	spdlog::register_logger(s_Logger);
	s_Logger->set_level(spdlog::level::trace);
	s_Logger->flush_on(spdlog::level::info);
}

std::shared_ptr<spdlog::logger> Log::GetLogger()
{
	return s_Logger;
}
