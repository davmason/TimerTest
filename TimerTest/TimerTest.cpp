// TimerTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#pragma comment(lib, "Winmm.lib")

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>
#include <cstdio>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include "TimerTest.h"
#include "StopWatch.h"

template<typename T>
class ThreadSafeVector
{
private:
    std::shared_mutex m_mutex;
    std::vector<T> m_vector;

public:
    ThreadSafeVector() :
        m_mutex(),
        m_vector()
    {

    }

    void Push(const T& element)
    {
        std::unique_lock lock(m_mutex);
        m_vector.push_back(element);
    }

    typename std::vector<T>::iterator Begin()
    {
        std::shared_lock lock(m_mutex);
        return m_vector.begin();
    }

    typename std::vector<T>::iterator End()
    {
        std::shared_lock lock(m_mutex);
        return m_vector.end();
    }
};

ThreadSafeVector<HANDLE> g_threads;

void DoTimerTest(LPCWSTR name, DWORD flags, LONG timerInterval);

void MainlyIdleThreadCallback();
void CPUHeavyThreadCallback();
void MonitorThread();

int main()
{
    HANDLE currentThread = OpenThread(THREAD_ALL_ACCESS, false, GetCurrentThreadId());
    g_threads.Push(currentThread);

    std::thread t1(MainlyIdleThreadCallback);
    std::thread t2(CPUHeavyThreadCallback);
    std::thread t3(MonitorThread);

    DoTimerTest(L"Default", 0, 1);
    
    // Only available on Windows 10 1803 and higher
    DoTimerTest(L"HighResolutionFlag", CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, 1);

    // Before Windows 10 2204 this sets the global timer state for the entire machine,
    // it can increase power consumption
    timeBeginPeriod(1);
    DoTimerTest(L"timeBeginPeriod", 0, 1);
    timeEndPeriod(1);

    t3.join();
}

void DoTimerTest(LPCWSTR name, DWORD flags, LONG timerInterval)
{
    wprintf(L"Running test %s with flags=0x%x and interval=%d\n", name, flags, timerInterval);

    HANDLE timer = CreateWaitableTimerExW(NULL, NULL, flags, TIMER_ALL_ACCESS);
    if (timer == 0)
    {
        printf("CreateWaitableTimerExW failed, GetLastError=0x%x\n", GetLastError());
        return;
    }

    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -1LL;
    if (!SetWaitableTimer(timer, &dueTime, timerInterval, NULL, NULL, 0))
    {
        CloseHandle(timer);
        printf("SetWaitableTimer failed, GetLastError=0x%x\n", GetLastError());
        return;
    }

    StopWatch sw;
    LARGE_INTEGER diff;
    LARGE_INTEGER sum;
    sum.QuadPart = 0;

    sw.Start();
    size_t numIterations = 100;
    for (size_t i = 0; i < numIterations; ++i)
    {
        if (WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0)
        {
            printf("WaitForSingleObject failed, GetLastError=0x%x\n", GetLastError());
            break;
        }

        sum.QuadPart += sw.ElapsedMicroSeconds().QuadPart;
        sw.Reset();
    }

    sw.Stop();

    double average = (sum.QuadPart / (double)numIterations) / 1000;
    printf("After %u iterations, average wait time is %f ms\n", numIterations, average);

    CancelWaitableTimer(timer);
    CloseHandle(timer);
}


void MainlyIdleThreadCallback()
{
    HANDLE currentThread = OpenThread(THREAD_ALL_ACCESS, false, GetCurrentThreadId());
    g_threads.Push(currentThread);

    while (true)
    {
        Sleep(1);
    }
}

void CPUHeavyThreadCallback()
{
    HANDLE currentThread = OpenThread(THREAD_ALL_ACCESS, false, GetCurrentThreadId());
    g_threads.Push(currentThread);

    while (true)
    {
        for (int i = INT_MAX; i > INT_MIN; --i)
        {
            if (i == 0)
            {
                printf("Here!\n");
            }
        }
    }
}

void MonitorThread()
{
    HANDLE currentThread = OpenThread(THREAD_ALL_ACCESS, false, GetCurrentThreadId());
    g_threads.Push(currentThread);

    while (true)
    {
        Sleep(1000);

        for (auto it = g_threads.Begin(); it != g_threads.End(); ++it)
        {
            HANDLE threadHandle = *it;
            printf("\n\n\nThread 0x%p\n", threadHandle);

            FILETIME creation;
            FILETIME end;
            FILETIME kernel;
            FILETIME user;
            if (!GetThreadTimes(threadHandle, &creation, &end, &kernel, &user))
            {
                printf("GetThreadTimes failed, GetLastError=0x%x\n", GetLastError());
            }
            else
            {
                SYSTEMTIME userSystemTime;
                if (!FileTimeToSystemTime(&user, &userSystemTime))
                {
                    printf("FileTimeToSystemTime failed, GetLastError=0x%x\n", GetLastError());
                }
                else
                {
                    printf("Thread user time: %dh%dm%ds%dms\n", userSystemTime.wHour, userSystemTime.wMinute, userSystemTime.wSecond, userSystemTime.wMilliseconds);
                }

                SYSTEMTIME kernelSystemTime;
                if (!FileTimeToSystemTime(&kernel, &kernelSystemTime))
                {
                    printf("FileTimeToSystemTime failed, GetLastError=0x%x\n", GetLastError());
                }
                else
                {
                    printf("Thread kernel time: %dh%dm%ds%dms\n", kernelSystemTime.wHour, kernelSystemTime.wMinute, kernelSystemTime.wSecond, kernelSystemTime.wMilliseconds);
                }
            }

            ULONG64 cycles;
            if (!QueryThreadCycleTime(threadHandle, &cycles))
            {
                printf("QueryThreadCycleTime failed, GetLastError=0x%x\n", GetLastError());
            }
            else
            {
                printf("Cycles: %lld\n", cycles);
            }

        }
    }
}
