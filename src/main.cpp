#include <Windows.h>
#include "main_wnd.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
   MainWnd mainWnd(hInstance, L"Drugs And Pills");

   MainWndCreateData data{};
   mainWnd.Create(data);

   MSG msg;
   while (GetMessage(&msg, nullptr, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return 0;
}