#pragma once

#include <Windows.h>

class StopWatch
{
private:
	LARGE_INTEGER m_startTime;
	LARGE_INTEGER m_endTime;
	LARGE_INTEGER m_frequency;
	bool m_running;

	void GetElapsedTicks(LARGE_INTEGER* li);

public:
	StopWatch();
	~StopWatch() = default;

	void Start();
	void Stop();
	void Reset();
	LARGE_INTEGER ElapsedMilliSeconds();
	LARGE_INTEGER ElapsedMicroSeconds();
};

