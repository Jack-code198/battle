#include "FrameTimer.h"

void FrameTimer::Init(int fps)
{
	QueryPerformanceFrequency(&timerFreq);
	QueryPerformanceCounter(&timeNow);
	timePrevious = timeNow;
	requestedFPS = fps;
	intervalsPerFrame = ((float)timerFreq.QuadPart / requestedFPS);
	intervalsSinceLastUpdate = 0.0f;
	framesToUpdate = 0;
	lastFramesToUpdate = 0;
}

int FrameTimer::FramesToUpdate()
{
	framesToUpdate = 0;
	QueryPerformanceCounter(&timeNow);
	intervalsSinceLastUpdate = (float)timeNow.QuadPart - (float)timePrevious.QuadPart;
	framesToUpdate = (int)(intervalsSinceLastUpdate / intervalsPerFrame);
	if (framesToUpdate > 0)
	{
		timePrevious.QuadPart += (LONGLONG)(framesToUpdate * intervalsPerFrame);
	}
	lastFramesToUpdate = framesToUpdate;
	return framesToUpdate;
}
