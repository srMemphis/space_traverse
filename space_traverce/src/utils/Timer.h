#pragma once
#include <chrono>
#include <ctime>
#include <string>

class Timer
{
public:

	Timer();

	Timer(const std::string& s);

	~Timer();

private:
	std::string m_name;
	std::chrono::steady_clock::time_point m_start;
};

// macros
#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)

#define TIME_SCOPE(functionname) Timer PPCAT(timer,__LINE__)(functionname);
#define TIME_FUNCTION() TIME_SCOPE(__FUNCTION__)
