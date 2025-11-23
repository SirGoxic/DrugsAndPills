#include "image_library.h"
#include "png_reader.h"
#include "files.h"

#define IMAGE_WIDTH 64
#define IMAGE_HEIGHT 64

static HBITMAP s_AtlasBM;
static HBITMAP s_FallbackBM;
static HDC s_AtlasDC;
static HDC s_FallbackDC;
static int s_Width, s_Height;
static bool s_AtlasIsValid;

#define IMAGE_COUNT 16
static POINT s_ImageMap[IMAGE_COUNT] = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, 
                                        {0, 1}, {1, 1}, {2, 1}, {3, 1}, 
                                        {0, 2}, {1, 2}, {2, 2}, {3, 2}, 
                                        {0, 3}, {1, 3}, {2, 3}, {3, 3}};

static void DrawFallbackImage(HDC hdc, POINT pos, POINT size);

void InitImageLibrary(HWND hWnd) {
   if (s_FallbackDC) {
      return;
   }

   HDC hdc = GetDC(hWnd);

   s_FallbackDC = CreateCompatibleDC(hdc);

   s_FallbackBM = CreateCompatibleBitmap(hdc, IMAGE_WIDTH, IMAGE_HEIGHT);
   SelectObject(s_FallbackDC, s_FallbackBM);

   ReleaseDC(hWnd, hdc);

   HBRUSH pinkBrush = CreateSolidBrush(RGB(255, 50, 255));
   HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));

   RECT rect = {0, 0, IMAGE_WIDTH / 2, IMAGE_HEIGHT / 2};
   FillRect(s_FallbackDC, &rect, pinkBrush);

   rect.left = rect.right;
   rect.right = IMAGE_WIDTH;
   FillRect(s_FallbackDC, &rect, blackBrush);

   rect.right = rect.left;
   rect.left = 0;
   rect.top = rect.bottom;
   rect.bottom = IMAGE_HEIGHT;
   FillRect(s_FallbackDC, &rect, blackBrush);

   rect.left = rect.right;
   rect.right = IMAGE_WIDTH;
   FillRect(s_FallbackDC, &rect, pinkBrush);

   DeleteObject(pinkBrush);
   DeleteObject(blackBrush);
}

void TerminateImageLibrary() {
   UnloadAtlas();

   DeleteDC(s_FallbackDC);
   DeleteObject(s_FallbackBM);
}

void LoadAtlas(HWND hWnd) {
   if (s_AtlasBM) {
      UnloadAtlas();
   }

   s_AtlasIsValid = false;

   HDC hdc = GetDC(hWnd);
   s_AtlasBM = LoadPNG(IMAGE_ATLAS, hdc, TRANSPARENCY_COLOR);
   s_AtlasDC = CreateCompatibleDC(hdc);

   if (s_AtlasBM) {
      BITMAP bm;
      GetObject(s_AtlasBM, sizeof(bm), &bm);
      s_Width = bm.bmWidth;
      s_Height = bm.bmHeight;

      if (s_Width % IMAGE_WIDTH == 0 && s_Height % IMAGE_HEIGHT == 0) {
         SelectObject(s_AtlasDC, s_AtlasBM);
         s_AtlasIsValid = true;
      } else {
         s_Width = 0;
         s_Height = 0;

         MessageBox(hWnd, L"Incompatible atlas size", L"Error", MB_OK);
      }
   } else {
      s_Width = 0;
      s_Height = 0;
      
      MessageBox(hWnd, L"Failed to load atlas", L"Error", MB_OK);
   }

   ReleaseDC(hWnd, hdc);
}

void UnloadAtlas() {
   DeleteDC(s_AtlasDC);
   DeleteObject(s_AtlasBM);
}

void DrawImage(HDC hdc, const POINT pos, const POINT size, int imageIndex) {
   SetStretchBltMode(hdc, HALFTONE);

   if (s_AtlasIsValid && s_Width > 0 && s_Height > 0 && imageIndex >= 0 && imageIndex < IMAGE_COUNT) {
      POINT imagePoint = s_ImageMap[imageIndex];
      int xOffset = imagePoint.x * IMAGE_WIDTH;
      int yOffset = imagePoint.y * IMAGE_HEIGHT;
      if (xOffset >= 0 && yOffset >= 0 && xOffset + IMAGE_WIDTH <= s_Width && yOffset + IMAGE_HEIGHT <= s_Height) {
         TransparentBlt(hdc, pos.x, pos.y, size.x, size.y, s_AtlasDC, xOffset, yOffset, IMAGE_WIDTH, IMAGE_HEIGHT, TRANSPARENCY_COLOR);
      } else {
         DrawFallbackImage(hdc, pos, size);
      }
   }else{
      DrawFallbackImage(hdc, pos, size);
   }
}

static void DrawFallbackImage(HDC hdc, const POINT pos, const POINT size) {
   SetStretchBltMode(hdc, HALFTONE);
   StretchBlt(hdc, pos.x, pos.y, size.x, size.y, s_FallbackDC, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT, SRCCOPY);
}