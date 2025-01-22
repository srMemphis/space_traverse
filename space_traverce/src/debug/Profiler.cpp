#include "Profiler.h"

#include "debug/Log.h"

#include <algorithm>
#include <iomanip>
#include <thread>
#include <mutex>
#include <sstream>


void Profiler::BeginSession(const std::string& name, const std::string& filepath)
{
	std::lock_guard lock(m_Mutex);
	if (m_CurrentSession)
	{
		// If there is already a current session, then close it before beginning new one.
		if (Log::GetLogger()) // Edge case: BeginSession() might be before Log::Init()
		{
			LOG_ERROR("Profiler::BeginSession('{}') when session '{}' already open.", name, m_CurrentSession->Name);
		}
		InternalEndSession();
	}
	m_OutputStream.open(filepath);

	if (m_OutputStream.is_open())
	{
		m_CurrentSession = new ProfilerSession({ name });
		WriteHeader();
	}
	else
	{
		if (Log::GetLogger()) // Edge case: BeginSession() might be before Log::Init()
		{
			LOG_ERROR("Profiler could not open file '{}'.", filepath);
		}
	}
}

void Profiler::EndSession()
{
	std::lock_guard lock(m_Mutex);
	InternalEndSession();
}

void Profiler::WriteProfile(const ProfileResult& result)
{
	std::stringstream json;

	json << std::setprecision(3) << std::fixed;
	json << ",{";
	json << "\"cat\":\"function\",";
	json << "\"dur\":" << (result.elapsedTime.count()) << ',';
	json << "\"name\":\"" << result.name << "\",";
	json << "\"ph\":\"X\",";
	json << "\"pid\":0,";
	json << "\"tid\":" << result.threadID << ",";
	json << "\"ts\":" << result.start.count();
	json << "}";

	std::lock_guard lock(m_Mutex);
	if (m_CurrentSession)
	{
		m_OutputStream << json.str();
		m_OutputStream.flush();
	}
}

Profiler::Profiler()
	: m_CurrentSession(nullptr)
{
}

Profiler::~Profiler()
{
	EndSession();
}

void Profiler::WriteHeader()
{
	m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
	m_OutputStream.flush();
}

void Profiler::WriteFooter()
{
	m_OutputStream << "]}";
	m_OutputStream.flush();
}

void Profiler::InternalEndSession()
{
	if (m_CurrentSession)
	{
		WriteFooter();
		m_OutputStream.close();
		delete m_CurrentSession;
		m_CurrentSession = nullptr;
	}
}

Timer::Timer(const char* name)
	: m_Name(name), m_Stopped(false)
{
	m_StartTimepoint = std::chrono::steady_clock::now();
}

Timer::~Timer()
{
	if (!m_Stopped)
		Stop();
}

void Timer::Stop()
{
	auto endTimepoint = std::chrono::steady_clock::now();
	auto highResStart = std::chrono::duration<double, std::micro>{ m_StartTimepoint.time_since_epoch() };
	auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

	Profiler::Get().WriteProfile({ m_Name, highResStart, elapsedTime, std::this_thread::get_id() });

	m_Stopped = true;
}
