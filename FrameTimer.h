#pragma once
#include <Windows.h>

class FrameTimer
{
public:
	void Init(int fps);
	int FramesToUpdate();
	void Reset();
	// Battle loop sets this once per displayed frame (P1/P2 + flow read the same value).
	void SetLogicSteps(int steps);
	// Value returned by the most recent FramesToUpdate() / SetLogicSteps() call.
	int GetLastFramesToUpdate() const { return lastFramesToUpdate; }

private:
	LARGE_INTEGER timerFreq;
	LARGE_INTEGER timeNow;
	LARGE_INTEGER timePrevious;
	int requestedFPS;
	float intervalsPerFrame;
	float intervalsSinceLastUpdate;
	int framesToUpdate;
	int lastFramesToUpdate;
};
