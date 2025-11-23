#pragma once
#include <Windows.h>

#define TABLET_IMAGE 0
#define PILL_IMAGE 1
#define SYRINGE_IMAGE 2
#define CAPSULE_IMAGE 3
#define DROP_IMAGE 4
#define BEFORE_FOOD_IMAGE 5
#define WITH_FOOD_IMAGE 6
#define AFTER_FOOD_IMAGE 7
#define TODO_IMAGE 8
#define SETTINGS_IMAGE 9

const COLORREF TRANSPARENCY_COLOR = RGB(255, 0, 0);

void InitImageLibrary(HWND hWnd);
void TerminateImageLibrary();
void LoadAtlas(HWND hWnd);
void UnloadAtlas();
void DrawImage(HDC hdc, const POINT pos, const POINT size, int imageIndex);