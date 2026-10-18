#pragma once
#include "debug/Define.h"

#include <memory>

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog\spdlog.h>
#pragma warning(pop)

class Log
{
public:
	static void Init(const std::string& filename);

	static std::shared_ptr<spdlog::logger> GetLogger();

private:
	Log();
	static std::shared_ptr<spdlog::logger> s_Logger;
};

// log macros
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(Log::GetLogger(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(Log::GetLogger(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(Log::GetLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(Log::GetLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(Log::GetLogger(), __VA_ARGS__)
