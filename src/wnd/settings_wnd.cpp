#include "settings_wnd.h"
#include "fonts.h"
#include <CommCtrl.h>
#include "messages.h"

#define WM_CLOSE_BUTTON 1
#define WM_DEBUG_TIME_CHECK 2

SettingsWnd::~SettingsWnd() {
   Destroy(false);
}

void SettingsWnd::Destroy(bool fromProc) {
   DeleteObject(m_CaptionFont);
   DeleteObject(m_TextFont);

   WndBase::Destroy(fromProc);
}

void SettingsWnd::Show(const WndShowData& data) {
   if (m_IsVisible) {
      return;
   }

   POINT pos = GetDefaultPos();
   SetWindowPos(m_Wnd, HWND_TOP, pos.x, pos.y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
   m_IsVisible = true;

   EnableWindow(m_ParentWnd, false);

#ifndef NDEBUG

   SYSTEMTIME localTime = m_TimeUtils->GetCurrentLocalTime();
   SendMessage(m_DebugDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);
   SendMessage(m_DebugTimePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);

#endif

   const int BUFFER_SIZE = 16;
   wchar_t buffer[BUFFER_SIZE];

   SendMessage(m_WindowCornerCombo, CB_SETCURSEL, (WPARAM)static_cast<int>(m_Settings.mainWindowCorner), 0);

   SendMessage(m_CreateRecordCollapsedCheck, BM_SETCHECK, m_Settings.createRecordCollapsed ? BST_CHECKED : BST_UNCHECKED, 0);

   _itow_s(m_Settings.updateTime, buffer, BUFFER_SIZE, 10);
   SendMessage(m_UpdateTimeEdit, WM_SETTEXT, 0, (LPARAM) buffer);

   _itow_s(m_Settings.bedTime, buffer, BUFFER_SIZE, 10);
   SendMessage(m_BedTimeEdit, WM_SETTEXT, 0, (LPARAM) buffer);

   SendMessage(m_SaveToLateCheck, BM_SETCHECK, m_Settings.shouldSaveToLate ? BST_CHECKED : BST_UNCHECKED, 0);

   SendMessage(m_ClearDoneCheck, BM_SETCHECK, m_Settings.shouldClearDone ? BST_CHECKED : BST_UNCHECKED, 0);

   SendMessage(m_UseNotificationCheck, BM_SETCHECK, m_Settings.useNotification ? BST_CHECKED : BST_UNCHECKED, 0);

   SendMessage(m_TemporaryNotificationCheck, BM_SETCHECK, m_Settings.temporaryNotification ? BST_CHECKED : BST_UNCHECKED, 0);

   _itow_s(m_Settings.notificationTime, buffer, BUFFER_SIZE, 10);
   SendMessage(m_NotificationTimeEdit, WM_SETTEXT, 0, (LPARAM) buffer);

   SendMessage(m_NotificationCornerCombo, CB_SETCURSEL, (WPARAM)static_cast<int>(m_Settings.notificationCorner), 0);
}

void SettingsWnd::Hide() {
   EnableWindow(m_ParentWnd, true);

   WndBase::Hide();
}

Settings SettingsWnd::GetSettings() {
   return m_Settings;
}

POINT SettingsWnd::GetDefaultPos() {
   POINT result{};

   POINT displaySize = GetDisplaySize();

   bool rd = m_Settings.mainWindowCorner == WindowCorner::RIGHT_DOWN;
   bool ld = m_Settings.mainWindowCorner == WindowCorner::LEFT_DOWN;
   bool lu = m_Settings.mainWindowCorner == WindowCorner::LEFT_UP;
   bool ru = m_Settings.mainWindowCorner == WindowCorner::RIGHT_UP;

   if (rd) {
      result.x = displaySize.x - WND_WIDTH * 2 - DISPLAY_OFFSET_X * 2;
      result.y = displaySize.y - WND_HEIGHT - DISPLAY_OFFSET_Y;
   } else if (ld) {
      result.x = WND_WIDTH + DISPLAY_OFFSET_X * 2;
      result.y = displaySize.y - WND_HEIGHT - DISPLAY_OFFSET_Y;
   } else if (lu) {
      result.x = WND_WIDTH + DISPLAY_OFFSET_X * 2;
      result.y = DISPLAY_OFFSET_Y;
   } else if (ru) {
      result.x = displaySize.x - WND_WIDTH * 2 - DISPLAY_OFFSET_X * 2;
      result.y = DISPLAY_OFFSET_Y;
   } else {
      result.x = displaySize.x - WND_WIDTH * 2 - DISPLAY_OFFSET_X * 2;
      result.y = displaySize.y - WND_HEIGHT - DISPLAY_OFFSET_Y;
   }

   UINT edge = GetBarEdge();
   POINT barSize = GetBarSize();

   if (edge == ABE_BOTTOM && (rd || ld)) {
      result.y -= barSize.y;
   } else if (edge == ABE_LEFT && (ld || lu)) {
      result.x += barSize.x;
   } else if (edge == ABE_TOP && (ru || lu)) {
      result.y += barSize.y;
   } else if (edge == ABE_RIGHT && (rd || ru)) {
      result.x -= barSize.x;
   }

   return result;
}

void SettingsWnd::BeforeWndCreate(const WndCreateData& data) {
   auto castData = (const SettingsWndCreateData&) data;
   m_TimeUtils = castData.timeUtils;
   m_Settings = castData.settings;

   m_CaptionFont = CreateFontIndirectW(&g_ArialCaption);
   m_TextFont = CreateFontIndirectW(&g_ArialBig);

   m_StartWidth = WND_WIDTH;
   m_StartHeight = WND_HEIGHT;
}

DWORD SettingsWnd::GetFlags() {
   return WS_POPUP | WS_BORDER;
}

LRESULT SettingsWnd::MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   switch (message) {
      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case EN_UPDATE:
               {
                  HWND handler = (HWND) lParam;
                  if (handler == m_UpdateTimeEdit) {
                     ValidateEditText(handler, 5, 86000);
                  } else if (handler == m_BedTimeEdit) {
                     ValidateEditText(handler, 0, 23);
                  } else if (handler == m_NotificationTimeEdit) {
                     ValidateEditText(handler, 1, 300);
                  }
               }
               break;
            case BN_CLICKED:
               {
                  HWND handler = (HWND) lParam;
                  if (handler != m_CloseButton) {
                     UpdateCheckBox(handler);
                  }
               }
               break;
            case WM_CLOSE_BUTTON:
               SaveSettings();
               Hide();
               break;
#ifndef NDEBUG
            case WM_DEBUG_TIME_CHECK:
               bool currentCheck = UpdateCheckBox(m_DebugTimeCheck);
               ShowWindow(m_DebugTimePicker, currentCheck ? SW_SHOW : SW_HIDE);
               ShowWindow(m_DebugDatePicker, currentCheck ? SW_SHOW : SW_HIDE);
               break;
#endif
         }
         break;
      case WM_PAINT:
         {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            SelectObject(hdc, m_CaptionFont);

            wchar_t str[32];

            swprintf_s(str, m_Caption);

            RECT rt = {LINE_X_OFFSET, LINE_Y_OFFSET, LIST_WIDTH + LINE_X_OFFSET, LINE_Y_OFFSET + TITLE_HEIGHT};
            DrawText(hdc, str, wcslen(str), &rt, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

            SelectObject(hdc, m_TextFont);

            ResetCorret();

#ifndef NDEBUG
            DrawText(hdc, L"Debug time: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            CorretNextLine();
#endif
            DrawText(hdc, L"Corner: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            CorretNextLine();

            DrawText(hdc, L"Create collapsed: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            CorretNextLine();

            DrawText(hdc, L"Update time: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

            m_PaintCorret.left = WND_WIDTH - LONG_FIELD_WIDTH - LINE_X_OFFSET + SHORT_FIELD_WIDTH + LINE_X_OFFSET;
            DrawText(hdc, L"sec", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

            CorretNextLine();
            m_PaintCorret.left = LINE_X_OFFSET;

            DrawText(hdc, L"Bed time: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

            m_PaintCorret.left = WND_WIDTH - LONG_FIELD_WIDTH - LINE_X_OFFSET + SHORT_FIELD_WIDTH + LINE_X_OFFSET;
            DrawText(hdc, L"o`clock", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

            CorretNextLine();
            m_PaintCorret.left = LINE_X_OFFSET;

            DrawText(hdc, L"Save lost: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            CorretNextLine();

            DrawText(hdc, L"Clear done: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            CorretNextLine();

            DrawText(hdc, L"Notification: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            CorretNextLine();

            DrawText(hdc, L"Temp notification: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            CorretNextLine();

            DrawText(hdc, L"Notification time: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

            m_PaintCorret.left = WND_WIDTH - LONG_FIELD_WIDTH - LINE_X_OFFSET + SHORT_FIELD_WIDTH + LINE_X_OFFSET;
            DrawText(hdc, L"sec", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

            CorretNextLine();
            m_PaintCorret.left = LINE_X_OFFSET;

            DrawText(hdc, L"Notification corner: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
            CorretNextLine();

            EndPaint(hWnd, &ps);
         }
         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}

void SettingsWnd::CreateView() {
   m_CloseButton = CreateWindow(L"BUTTON", L"X", WS_BORDER | WS_VISIBLE | WS_CHILD | BS_TEXT | BS_CENTER, WND_WIDTH - LINE_X_OFFSET - IMAGE_SIZE, LINE_Y_OFFSET + Y_OFFSET, IMAGE_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) WM_CLOSE_BUTTON, m_Instance, nullptr);

   ResetCorret();
   m_PaintCorret.left = WND_WIDTH - LONG_FIELD_WIDTH - LINE_X_OFFSET;

#ifndef NDEBUG
   m_DebugTimeCheck = CreateWindow(L"BUTTON", L"Activate", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, m_PaintCorret.left, m_PaintCorret.top, (LONG_FIELD_WIDTH - LINE_X_OFFSET * 3) / 3 , LINE_HEIGHT, m_Wnd, (HMENU) WM_DEBUG_TIME_CHECK, m_Instance, nullptr);

   {
      INITCOMMONCONTROLSEX icex;
      icex.dwSize = sizeof(icex);
      icex.dwICC = ICC_DATE_CLASSES;

      InitCommonControlsEx(&icex);

      m_DebugDatePicker = CreateWindow(DATETIMEPICK_CLASS, L"Deubg Date", WS_BORDER | WS_CHILD, WND_WIDTH - (LONG_FIELD_WIDTH / 3) * 2 - LINE_X_OFFSET * 2, m_PaintCorret.top, (LONG_FIELD_WIDTH - LINE_X_OFFSET * 3) / 3, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
      m_DebugTimePicker = CreateWindow(DATETIMEPICK_CLASS, L"Debug Time", WS_BORDER | WS_CHILD | DTS_TIMEFORMAT, WND_WIDTH - LONG_FIELD_WIDTH / 3 - LINE_X_OFFSET, m_PaintCorret.top, (LONG_FIELD_WIDTH - LINE_X_OFFSET * 3) / 3, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);

      SYSTEMTIME localTime = m_TimeUtils->GetCurrentLocalTime();
      SendMessage(m_DebugDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);
      SendMessage(m_DebugTimePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);
   }
   CorretNextLine();
#endif

   m_WindowCornerCombo = CreateWindow(L"COMBOBOX", nullptr, WS_BORDER | WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS, m_PaintCorret.left, m_PaintCorret.top, LONG_FIELD_WIDTH, DROP_LIST_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   for (WindowCorner corner = WindowCorner::begin; corner <= WindowCorner::end; corner = static_cast<WindowCorner>(static_cast<size_t>(corner) + 1)) {
      SendMessage(m_WindowCornerCombo, CB_ADDSTRING, 0, (LPARAM) WindowCornerToString(corner));
   }
   SendMessage(m_WindowCornerCombo, CB_SETCURSEL, 0, 0);
   CorretNextLine();

   m_CreateRecordCollapsedCheck = CreateWindow(L"BUTTON", L"Active", WS_CHILD | BS_CHECKBOX | WS_VISIBLE, m_PaintCorret.left, m_PaintCorret.top, CHECKBOX_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_CreateRecordCollapsedCheck, BM_SETCHECK, BST_CHECKED, 0);
   CorretNextLine();

   m_UpdateTimeEdit = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ES_NUMBER | WS_VISIBLE, m_PaintCorret.left, m_PaintCorret.top, SHORT_FIELD_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_UpdateTimeEdit, EM_SETLIMITTEXT, (WPARAM) 6, 0);
   SendMessage(m_UpdateTimeEdit, WM_SETTEXT, 0, (LPARAM) L"30");
   CorretNextLine();

   m_BedTimeEdit = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ES_NUMBER | WS_VISIBLE, m_PaintCorret.left, m_PaintCorret.top, SHORT_FIELD_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_BedTimeEdit, EM_SETLIMITTEXT, (WPARAM) 3, 0);
   SendMessage(m_BedTimeEdit, WM_SETTEXT, 0, (LPARAM) L"22");
   CorretNextLine();

   m_SaveToLateCheck = CreateWindow(L"BUTTON", L"Active", WS_CHILD | BS_CHECKBOX | WS_VISIBLE, m_PaintCorret.left, m_PaintCorret.top, CHECKBOX_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_SaveToLateCheck, BM_SETCHECK, BST_CHECKED, 0);
   CorretNextLine();

   m_ClearDoneCheck = CreateWindow(L"BUTTON", L"Active", WS_CHILD | BS_CHECKBOX | WS_VISIBLE, m_PaintCorret.left, m_PaintCorret.top, CHECKBOX_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_ClearDoneCheck, BM_SETCHECK, BST_CHECKED, 0);
   CorretNextLine();

   m_UseNotificationCheck = CreateWindow(L"BUTTON", L"Active", WS_CHILD | BS_CHECKBOX | WS_VISIBLE, m_PaintCorret.left, m_PaintCorret.top, CHECKBOX_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_UseNotificationCheck, BM_SETCHECK, BST_CHECKED, 0);
   CorretNextLine();

   m_TemporaryNotificationCheck = CreateWindow(L"BUTTON", L"Active", WS_CHILD | BS_CHECKBOX | WS_VISIBLE, m_PaintCorret.left, m_PaintCorret.top, CHECKBOX_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_TemporaryNotificationCheck, BM_SETCHECK, BST_CHECKED, 0);
   CorretNextLine();

   m_NotificationTimeEdit = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ES_NUMBER | WS_VISIBLE, m_PaintCorret.left, m_PaintCorret.top, SHORT_FIELD_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_NotificationTimeEdit, EM_SETLIMITTEXT, (WPARAM) 3, 0);
   SendMessage(m_NotificationTimeEdit, WM_SETTEXT, 0, (LPARAM) L"5");
   CorretNextLine();

   m_NotificationCornerCombo = CreateWindow(L"COMBOBOX", nullptr, WS_BORDER | WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS, m_PaintCorret.left, m_PaintCorret.top, LONG_FIELD_WIDTH, DROP_LIST_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   for (WindowCorner corner = WindowCorner::begin; corner <= WindowCorner::end; corner = static_cast<WindowCorner>(static_cast<size_t>(corner) + 1)) {
      SendMessage(m_NotificationCornerCombo, CB_ADDSTRING, 0, (LPARAM) WindowCornerToString(corner));
   }
   SendMessage(m_NotificationCornerCombo, CB_SETCURSEL, 0, 0);
   CorretNextLine();
}

void SettingsWnd::ResetCorret() {
   m_PaintCorret.left = LINE_X_OFFSET;
   m_PaintCorret.top = TITLE_HEIGHT_WITH_OFFSET;
   m_PaintCorret.right = WND_WIDTH - LINE_X_OFFSET;
   m_PaintCorret.bottom = m_PaintCorret.top + LINE_HEIGHT;
}

void SettingsWnd::CorretNextLine() {
   m_PaintCorret.top = m_PaintCorret.bottom + LINE_Y_OFFSET;
   m_PaintCorret.bottom = m_PaintCorret.top + LINE_HEIGHT;
}

void SettingsWnd::SaveSettings() {
#ifndef NDEBUG

   bool activeDebugTime = SendMessage(m_DebugTimeCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
   m_TimeUtils->SetActiveDebugTime(activeDebugTime);

   if (activeDebugTime) {
      SYSTEMTIME debugDate, debugTime;
      SendMessage(m_DebugDatePicker, DTM_GETSYSTEMTIME, 0, (LPARAM) &debugDate);
      SendMessage(m_DebugTimePicker, DTM_GETSYSTEMTIME, 0, (LPARAM) &debugTime);

      debugDate.wHour = debugTime.wHour;
      debugDate.wMinute = debugTime.wMinute;
      debugDate.wSecond = debugTime.wSecond;
      debugDate.wMilliseconds = debugTime.wMilliseconds;

      m_TimeUtils->SetDebugTime(debugDate);
   }

#endif

   const int BUFFER_SIZE = 16;
   wchar_t buffer[BUFFER_SIZE];

   int index = SendMessage(m_WindowCornerCombo, CB_GETCURSEL, 0, 0);
   m_Settings.mainWindowCorner = static_cast<WindowCorner>(index);

   m_Settings.createRecordCollapsed = SendMessage(m_CreateRecordCollapsedCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;

   GetWindowText(m_UpdateTimeEdit, buffer, BUFFER_SIZE);
   m_Settings.updateTime = _wtoi(buffer);

   GetWindowText(m_BedTimeEdit, buffer, BUFFER_SIZE);
   m_Settings.bedTime = _wtoi(buffer);

   m_Settings.shouldSaveToLate = SendMessage(m_SaveToLateCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
   m_Settings.shouldClearDone = SendMessage(m_ClearDoneCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;

   m_Settings.useNotification = SendMessage(m_UseNotificationCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
   m_Settings.temporaryNotification = SendMessage(m_TemporaryNotificationCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;

   GetWindowText(m_NotificationTimeEdit, buffer, BUFFER_SIZE);
   m_Settings.notificationTime = _wtoi(buffer);

   index = SendMessage(m_NotificationCornerCombo, CB_GETCURSEL, 0, 0);
   m_Settings.notificationCorner = static_cast<WindowCorner>(index);

   SendMessage(m_ParentWnd, WM_SETTINGS_UPDATE, 0, 0);
}
