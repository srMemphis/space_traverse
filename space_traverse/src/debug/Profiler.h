#pragma once
#include "debug/Define.h"

#include <chrono>
#include <ctime>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>

struct ProfileResult
{
	std::string name;

	std::chrono::duration<double, std::micro> start;
	std::chrono::microseconds elapsedTime;
	std::thread::id threadID;
};

struct ProfilerSession
{
	std::string Name;
};

class Profiler
{
public:
	Profiler(const Profiler&) = delete;
	Profiler(Profiler&&) = delete;

	void BeginSession(const std::string& name, const std::string& filepath = "profiler.json");
	void EndSession();
	void WriteProfile(const ProfileResult& result);

	static Profiler& Get()
	{
		static Profiler instance;
		return instance;
	}


	~Profiler();

private:
	Profiler();
	void WriteHeader();
	void WriteFooter();
	void InternalEndSession();

private:
	std::mutex m_Mutex;
	ProfilerSession* m_CurrentSession;
	std::ofstream m_OutputStream;
};

class Timer
{
public:
	Timer(const char* name);
	~Timer();
	void Stop();

private:
	const char* m_Name;
	std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
	bool m_Stopped;
};

template <size_t N>
struct ChangeResult
{
	char Data[N];
};

template <size_t N, size_t K>
constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
{
	ChangeResult<N> result = {};

	size_t srcIndex = 0;
	size_t dstIndex = 0;
	while (srcIndex < N)
	{
		size_t matchIndex = 0;
		while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
			matchIndex++;
		if (matchIndex == K - 1)
			srcIndex += matchIndex;
		result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
		srcIndex++;
	}
	return result;
}

// Resolve which function signature macro will be used
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif (defined(__FUNCSIG__) || (_MSC_VER))
#define FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define FUNC_SIG __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define FUNC_SIG __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define FUNC_SIG __func__
#else
#define FUNC_SIG "HZ_FUNC_SIG unknown!"
#endif

#ifdef PROFILER

	#define PROFILE_BEGIN_SESSION(name, filepath) Profiler::Get().BeginSession(name, filepath);
	#define PROFILE_END_SESSION() Profiler::Get().EndSession();

	#define PROFILE_SCOPE_LINE_EXPAND(name, line) auto fixedName##line = CleanupOutputString(name, "__cdecl ");\
												   Timer timer##line(fixedName##line.Data);
	#define PROFILE_SCOPE_LINE(name, line) PROFILE_SCOPE_LINE_EXPAND(name, line)

	#define PROFILE_SCOPE(name) PROFILE_SCOPE_LINE(name, __LINE__)
	#define PROFILE_FUNCTION() PROFILE_SCOPE(FUNC_SIG)

#else
	#define PROFILE_SCOPE(name)
	#define PROFILE_FUNCTION()
	#define PROFILE_BEGIN_SESSION(name, filepath)
	#define PROFILE_END_SESSION()

#endif
