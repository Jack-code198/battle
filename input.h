#pragma once
#include "config.h"

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateDirectInput();
void CleanUpDirectInput();
void GetInput();
void CreateMyWindow();
bool WindowIsRunning();
void CleanUpWindow();
void ToggleFullscreen();