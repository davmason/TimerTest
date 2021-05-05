
#include "StopWatch.h"
#include <utility>

StopWatch::StopWatch() :
	m_startTime(),
	m_endTime(),
	m_running(false)
{
	QueryPerformanceFrequency(&m_frequency);
}

void StopWatch::GetElapsedTicks(LARGE_INTEGER* li)
{
	if (m_running)
	{
		QueryPerformanceCounter(li);
		li->QuadPart -= m_startTime.QuadPart;
	}
	else
	{
		li->QuadPart = m_endTime.QuadPart - m_startTime.QuadPart;
	}
}

void StopWatch::Start()
{
	m_running = true;
	QueryPerformanceCounter(&m_startTime);
}

void StopWatch::Stop()
{
	QueryPerformanceCounter(&m_endTime);
	m_running = false;
}

void StopWatch::Reset()
{
	m_running = true;
	QueryPerformanceCounter(&m_startTime);
}

LARGE_INTEGER StopWatch::ElapsedMilliSeconds()
{
	LARGE_INTEGER li;
	GetElapsedTicks(&li);
	li.QuadPart /= (m_frequency.QuadPart / (1000));

	return std::move(li);
}

LARGE_INTEGER StopWatch::ElapsedMicroSeconds()
{
	LARGE_INTEGER li;
	GetElapsedTicks(&li);
	li.QuadPart /= (m_frequency.QuadPart / (1000 * 1000));

	return std::move(li);
}
