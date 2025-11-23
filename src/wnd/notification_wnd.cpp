#include "notification_wnd.h"
#include "ui_consts.h"
#include "fonts.h"

#define WM_OK_BUTTON 1

#define TIMER_HIDE 1

NotificationWnd::~NotificationWnd() {
   Destroy(false);
}

void NotificationWnd::Destroy(bool fromProc) {
   KillTimer(m_Wnd, TIMER_HIDE);

   DeleteObject(m_CaptionFont);
   DeleteObject(m_TextFont);

   WndBase::Destroy(fromProc);
}

void NotificationWnd::Show(const WndShowData& data) {
   auto castData = (const NotificationWndShowData&) data;

   m_Settings = castData.settings;
   m_IsWarning = castData.isWarning;

   if (m_IsVisible) {
      return;
   }

   POINT pos = GetDefaultPos();
   SetWindowPos(m_Wnd, HWND_TOPMOST, pos.x, pos.y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOACTIVATE);
   m_IsVisible = true;

   if (m_Settings.temporaryNotification) {
      SetTimer(m_Wnd, TIMER_HIDE, m_Settings.notificationTime * 1000, nullptr);
   }
}

void NotificationWnd::Hide() {
   if (m_IsVisible) {
      KillTimer(m_Wnd, TIMER_HIDE);
   }

   WndBase::Hide();
}

void NotificationWnd::SetIsWarning(bool isWarning) {
   if (m_IsWarning == isWarning) {
      return;
   }

   m_IsWarning = isWarning;

   InvalidateRect(m_Wnd, nullptr, true);
}

void NotificationWnd::ResetTimer(unsigned int time) {
   SetTimer(m_Wnd, TIMER_HIDE, time * 1000, nullptr);
}

POINT NotificationWnd::GetDefaultPos() {
   POINT result{};

   POINT displaySize = GetDisplaySize();

   bool rd = m_Settings.notificationCorner == WindowCorner::RIGHT_DOWN;
   bool ld = m_Settings.notificationCorner == WindowCorner::LEFT_DOWN;
   bool lu = m_Settings.notificationCorner == WindowCorner::LEFT_UP;
   bool ru = m_Settings.notificationCorner == WindowCorner::RIGHT_UP;

   if (rd) {
      result.x = displaySize.x - NOTIFICATION_WIDTH - DISPLAY_OFFSET_X;
      result.y = displaySize.y - NOTIFICATION_HEIGHT - DISPLAY_OFFSET_Y;
   } else if (ld) {
      result.x = DISPLAY_OFFSET_X;
      result.y = displaySize.y - NOTIFICATION_HEIGHT - DISPLAY_OFFSET_Y;
   } else if (lu) {
      result.x = DISPLAY_OFFSET_X;
      result.y = DISPLAY_OFFSET_Y;
   } else if (ru) {
      result.x = displaySize.x - NOTIFICATION_WIDTH - DISPLAY_OFFSET_X;
      result.y = DISPLAY_OFFSET_Y;
   } else {
      result.x = displaySize.x - NOTIFICATION_WIDTH - DISPLAY_OFFSET_X;
      result.y = displaySize.y - NOTIFICATION_HEIGHT - DISPLAY_OFFSET_Y;
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

void NotificationWnd::BeforeWndCreate(const WndCreateData& data) {
   m_CaptionFont = CreateFontIndirectW(&g_ArialCaption);
   m_TextFont = CreateFontIndirectW(&g_ArialBig);

   m_StartWidth = NOTIFICATION_WIDTH;
   m_StartHeight = NOTIFICATION_HEIGHT;
}

DWORD NotificationWnd::GetFlags() {
   return WS_POPUP | WS_BORDER;
}

LRESULT NotificationWnd::MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   switch (message) {
      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case WM_OK_BUTTON:
               Hide();
               break;
         }
         break;
      case WM_TIMER:
         if (wParam == TIMER_HIDE) {
            Hide();
         }
         break;
      case WM_PAINT:
         {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            COLORREF color = TO_LATE_COLOR;

            if (m_IsWarning) {
               color = CURRENT_COLOR;
            }

            COLORREF bkg = GetSysColor(COLOR_WINDOW);

            TRIVERTEX vertex[2];
            vertex[0].x = 0;
            vertex[0].y = 0;
            vertex[0].Red = GetRValue(color) << 8;
            vertex[0].Green = GetGValue(color) << 8;
            vertex[0].Blue = GetBValue(color) << 8;
            vertex[0].Alpha = 0x0000;

            vertex[1].x = NOTIFICATION_WIDTH / 2;
            vertex[1].y = LINE_Y_OFFSET * 2 + TITLE_HEIGHT;
            vertex[1].Red = GetRValue(bkg) << 8;
            vertex[1].Green = GetGValue(bkg) << 8;
            vertex[1].Blue = GetBValue(bkg) << 8;
            vertex[1].Alpha = 0x0000;

            GRADIENT_RECT gRect;
            gRect.UpperLeft = 0;
            gRect.LowerRight = 1;

            GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);

            SelectObject(hdc, m_CaptionFont);
            SetBkMode(hdc, TRANSPARENT);

            wchar_t str[256];

            swprintf_s(str, m_Caption);

            RECT rt = {LINE_X_OFFSET, LINE_Y_OFFSET, NOTIFICATION_WIDTH - LINE_X_OFFSET, LINE_Y_OFFSET + TITLE_HEIGHT};
            DrawText(hdc, str, wcslen(str), &rt, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

            SelectObject(hdc, m_TextFont);

            if (m_IsWarning) {
               swprintf_s(str, L"You can take your medicine.");
            } else {
               swprintf_s(str, L"You forgot to take your medicine!");
            }

            rt.top = LINE_Y_OFFSET * 2 + TITLE_HEIGHT;
            rt.bottom = NOTIFICATION_HEIGHT - IMAGE_SIZE - LINE_Y_OFFSET * 2;
            DrawText(hdc, str, wcslen(str), &rt, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

            EndPaint(hWnd, &ps);
         }
         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}

void NotificationWnd::CreateView() {
   m_OkButton = CreateWindow(L"BUTTON", L"Ok", WS_BORDER | WS_VISIBLE | WS_CHILD | BS_TEXT | BS_CENTER, NOTIFICATION_WIDTH / 2 - WIDE_BUTTON_SIZE / 2, NOTIFICATION_HEIGHT - IMAGE_SIZE - LINE_Y_OFFSET, WIDE_BUTTON_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) WM_OK_BUTTON, m_Instance, nullptr);
}
