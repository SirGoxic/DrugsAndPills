#include "main_wnd.h"
#include "record_list_wnd.h"
#include "image_library.h"
#include "messages.h"
#include "fonts.h"
#include "settings.h"
#include "files.h"

#define WM_OPEN_SETTINGS 1
#define WM_CLOSE_BUTTON 2
#define WM_POPUP_CANCEL 3
#define WM_POPUP_CLOSE 4

#define TIMER_UPDATE 1

#define SHELL_ICON_ID 128

MainWnd::~MainWnd() {
   Destroy(false);

   delete m_NotificationWnd;
   delete m_PanelWnd;
   delete m_SettingsWnd;
   delete m_SetupRecordWnd;
}

void MainWnd::Destroy(bool fromProc) {
   KillTimer(m_Wnd, TIMER_UPDATE);

   m_NotificationWnd->Destroy(false);
   m_PanelWnd->Destroy(false);
   m_SettingsWnd->Destroy(false);
   m_SetupRecordWnd->Destroy(false);

   DeleteObject(m_CaptionFont);
   DeleteObject(m_SettingsBm);

   DestroyIcon(m_Icon);
   DestroyIcon(m_IconWarning);
   DestroyIcon(m_IconFail);

   DestroyMenu(m_SubMenu);
   DestroyMenu(m_Menu);

   WndBase::Destroy(fromProc);

   PostQuitMessage(0);

   Shell_NotifyIcon(NIM_DELETE, &m_IconData);

   TerminateImageLibrary();
}

