#include "Timer.h"

#include "debug/Log.h"

Timer::Timer()
	: Timer("Default timer name")
{
}

Timer::Timer(const std::string& s)
{
	m_name = s;
	m_start = std::chrono::high_resolution_clock::now();
}

Timer::~Timer()
{
	auto stop = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float> time = stop - m_start;
	auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();

	LOG_TRACE("{} elapsed in {}m:{}s:{}ms", m_name, 
		ms_time / 60000, (ms_time % 60000) / 1000, (ms_time % 60000) % 1000);
}
