#include "wnd_base.h"

static const wchar_t* HANDLER = L"PROP_HANDLER";

WndBase::WndBase(HINSTANCE instance, const wchar_t* caption)
   : m_Instance(instance) {
   wcsncpy_s(m_Caption, caption, 256);
}

WndBase::~WndBase() {
   DestroyWindow(m_Wnd);
}

HWND WndBase::GetWnd() {
   return m_Wnd;
}

void WndBase::Create(const WndCreateData& data) {
   WNDCLASSEXW wcex{0};

   const char* name = typeid(*this).name();
   size_t outSize;
   mbstowcs_s(&outSize, m_Name, 256, name, 255);

   if (!GetClassInfoExW(m_Instance, m_Name, &wcex)) {
      wcex = CreateWndClass();
      RegisterClassExW(&wcex);
   }

   m_ParentWnd = data.parentWnd;

   BeforeWndCreate(data);

   m_Wnd = CreateWindowExW(GetExFlags(), m_Name, m_Caption, GetFlags(), data.pos.x, data.pos.y, m_StartWidth, m_StartHeight, m_ParentWnd, nullptr, m_Instance, this);
   SetPropW(m_Wnd, HANDLER, this);

   CreateView();

   m_IsCreated = true;
}

void WndBase::Destroy(bool fromProc) {
   if (!fromProc) {
      DestroyWindow(m_Wnd);
   }

   m_IsCreated = false;
}

void WndBase::Show(const WndShowData& data) {
   if (m_IsVisible) {
      return;
   }

   ShowWindow(m_Wnd, SW_SHOW);
   m_IsVisible = true;
}

void WndBase::Hide() {
   if (!m_IsVisible) {
      return;
   }

   ShowWindow(m_Wnd, SW_HIDE);
   m_IsVisible = false;
}

bool WndBase::IsVisible() {
   return m_IsVisible;
}

void WndBase::SetPos(POINT pos) {
   POINT size = GetSize();
   MoveWindow(m_Wnd, pos.x, pos.y, size.x, size.y, true);
}

POINT WndBase::GetSize() {
   RECT rect{0};
   GetWindowRect(m_Wnd, &rect);
   return {rect.right - rect.left, rect.bottom - rect.top};
}

POINT WndBase::GetDefaultPos() {
   return {0,0};
}

LRESULT WndBase::StaticMessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   if (message == WM_CREATE) {
      CREATESTRUCTW* cs = (CREATESTRUCTW*) lParam;
      WndBase* wnd = (WndBase*) cs->lpCreateParams;
      if (wnd) {
         return DefWindowProc(hWnd, message, wParam, lParam);
      }
   } else if (message == WM_DESTROY) {
      WndBase* wnd = (WndBase*) GetPropW(hWnd, HANDLER);
      if (wnd) {
         if (wnd->m_IsCreated) {
            RemovePropW(hWnd, HANDLER);
            wnd->Destroy(true);
         }
         return DefWindowProc(hWnd, message, wParam, lParam);
      }
   }

   WndBase* wnd = (WndBase*) GetPropW(hWnd, HANDLER);
   if (wnd) {
      return wnd->MessageProc(hWnd, message, wParam, lParam);
   }

   return DefWindowProc(hWnd, message, wParam, lParam);
}

WNDCLASSEXW WndBase::CreateWndClass() {
   WNDCLASSEXW wcex{0};
   wcex.cbSize = sizeof(WNDCLASSEXW);
   wcex.style = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc = StaticMessageProc;
   wcex.hInstance = m_Instance;
   wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
   wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
   wcex.lpszClassName = m_Name;
   return wcex;
}

DWORD WndBase::GetExFlags() {
   return 0;
}

void WndBase::CreateView() {}

bool WndBase::UpdateCheckBox(HWND hCheck) {
   if (SendMessage(hCheck, BM_GETCHECK, 0, 0) == BST_CHECKED) {
      SendMessage(hCheck, BM_SETCHECK, BST_UNCHECKED, 0);
      return false;
   } else {
      SendMessage(hCheck, BM_SETCHECK, BST_CHECKED, 0);
      return true;
   }
}

void WndBase::ValidateEditText(HWND hEdit, int min, int max) {
   const int BUFFER_SIZE = 16;
   wchar_t buffer[BUFFER_SIZE];
   int charCount = GetWindowText(hEdit, buffer, BUFFER_SIZE);

   if (charCount) {
      for (int i = 0; i < charCount; i++) {
         if (buffer[i] < 48 || buffer[i] > 57) {
            _itow_s(min, buffer, BUFFER_SIZE, 10);
            SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) buffer);
            return;
         }
      }
   } else {
      _itow_s(min, buffer, BUFFER_SIZE, 10);
      SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) buffer);
      return;
   }

   int number = _wtoi(buffer);
   if (number < min) {
      _itow_s(min, buffer, BUFFER_SIZE, 10);
      SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) buffer);
   } else if (number > max) {
      _itow_s(max, buffer, BUFFER_SIZE, 10);
      SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM) buffer);
   }
}

POINT WndBase::GetDisplaySize() {
   DEVMODE devMode{0};
   devMode.dmSize = sizeof(DEVMODE);
   devMode.dmDriverExtra = 0;
   EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode);

   POINT size{0};
   size.x = devMode.dmPelsWidth;
   size.y = devMode.dmPelsHeight;
   return size;
}

UINT WndBase::GetBarEdge() {
   APPBARDATA abd{};
   abd.cbSize = sizeof(abd);

   SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
   return abd.uEdge;
}

POINT WndBase::GetBarSize() {
   APPBARDATA abd{};
   abd.cbSize = sizeof(abd);

   SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
   return {abd.rc.right - abd.rc.left, abd.rc.bottom - abd.rc.top};
}
