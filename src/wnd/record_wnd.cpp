#include "record_wnd.h"
#include "fonts.h"
#include "ui_consts.h"
#include "image_library.h"
#include "messages.h"
#include <corecrt_wstdio.h>

#define EXPAND_SYMBOL L"\u25A1"
#define COLLAPSE_SYMBOL L"_"

#define BTN_COLLAPSE 1
#define BTN_DONE 2
#define BTN_DELETE 3
#define BTN_EDIT 4

RecordWnd::~RecordWnd() {
   Destroy(false);
}

void RecordWnd::Destroy(bool fromProc) {
   DeleteObject(m_BorderPen);
   DeleteObject(m_NameFontBig);
   DeleteObject(m_NameFontSmall);

   WndBase::Destroy(fromProc);
}

void RecordWnd::SetShorted(bool isShorted) {
   if (m_IsShorted == isShorted) {
      return;
   }

   m_IsShorted = isShorted;

   RECT rect{0};
   GetWindowRect(m_Wnd, &rect);
   MapWindowPoints(HWND_DESKTOP, m_ParentWnd, (LPPOINT) &rect, 2);
   MoveWindow(m_Wnd, rect.left, rect.top, GetWidth(), rect.bottom - rect.top, true);

   int recordWidth = m_IsShorted ? RECORD_WIDTH_SHORTED : RECORD_WIDTH;
   if (m_CollapseButton) {
      GetWindowRect(m_CollapseButton, &rect);
      MapWindowPoints(HWND_DESKTOP, m_Wnd, (LPPOINT) &rect, 2);
      MoveWindow(m_CollapseButton, recordWidth - X_OFFSET * 2 - IMAGE_SIZE, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
   }

   if (m_DoneButton) {
      GetWindowRect(m_DoneButton, &rect);
      MapWindowPoints(HWND_DESKTOP, m_Wnd, (LPPOINT) &rect, 2);
      MoveWindow(m_DoneButton, recordWidth - X_OFFSET * 4 - IMAGE_SIZE - WIDE_BUTTON_SIZE, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
   }

   if (m_EditButton) {
      GetWindowRect(m_EditButton, &rect);
      MapWindowPoints(HWND_DESKTOP, m_Wnd, (LPPOINT) &rect, 2);
      MoveWindow(m_EditButton, recordWidth - X_OFFSET * 2 - WIDE_BUTTON_SIZE, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
   }
}

void RecordWnd::Collapse() {
   m_IsExpanded = false;
   UpdateCollapse();
}

void RecordWnd::Expand() {
   m_IsExpanded = true;
   UpdateCollapse();
}

bool RecordWnd::IsExpanded() {
    return m_IsExpanded;
}

void RecordWnd::SetPos(POINT pos) {
   MoveWindow(m_Wnd, pos.x, pos.y, GetWidth(), GetHeight(), true);
}

int RecordWnd::GetWidth() {
   return m_IsShorted ? RECORD_WIDTH_SHORTED : RECORD_WIDTH;
}

int RecordWnd::GetHeight() {
   return m_IsExpanded ? RECORD_HEIGHT : TITLE_HEIGHT + Y_OFFSET;
}

void RecordWnd::SetRecord(Record* record) {
   m_Record = record;
   InvalidateRect(m_Wnd, nullptr, true);
}

void RecordWnd::SetStatus(StatusType status) {
   if (m_Status == status) {
      return;
   }

   m_Status = status;
   UpdateStatus();
}

StatusType RecordWnd::GetStatus() {
   return m_Status;
}

int RecordWnd::IconTypeToImageIndex(IconType icon) {
   if (icon == IconType::TABLET) {
      return TABLET_IMAGE;
   } else if (icon == IconType::PILL) {
      return PILL_IMAGE;
   } else if (icon == IconType::SYRINGE) {
      return SYRINGE_IMAGE;
   } else if (icon == IconType::CAPSULE) {
      return CAPSULE_IMAGE;
   } else if (icon == IconType::DROP) {
      return DROP_IMAGE;
   }

   return -1;
}

int RecordWnd::FoodTypeToImageIndex(FoodType icon) {
   if (icon == FoodType::BEFORE_FOOD) {
      return BEFORE_FOOD_IMAGE;
   } else if (icon == FoodType::WITH_FOOD) {
      return WITH_FOOD_IMAGE;
   } else if (icon == FoodType::AFTER_FOOD) {
      return AFTER_FOOD_IMAGE;
   }

   return -1;
}

COLORREF RecordWnd::StatusToColor(StatusType status) {
   if (status == StatusType::UPCOMING) {
      return UPCOMING_COLOR;
   } else if (status == StatusType::CURRENT) {
      return CURRENT_COLOR;
   } else if (status == StatusType::TO_LATE) {
      return TO_LATE_COLOR;
   } else if (status == StatusType::DONE) {
      return DONE_COLOR;
   } else if (status == StatusType::END) {
      return END_COLOR;
   }

   return RGB(0, 0, 0);
}

void RecordWnd::BeforeWndCreate(const WndCreateData& data) {
   auto castData = (const RecordWndCreateData&) data;
   m_IsWorkable = castData.isWorkable;

   m_StartWidth = RECORD_WIDTH;
   m_StartHeight = RECORD_HEIGHT;

   m_NameFontBig = CreateFontIndirectW(&g_ArialBig);
   m_NameFontSmall = CreateFontIndirectW(&g_ArialSmall);
}

DWORD RecordWnd::GetFlags() {
   return WS_BORDER | WS_CHILD;
}

LRESULT RecordWnd::MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   switch (message) {
      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case BTN_COLLAPSE:
               m_IsExpanded = !m_IsExpanded;

               UpdateCollapse();

               SendMessage(m_ParentWnd, WM_EXPAND_RECORD, (WPARAM) m_IsExpanded, (LPARAM) 0);

               break;
            case BTN_DONE:
               m_Status = StatusType::DONE;
               UpdateStatus();
               break;
            case BTN_DELETE:
               if (MessageBoxW(m_Wnd, L"Delete record?", L"Record", MB_YESNO | MB_ICONWARNING | MB_APPLMODAL) == IDYES) {
                  SendMessage(m_ParentWnd, WM_DELETE_RECORD, (WPARAM) m_Record, (LPARAM) 0);
               }
               break;
            case BTN_EDIT:
               SendMessage(m_ParentWnd, WM_EDIT_RECORD, (WPARAM) m_Record, (LPARAM) 0);
               break;
            default:
               return DefWindowProc(m_Wnd, message, wParam, lParam);
         }
         break;
      case WM_PAINT:
         {
            if (!m_Record) {
               break;
            }

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_Wnd, &ps);

            if (m_IsWorkable || m_Status == StatusType::END) {
               COLORREF color = StatusToColor(m_Status);
               COLORREF bkg = GetSysColor(COLOR_WINDOW);

               TRIVERTEX vertex[2];
               vertex[0].x = 0;
               vertex[0].y = 0;
               vertex[0].Red = GetRValue(color) << 8;
               vertex[0].Green = GetGValue(color) << 8;
               vertex[0].Blue = GetBValue(color) << 8;
               vertex[0].Alpha = 0x0000;

               vertex[1].x = GetWidth() / 2;
               vertex[1].y = TITLE_HEIGHT;
               vertex[1].Red = GetRValue(bkg) << 8;
               vertex[1].Green = GetGValue(bkg) << 8;
               vertex[1].Blue = GetBValue(bkg) << 8;
               vertex[1].Alpha = 0x0000;

               GRADIENT_RECT gRect;
               gRect.UpperLeft = 0;
               gRect.LowerRight = 1;

               GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_H);
            }

            POINT offsetPoint = {X_OFFSET, Y_OFFSET};
            POINT imageSize = {IMAGE_SIZE, IMAGE_SIZE};
            int imageIndex = IconTypeToImageIndex(m_Record->iconType);
            if (imageIndex >= 0) {
               DrawImage(hdc, offsetPoint, imageSize, imageIndex);
               offsetPoint.x += IMAGE_SIZE + X_OFFSET * 2;
            }

            imageIndex = FoodTypeToImageIndex(m_Record->foodType);
            if (imageIndex >= 0) {
               DrawImage(hdc, offsetPoint, imageSize, imageIndex);
               offsetPoint.x += IMAGE_SIZE + X_OFFSET * 2;
            }

            SelectObject(hdc, m_NameFontBig);

            SetBkMode(hdc, TRANSPARENT);

            wchar_t str[512];

            swprintf_s(str, L"%s", m_Record->name);
            RECT rt = {offsetPoint.x, offsetPoint.y, (int) (GetWidth() * 0.7) - X_OFFSET * 2, IMAGE_SIZE + offsetPoint.y};
            DrawText(hdc, str, wcslen(str), &rt, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

            if (m_IsExpanded) {
               SelectObject(hdc, m_BorderPen);
               MoveToEx(hdc, 0, TITLE_HEIGHT, nullptr);
               LineTo(hdc, GetWidth(), TITLE_HEIGHT);

               rt.left = LINE_X_OFFSET;
               rt.top = TITLE_HEIGHT + 1 + Y_OFFSET + LINE_Y_OFFSET;
               rt.right = GetWidth() - LINE_X_OFFSET - X_OFFSET;
               rt.bottom = rt.top + LINE_HEIGHT;

               swprintf_s(str, L"Dose: ");
               if (m_Record->doseInteger > 0 || !m_Record->hasFractional) {
                  wcscat_s(str, L"%d ");
                  swprintf_s(str, str, m_Record->doseInteger);
               }

               if (m_Record->hasFractional) {
                  if (m_Record->doseInteger > 0) {
                     wcscat_s(str, L"+ ");
                  }
                  wcscat_s(str, L"%d/%d");
                  swprintf_s(str, str, m_Record->doseNumerator, m_Record->doseDenominator);
               }

               DrawText(hdc, str, wcslen(str), &rt, DT_LEFT | DT_BOTTOM | DT_SINGLELINE);

               swprintf_s(str, L"Food: %s", FoodTypeToString(m_Record->foodType));

               DrawText(hdc, str, wcslen(str), &rt, DT_CENTER | DT_BOTTOM | DT_SINGLELINE);

               if (m_Record->takingDayType != TakingDayType::IN_N_DAYS) {
                  swprintf_s(str, TakingDayTypeToString(m_Record->takingDayType));
               } else {
                  swprintf_s(str, L"Every %d days", m_Record->takingDayPeriod);
               }

               DrawText(hdc, str, wcslen(str), &rt, DT_RIGHT | DT_BOTTOM | DT_SINGLELINE);

               rt.bottom = m_IsWorkable ? RECORD_HEIGHT - LINE_Y_OFFSET : RECORD_HEIGHT - IMAGE_SIZE - Y_OFFSET - LINE_Y_OFFSET;
               rt.top = rt.bottom - LINE_HEIGHT;

               if (m_Record->takingTimeType == TakingTimeType::IN_ANY_TIME || m_Record->takingTimeType == TakingTimeType::BEFORE_BED) {
                  swprintf_s(str, TakingTimeTypeToString(m_Record->takingTimeType));
               } else if (m_Record->takingTimeType == TakingTimeType::BEFORE_HOUR) {
                  swprintf_s(str, L"Before %d o'clock", m_Record->firstHour);
               } else if (m_Record->takingTimeType == TakingTimeType::AFTER_HOUR) {
                  swprintf_s(str, L"After %d o'clock", m_Record->firstHour);
               } else if (m_Record->takingTimeType == TakingTimeType::IN_BETWEEN_HOURS) {
                  swprintf_s(str, L"From %d to %d o'clock", m_Record->firstHour, m_Record->secondHour);
               }

               DrawText(hdc, str, wcslen(str), &rt, DT_LEFT | DT_BOTTOM | DT_SINGLELINE);

               if (m_Record->hasEndDate) {
                  swprintf_s(str, L"Until(d.m.y): %d.%d.%d", m_Record->endDateDay, m_Record->endDateMonth, m_Record->endDateYear);
                  DrawText(hdc, str, wcslen(str), &rt, DT_RIGHT | DT_BOTTOM | DT_SINGLELINE);
               }
            }

            EndPaint(m_Wnd, &ps);
         }

         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}

void RecordWnd::CreateView() {
   m_CollapseButton = CreateWindowW(L"BUTTON", m_IsExpanded ? COLLAPSE_SYMBOL : EXPAND_SYMBOL, WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_TEXT, (int) RECORD_WIDTH - X_OFFSET * 2 - IMAGE_SIZE, Y_OFFSET, IMAGE_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) BTN_COLLAPSE, m_Instance, nullptr);
   if (m_IsWorkable) {
      m_DoneButton = CreateWindowW(L"BUTTON", L"Done", WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_TEXT, (int) RECORD_WIDTH - X_OFFSET * 4 - IMAGE_SIZE - WIDE_BUTTON_SIZE, Y_OFFSET, WIDE_BUTTON_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) BTN_DONE, m_Instance, nullptr);
      if (m_Status == StatusType::DONE) {
         EnableWindow(m_DoneButton, false);
      }
   } else {
      m_DeleteButton = CreateWindowW(L"BUTTON", L"Delete", WS_BORDER | WS_CHILD | (m_IsExpanded ? WS_VISIBLE : 0) | BS_CENTER | BS_TEXT, X_OFFSET, RECORD_HEIGHT - Y_OFFSET * 2 - IMAGE_SIZE, WIDE_BUTTON_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) BTN_DELETE, m_Instance, nullptr);
      m_EditButton = CreateWindowW(L"BUTTON", L"Edit", WS_BORDER | WS_CHILD | (m_IsExpanded ? WS_VISIBLE : 0) | BS_CENTER | BS_TEXT, (int) RECORD_WIDTH - X_OFFSET * 2 - WIDE_BUTTON_SIZE, RECORD_HEIGHT - Y_OFFSET * 2 - IMAGE_SIZE, WIDE_BUTTON_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) BTN_EDIT, m_Instance, nullptr);
   }
}

void RecordWnd::UpdateStatus() {
   if (m_IsWorkable) {
      if (m_Status == StatusType::DONE) {
         EnableWindow(m_DoneButton, false);
         SendMessage(m_ParentWnd, WM_RECORD_DONE, (WPARAM) m_Record, 0);
      } else {
         EnableWindow(m_DoneButton, true);
      }
   }

   InvalidateRect(m_Wnd, nullptr, true);
}

void RecordWnd::UpdateCollapse() {
   SetWindowTextW(m_CollapseButton, m_IsExpanded ? COLLAPSE_SYMBOL : EXPAND_SYMBOL);

   if (m_IsExpanded) {
      ShowWindow(m_DeleteButton, SW_SHOW);
      ShowWindow(m_EditButton, SW_SHOW);
      EnableWindow(m_DeleteButton, true);
      EnableWindow(m_EditButton, true);
   } else {
      ShowWindow(m_DeleteButton, SW_HIDE);
      ShowWindow(m_EditButton, SW_HIDE);
      EnableWindow(m_DeleteButton, false);
      EnableWindow(m_EditButton, false);
   }
}
