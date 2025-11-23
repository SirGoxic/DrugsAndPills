#pragma once
#include <Windows.h>
#include <typeinfo>
#include <string>

struct WndCreateData {
   HWND parentWnd = nullptr;
   POINT pos = {0, 0};
};

struct WndShowData {

};

class WndBase {
public:

   WndBase(HINSTANCE instance, const wchar_t* caption);
   ~WndBase();

public:

   HWND GetWnd();

   virtual void Create(const WndCreateData& data);
   virtual void Destroy(bool fromProc);

   virtual void Show(const WndShowData& data);
   virtual void Hide();
   bool IsVisible();

   virtual void SetPos(POINT pos);
   POINT GetSize();

   virtual POINT GetDefaultPos();

protected:

   static LRESULT CALLBACK StaticMessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:

   wchar_t m_Name[256] = L"";

protected:

   bool m_IsCreated = false;
   bool m_IsVisible = false;

   HINSTANCE m_Instance;
   
   wchar_t m_Caption[256] = L"";

   int m_StartWidth = 0;
   int m_StartHeight = 0;

   HWND m_Wnd = nullptr;
   HWND m_ParentWnd = nullptr;

protected:

   WNDCLASSEXW CreateWndClass();

   virtual void BeforeWndCreate(const WndCreateData& data) = 0;
   virtual DWORD GetFlags() = 0;
   virtual DWORD GetExFlags();

   virtual LRESULT CALLBACK MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;

   virtual void CreateView();

   bool UpdateCheckBox(HWND hCheck);
   void ValidateEditText(HWND hEdit, int min, int max);

   POINT GetDisplaySize();
   UINT GetBarEdge();
   POINT GetBarSize();
};