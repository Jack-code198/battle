#pragma once
#include <Windows.h>

class FrameTimer
{
public:
	void Init(int fps);
	int FramesToUpdate();
	// Value returned by the most recent FramesToUpdate() call (for shared anim stepping).
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
