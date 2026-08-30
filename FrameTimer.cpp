#include "FrameTimer.h"
#include "Config.h"

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

void FrameTimer::Reset()
{
	QueryPerformanceCounter(&timePrevious);
	intervalsSinceLastUpdate = 0.0f;
	framesToUpdate = 0;
	lastFramesToUpdate = 0;
}

void FrameTimer::SetLogicSteps(int steps)
{
	if (steps < 1) steps = 1;
	if (steps > GAME_TIMER_MAX_STEPS_PER_FRAME) {
		steps = GAME_TIMER_MAX_STEPS_PER_FRAME;
	}
	framesToUpdate = steps;
	lastFramesToUpdate = steps;
}

int FrameTimer::FramesToUpdate()
{
	QueryPerformanceCounter(&timeNow);
	intervalsSinceLastUpdate = (float)timeNow.QuadPart - (float)timePrevious.QuadPart;
	int rawFrames = (int)(intervalsSinceLastUpdate / intervalsPerFrame);
	if (rawFrames > GAME_TIMER_MAX_STEPS_PER_FRAME) {
		rawFrames = GAME_TIMER_MAX_STEPS_PER_FRAME;
	}
	framesToUpdate = rawFrames;
	if (framesToUpdate > 0) {
		timePrevious.QuadPart += (LONGLONG)(framesToUpdate * intervalsPerFrame);
	}
	lastFramesToUpdate = framesToUpdate;
	return framesToUpdate;
}