void MainWnd::Show(const WndShowData& data) {
   m_NotificationWnd->Hide();

   if (m_IsVisible) {
      return;
   }
   POINT pos = GetDefaultPos();
   SetWindowPos(m_Wnd, HWND_TOP, pos.x, pos.y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
   m_IsVisible = true;
}

POINT MainWnd::GetDefaultPos() {
   POINT result{};

   POINT displaySize = GetDisplaySize();

   Settings settings = m_SettingsWnd->GetSettings();

   bool rd = settings.mainWindowCorner == WindowCorner::RIGHT_DOWN;
   bool ld = settings.mainWindowCorner == WindowCorner::LEFT_DOWN;
   bool lu = settings.mainWindowCorner == WindowCorner::LEFT_UP;
   bool ru = settings.mainWindowCorner == WindowCorner::RIGHT_UP;

   if (rd) {
      result.x = displaySize.x - WND_WIDTH - DISPLAY_OFFSET_X;
      result.y = displaySize.y - WND_HEIGHT - DISPLAY_OFFSET_Y;
   } else if (ld) {
      result.x = DISPLAY_OFFSET_X;
      result.y = displaySize.y - WND_HEIGHT - DISPLAY_OFFSET_Y;
   } else if (lu) {
      result.x = DISPLAY_OFFSET_X;
      result.y = DISPLAY_OFFSET_Y;
   } else if (ru) {
      result.x = displaySize.x - WND_WIDTH - DISPLAY_OFFSET_X;
      result.y = DISPLAY_OFFSET_Y;
   } else {
      result.x = displaySize.x - WND_WIDTH - DISPLAY_OFFSET_X;
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

void MainWnd::BeforeWndCreate(const WndCreateData& data) {
   m_Icon = (HICON)LoadImage(m_Instance, ICON, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
   m_IconWarning = (HICON)LoadImage(m_Instance, ICON_WARNING, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
   m_IconFail = (HICON)LoadImage(m_Instance, ICON_FAIL, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);

   m_SubMenu = CreateMenu();
   AppendMenu(m_SubMenu, MF_ENABLED | MF_STRING, WM_POPUP_CANCEL, L"Cancel");
   AppendMenu(m_SubMenu, MF_ENABLED | MF_STRING, WM_POPUP_CLOSE, L"Close");

   m_Menu = CreateMenu();
   AppendMenu(m_Menu, MF_POPUP, (UINT_PTR)m_SubMenu, nullptr);

   m_CaptionFont = CreateFontIndirectW(&g_ArialCaption);

   m_NotificationWnd = new NotificationWnd(m_Instance, L"Notification");

   m_PanelWnd = new PanelWnd(m_Instance, L"");

   m_SettingsWnd = new SettingsWnd(m_Instance, L"Settings");

   m_SetupRecordWnd = new SetupRecordWnd(m_Instance, L"Setup Record");

   m_StartWidth = WND_WIDTH;
   m_StartHeight = WND_HEIGHT;
}

DWORD MainWnd::GetFlags() {
   return WS_POPUP | WS_BORDER;
}

DWORD MainWnd::GetExFlags() {
   return WS_EX_LAYERED;
}

LRESULT MainWnd::MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   switch (message) {
      case WM_EDIT_RECORD:
         m_SetupRecordWnd->Show(*((SetupRecordWndShowData*) wParam));
         break;
      case WM_ADD_RECORD:
         m_SetupRecordWnd->Show(*((SetupRecordWndShowData*) wParam));
         break;
      case WM_END_EDIT:
         SendMessage(m_PanelWnd->GetWnd(), WM_END_EDIT, wParam, lParam);
         break;
      case CM_OPEN_WND:
         switch (LOWORD(lParam)) {
            case WM_LBUTTONDOWN:
               {
                  if (IsVisible()) {
                     SetForegroundWindow(hWnd);
                  } else {
                     MainWndShowData data{};
                     Show(data);
                  }
               }
               break;
            case WM_RBUTTONDOWN:
               {
                  POINT mousePos{0};
                  GetCursorPos(&mousePos);
                  SetForegroundWindow(hWnd);
                  TrackPopupMenu(GetSubMenu(m_Menu, 0), TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON, mousePos.x, mousePos.y, 0, hWnd, nullptr);
               }
               break;
            default:
               return DefWindowProc(hWnd, message, wParam, lParam);
         }
         break;
      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case WM_OPEN_SETTINGS:
               {
                  SettingsWndShowData settingsShowData{};
                  m_SettingsWnd->Show(settingsShowData);
               }
               break;
            case WM_CLOSE_BUTTON:
               Hide();
               break;
            case WM_POPUP_CLOSE:
               m_PanelWnd->SaveState();
               Destroy(false);
               break;
         }
         break;
      case WM_VSCROLL:
         SendMessage(m_PanelWnd->GetWnd(), WM_VSCROLL, wParam, lParam);
         break;
      case WM_KEYUP:
         SendMessage(m_PanelWnd->GetWnd(), WM_KEYUP, wParam, lParam);
         break;
      case WM_SETTINGS_UPDATE:
         {
            Settings settings = m_SettingsWnd->GetSettings();
            SetTimer(m_Wnd, TIMER_UPDATE, settings.updateTime * 1000, nullptr);

            m_PanelWnd->SetSettings(settings);

            POINT pos = GetDefaultPos();
            SetWindowPos(m_Wnd, HWND_TOP, pos.x, pos.y, 0, 0, SWP_NOSIZE);

            SaveSettings(settings);
         }

         break;
      case WM_TIMER:
         if (wParam == TIMER_UPDATE) {
            SendMessage(m_PanelWnd->GetWnd(), WM_TICK_UPDATE, 0, 0);
         }
         break;
      case WM_STATUS_UPDATE:
         UpdateStatus(m_PanelWnd->GetTodayStatus());
         break;
      case WM_PAINT:
         {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            SelectObject(hdc, m_CaptionFont);

            wchar_t str[32];

            swprintf_s(str, m_Caption);

            RECT rt = {LINE_X_OFFSET, LINE_Y_OFFSET, WND_WIDTH - LINE_X_OFFSET, LINE_Y_OFFSET + TITLE_HEIGHT};
            DrawText(hdc, str, wcslen(str), &rt, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

            EndPaint(hWnd, &ps);
         }
         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}

void MainWnd::CreateView() {
   SetLayeredWindowAttributes(m_Wnd, TRANSPARENCY_COLOR, 255, LWA_COLORKEY);

   m_IconData = {};
   m_IconData.cbSize = sizeof(m_IconData);
   m_IconData.hWnd = m_Wnd;
   m_IconData.uID = SHELL_ICON_ID;
   m_IconData.uFlags = NIF_ICON | NIF_MESSAGE;
   m_IconData.uCallbackMessage = CM_OPEN_WND;
   m_IconData.hIcon = m_Icon;

   Shell_NotifyIcon(NIM_ADD, &m_IconData);

   InitImageLibrary(m_Wnd);
   LoadAtlas(m_Wnd);

   HDC hdc = GetDC(m_Wnd);
   m_SettingsBm = CreateCompatibleBitmap(hdc, IMAGE_SIZE, IMAGE_SIZE);
   HDC virtHdc = CreateCompatibleDC(hdc);
   ReleaseDC(m_Wnd, hdc);

   SelectObject(virtHdc, m_SettingsBm);

   HBRUSH tempBrush = CreateSolidBrush(GetBkColor(virtHdc));
   RECT rect = {0, 0, IMAGE_SIZE, IMAGE_SIZE};
   FillRect(virtHdc, &rect, tempBrush);
   DeleteObject(tempBrush);

   DrawImage(virtHdc, {3, 3}, {IMAGE_SIZE - 6, IMAGE_SIZE - 6}, SETTINGS_IMAGE);

   DeleteDC(virtHdc);

   m_SettingsButton = CreateWindow(L"BUTTON", L"", WS_BORDER | WS_VISIBLE | WS_CHILD | BS_BITMAP, LINE_X_OFFSET, LINE_Y_OFFSET + Y_OFFSET, IMAGE_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) WM_OPEN_SETTINGS, m_Instance, nullptr);
   SendMessage(m_SettingsButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) m_SettingsBm);

   m_CloseButton = CreateWindow(L"BUTTON", L"X", WS_BORDER | WS_VISIBLE | WS_CHILD | BS_TEXT | BS_CENTER, WND_WIDTH - LINE_X_OFFSET - IMAGE_SIZE, LINE_Y_OFFSET + Y_OFFSET, IMAGE_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) WM_CLOSE_BUTTON, m_Instance, nullptr);

   NotificationWndCreateData notificationData{};
   notificationData.parentWnd = m_Wnd;
   notificationData.pos = {0, 0};
   m_NotificationWnd->Create(notificationData);

   Settings settings = LoadSettings();

   PanelWndCreateData panelData{};
   panelData.parentWnd = m_Wnd;
   panelData.pos = {0, TITLE_HEIGHT_WITH_OFFSET};
   panelData.mainHeight = WND_HEIGHT;
   panelData.clientHeight = WND_CLIENT_HEIGHT;
   panelData.timeUtils = &m_TimeUtils;
   panelData.serializer = &m_Serializer;
   panelData.settings = settings;
   m_PanelWnd->Create(panelData);

   PanelWndShowData panelShowData{};
   m_PanelWnd->Show(panelShowData);

   SettingsWndCreateData settingsData{};
   settingsData.parentWnd = m_Wnd;
   settingsData.pos = {0, 0};
   settingsData.timeUtils = &m_TimeUtils;
   settingsData.settings = settings;
   m_SettingsWnd->Create(settingsData);

   SetupRecordWndCreateData setupData{};
   setupData.parentWnd = m_Wnd;
   setupData.pos = {0, 0};
   setupData.timeUtils = &m_TimeUtils;
   m_SetupRecordWnd->Create(setupData);

   SetTimer(m_Wnd, TIMER_UPDATE, settings.updateTime * 1000, nullptr);
}

void MainWnd::UpdateStatus(StatusType status) {
   HICON icon;
   if (status == StatusType::UPCOMING) {
      icon = m_Icon;
   } else if (status == StatusType::CURRENT) {
      icon = m_IconWarning;
   } else if (status == StatusType::TO_LATE) {
      icon = m_IconFail;
   } else {
      return;
   }

   NOTIFYICONDATA iconData = {};
   iconData.cbSize = sizeof(iconData);
   iconData.hWnd = m_Wnd;
   iconData.uID = SHELL_ICON_ID;
   iconData.uFlags = NIF_ICON;
   iconData.hIcon = icon;

   Shell_NotifyIcon(NIM_MODIFY, &iconData);

   if (IsVisible()) {
      return;
   }

   Settings settings = m_SettingsWnd->GetSettings();

   if (!settings.useNotification) {
      m_NotificationWnd->Hide();
      return;
   }

   if (status == StatusType::UPCOMING) {
      m_NotificationWnd->Hide();
   } else if (m_NotificationWnd->IsVisible()) {
      m_NotificationWnd->SetIsWarning(status == StatusType::CURRENT);
      if (settings.temporaryNotification) {
         m_NotificationWnd->ResetTimer(settings.notificationTime);
      }
   } else {
      NotificationWndShowData showData{};
      showData.settings = settings;
      showData.isWarning = status == StatusType::CURRENT;
      m_NotificationWnd->Show(showData);
   }
}

void MainWnd::SaveSettings(Settings settings) {
   m_Serializer.TryOpenForSerialize(SETTINGS_SAVE);

   settings.Save(m_Serializer);

   m_Serializer.Close();
}

Settings MainWnd::LoadSettings() {
   Settings result{};

   m_Serializer.TryOpenForDeserialize(SETTINGS_SAVE);

   result.Load(m_Serializer);

   m_Serializer.Close();

   return result;
}
