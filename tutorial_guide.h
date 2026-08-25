#pragma once

void ResetTutorialGuide();
void UpdateTutorialGuide(int steps);
const char* GetTutorialGuideObjective();
const char* GetTutorialGuideDetail();
int GetTutorialGuideStepIndex();
int GetTutorialGuideStepCount();
bool IsTutorialGuideComplete();
