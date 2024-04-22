#pragma once

#include <chrono>
#include <ctime>
#include <iostream>
#include <string>

class Timer
{
public:

	Timer()
		: Timer("Default timer name")
	{
	}

	Timer(const std::string& s)
	{
		m_name = s;
		m_start = std::chrono::high_resolution_clock::now();
	}

	~Timer()
	{
		auto stop = std::chrono::high_resolution_clock::now();

		std::chrono::duration<float> time = stop - m_start;
		auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(time);

		std::cout << m_name << " elapsed in " << ms_time.count() << "ms." << std::endl;
	}

private:
	std::string m_name;
	std::chrono::steady_clock::time_point m_start;
};

// macros
#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)

#define INIT_TIMER(functionname) Timer PPCAT(timer,__LINE__)(functionname);
#define TIME_SCOPE() INIT_TIMER(__FUNCTION__)
