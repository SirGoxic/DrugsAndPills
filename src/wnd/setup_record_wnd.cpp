#include "setup_record_wnd.h"
#include "commctrl.h"
#include "messages.h"
#include <stdio.h>
#include "record_checker.h"
#include "fonts.h"

SetupRecordWnd::~SetupRecordWnd() {
   Destroy(false);
}

void SetupRecordWnd::Destroy(bool fromProc) {
   DeleteObject(m_CaptionFont);
   DeleteObject(m_TextFont);

   WndBase::Destroy(fromProc);
}

void SetupRecordWnd::Show(const WndShowData& data) {
   auto castData = (const SetupRecordWndShowData&) data;

   if (!castData.record) {
      return;
   }

   m_Settings = castData.settings;
   m_EditingRecord = castData.record;
   m_Edit = castData.edit;

   if (m_Edit) {
      LoadRecord();
   } else {
      ClearRecord();
   }

   if (m_IsVisible) {
      return;
   }
   POINT pos = GetDefaultPos();
   SetWindowPos(m_Wnd, HWND_TOP, pos.x, pos.y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
   m_IsVisible = true;

   EnableWindow(m_ParentWnd, false);
}

void SetupRecordWnd::Hide() {
   m_EditingRecord = nullptr;
   m_Edit = false;

   EnableWindow(m_ParentWnd, true);

   WndBase::Hide();
}

POINT SetupRecordWnd::GetDefaultPos() {
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

void SetupRecordWnd::BeforeWndCreate(const WndCreateData& data) {
   auto castData = (const SetupRecordWndCreateData&) data;
   m_TimeUtils = castData.timeUtils;
   m_CaptionFont = CreateFontIndirectW(&g_ArialCaption);
   m_TextFont = CreateFontIndirectW(&g_ArialBig);

   m_StartWidth = WND_WIDTH;
   m_StartHeight = WND_HEIGHT;
}

DWORD SetupRecordWnd::GetFlags() {
   return WS_POPUP | WS_BORDER;
}

LRESULT SetupRecordWnd::MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   switch (message) {
      case WM_COMMAND:
         CommandHandle(wParam, lParam);
         break;
      case WM_NOTIFY:
         NotifyHandle(wParam, lParam);
         break;
      case WM_PAINT:
         Paint();
         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}

void SetupRecordWnd::CreateView() {

   m_CloseButton = CreateWindow(L"BUTTON", L"X", WS_BORDER | WS_VISIBLE | WS_CHILD | BS_TEXT | BS_CENTER, WND_WIDTH - LINE_X_OFFSET - IMAGE_SIZE, LINE_Y_OFFSET + Y_OFFSET, IMAGE_SIZE, IMAGE_SIZE, m_Wnd, nullptr, m_Instance, nullptr);

   ResetCorret();
   m_PaintCorret.left = WND_WIDTH - LONG_FIELD_WIDTH - LINE_X_OFFSET;
   
   m_NameEdit = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT, m_PaintCorret.left, m_PaintCorret.top, LONG_FIELD_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_NameEdit, EM_SETLIMITTEXT, (WPARAM) NAME_SIZE - 1, 0);
   CorretNextLine();

   m_IconCombo = CreateWindow(L"COMBOBOX", nullptr, WS_BORDER | WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS, m_PaintCorret.left, m_PaintCorret.top, LONG_FIELD_WIDTH, DROP_LIST_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   for (IconType icon = IconType::begin; icon <= IconType::end; icon = static_cast<IconType>(static_cast<size_t>(icon) + 1)) {
      SendMessage(m_IconCombo, CB_ADDSTRING, 0, (LPARAM) IconTypeToString(icon));
   }
   SendMessage(m_IconCombo, CB_SETCURSEL, 0, 0);
   CorretNextLine();

   m_FoodCombo = CreateWindow(L"COMBOBOX", nullptr, WS_BORDER | WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS, m_PaintCorret.left, m_PaintCorret.top, LONG_FIELD_WIDTH, DROP_LIST_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   for (FoodType food = FoodType::begin; food <= FoodType::end; food = static_cast<FoodType>(static_cast<size_t>(food) + 1)) {
      SendMessage(m_FoodCombo, CB_ADDSTRING, 0, (LPARAM) FoodTypeToString(food));
   }
   SendMessage(m_FoodCombo, CB_SETCURSEL, 0, 0);
   CorretNextLine();

   m_DoseIntegerEdit = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | ES_NUMBER, m_PaintCorret.left, m_PaintCorret.top, LONG_FIELD_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_DoseIntegerEdit, EM_SETLIMITTEXT, (WPARAM) 3, 0);
   SendMessage(m_DoseIntegerEdit, WM_SETTEXT, 0, (LPARAM) L"0");
   CorretNextLine();

   m_DoseFractionalCheck = CreateWindow(L"BUTTON", L"Has Fractional", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, m_PaintCorret.left, m_PaintCorret.top, CHECKBOX_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);

   m_DoseNumeratorEdit = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ES_NUMBER, m_PaintCorret.left + CHECKBOX_WIDTH + LINE_X_OFFSET, m_PaintCorret.top, SHORT_FIELD_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_DoseNumeratorEdit, EM_SETLIMITTEXT, (WPARAM) 3, 0);
   SendMessage(m_DoseNumeratorEdit, WM_SETTEXT, 0, (LPARAM) L"1");

   m_DoseDenominatorEdit = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ES_NUMBER, WND_WIDTH - SHORT_FIELD_WIDTH - LINE_X_OFFSET, m_PaintCorret.top, SHORT_FIELD_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_DoseDenominatorEdit, EM_SETLIMITTEXT, (WPARAM) 3, 0);
   SendMessage(m_DoseDenominatorEdit, WM_SETTEXT, 0, (LPARAM) L"2");
   CorretNextLine();

   m_EndDateCheck = CreateWindow(L"BUTTON", L"Has End Date", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, m_PaintCorret.left, m_PaintCorret.top, CHECKBOX_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);

   {
      INITCOMMONCONTROLSEX icex;
      icex.dwSize = sizeof(icex);
      icex.dwICC = ICC_DATE_CLASSES;

      InitCommonControlsEx(&icex);

      m_EndDatePicker = CreateWindow(DATETIMEPICK_CLASS, L"End Date Time", WS_BORDER | WS_CHILD, WND_WIDTH - CHECKBOX_WIDTH - LINE_X_OFFSET, m_PaintCorret.top, CHECKBOX_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);

      SYSTEMTIME localTime = m_TimeUtils->GetCurrentLocalTime();
      localTime = m_TimeUtils->AddDays(&localTime, 1);

      SendMessage(m_EndDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);
   }
   CorretNextLine();

   m_TakingDayCombo = CreateWindow(L"COMBOBOX", nullptr, WS_BORDER | WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS, m_PaintCorret.left, m_PaintCorret.top, LONG_FIELD_WIDTH, DROP_LIST_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   for (TakingDayType dayType = TakingDayType::begin; dayType <= TakingDayType::end; dayType = static_cast<TakingDayType>(static_cast<size_t>(dayType) + 1)) {
      SendMessage(m_TakingDayCombo, CB_ADDSTRING, 0, (LPARAM) TakingDayTypeToString(dayType));
   }
   SendMessage(m_TakingDayCombo, CB_SETCURSEL, 0, 0);
   CorretNextLine();

   m_StartDateCheck = CreateWindow(L"BUTTON", L"From Current Date", WS_CHILD | BS_CHECKBOX, m_PaintCorret.left, m_PaintCorret.top, CHECKBOX_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_StartDateCheck, BM_SETCHECK, BST_CHECKED, 0);

   {
      INITCOMMONCONTROLSEX icex;
      icex.dwSize = sizeof(icex);
      icex.dwICC = ICC_DATE_CLASSES;

      InitCommonControlsEx(&icex);

      m_StartDatePicker = CreateWindow(DATETIMEPICK_CLASS, L"Start Date Time", WS_BORDER | WS_CHILD, WND_WIDTH - CHECKBOX_WIDTH - LINE_X_OFFSET, m_PaintCorret.top, CHECKBOX_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);

      SYSTEMTIME localTime = m_TimeUtils->GetCurrentLocalTime();

      SendMessage(m_StartDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);
   }
   CorretNextLine();

   m_TakingDayPeriodEdit = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ES_NUMBER, m_PaintCorret.left, m_PaintCorret.top, SHORT_FIELD_WIDTH, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_TakingDayPeriodEdit, EM_SETLIMITTEXT, (WPARAM) 3, 0);
   SendMessage(m_TakingDayPeriodEdit, WM_SETTEXT, 0, (LPARAM) L"2");
   CorretNextLine();

   m_TakingTimeCombo = CreateWindow(L"COMBOBOX", nullptr, WS_BORDER | WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS, m_PaintCorret.left, m_PaintCorret.top, 300, DROP_LIST_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   for (TakingTimeType timeType = TakingTimeType::begin; timeType <= TakingTimeType::end; timeType = static_cast<TakingTimeType>(static_cast<size_t>(timeType) + 1)) {
      SendMessage(m_TakingTimeCombo, CB_ADDSTRING, 0, (LPARAM) TakingTimeTypeToString(timeType));
   }
   SendMessage(m_TakingTimeCombo, CB_SETCURSEL, 0, 0);
   CorretNextLine();

   m_TakingTimeFirstHour = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ES_NUMBER, m_PaintCorret.left, m_PaintCorret.top, SHORT_FIELD_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_TakingTimeFirstHour, EM_SETLIMITTEXT, (WPARAM) 3, 0);
   SendMessage(m_TakingTimeFirstHour, WM_SETTEXT, 0, (LPARAM) L"12");

   m_TakingTimeSecondHour = CreateWindow(L"EDIT", nullptr, WS_BORDER | WS_CHILD | ES_AUTOHSCROLL | ES_LEFT | ES_NUMBER, WND_WIDTH - SHORT_FIELD_WIDTH - LINE_X_OFFSET, m_PaintCorret.top, SHORT_FIELD_WIDTH - 1, LINE_HEIGHT, m_Wnd, nullptr, m_Instance, nullptr);
   SendMessage(m_TakingTimeSecondHour, EM_SETLIMITTEXT, (WPARAM) 3, 0);
   SendMessage(m_TakingTimeSecondHour, WM_SETTEXT, 0, (LPARAM) L"14");
   CorretNextLine();

   m_SaveButton = CreateWindow(L"BUTTON", L"Confirm", WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_TEXT, LINE_X_OFFSET, WND_HEIGHT - LINE_Y_OFFSET - IMAGE_SIZE, WIDE_BUTTON_SIZE, IMAGE_SIZE, m_Wnd, nullptr, m_Instance, nullptr);
   m_CancelButton = CreateWindow(L"BUTTON", L"Cancel", WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_TEXT, WND_WIDTH - WIDE_BUTTON_SIZE - LINE_X_OFFSET, WND_HEIGHT - LINE_Y_OFFSET - IMAGE_SIZE, WIDE_BUTTON_SIZE, IMAGE_SIZE, m_Wnd, nullptr, m_Instance, nullptr);
}

void SetupRecordWnd::LoadRecord() {
   const int BUFFER_SIZE = 16;
   wchar_t buffer[BUFFER_SIZE];

   Record record = *m_EditingRecord;
   SendMessage(m_NameEdit, WM_SETTEXT, 0, (LPARAM) record.name);
   SendMessage(m_IconCombo, CB_SETCURSEL, (WPARAM)static_cast<int>(record.iconType), 0);
   SendMessage(m_FoodCombo, CB_SETCURSEL, (WPARAM)static_cast<int>(record.foodType), 0);

   _itow_s(record.doseInteger, buffer, BUFFER_SIZE, 10);
   SendMessage(m_DoseIntegerEdit, WM_SETTEXT, 0, (LPARAM) buffer);

   SendMessage(m_DoseFractionalCheck, BM_SETCHECK, record.hasFractional ? BST_CHECKED : BST_UNCHECKED, 0);

   if (record.hasFractional) {
      _itow_s(record.doseNumerator, buffer, BUFFER_SIZE, 10);
      SendMessage(m_DoseNumeratorEdit, WM_SETTEXT, 0, (LPARAM) buffer);
      ShowWindow(m_DoseNumeratorEdit, SW_SHOW);

      _itow_s(record.doseDenominator, buffer, BUFFER_SIZE, 10);
      SendMessage(m_DoseDenominatorEdit, WM_SETTEXT, 0, (LPARAM) buffer);
      ShowWindow(m_DoseDenominatorEdit, SW_SHOW);
   }

   SendMessage(m_EndDateCheck, BM_SETCHECK, record.hasEndDate ? BST_CHECKED : BST_UNCHECKED, 0);

   if (record.hasEndDate) {
      SYSTEMTIME endTime = m_TimeUtils->CreateSysTime(record.endDateYear, record.endDateMonth, record.endDateDay);
      SendMessage(m_EndDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &endTime);
      ShowWindow(m_EndDatePicker, SW_SHOW);
   }

   SendMessage(m_TakingDayCombo, CB_SETCURSEL, (WPARAM)static_cast<int>(record.takingDayType), 0);

   if (record.takingDayType != TakingDayType::EVERY_DAY) {
      SendMessage(m_StartDateCheck, BM_SETCHECK, BST_UNCHECKED, 0);
      ShowWindow(m_StartDateCheck, SW_SHOW);

      SYSTEMTIME startTime = m_TimeUtils->CreateSysTime(record.startDateYear, record.startDateMonth, record.startDateDay);
      SendMessage(m_StartDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &startTime);
      ShowWindow(m_StartDatePicker, SW_SHOW);

      if (record.takingDayType == TakingDayType::IN_N_DAYS) {
         _itow_s(record.takingDayPeriod, buffer, BUFFER_SIZE, 10);
         SendMessage(m_TakingDayPeriodEdit, WM_SETTEXT, 0, (LPARAM) buffer);
         ShowWindow(m_TakingDayPeriodEdit, SW_SHOW);
      }
   }

   SendMessage(m_TakingTimeCombo, CB_SETCURSEL, (WPARAM)static_cast<int>(record.takingTimeType), 0);

   if (record.takingTimeType != TakingTimeType::IN_ANY_TIME && record.takingTimeType != TakingTimeType::BEFORE_BED) {
      _itow_s(record.firstHour, buffer, BUFFER_SIZE, 10);
      SendMessage(m_TakingTimeFirstHour, WM_SETTEXT, 0, (LPARAM) buffer);
      ShowWindow(m_TakingTimeFirstHour, SW_SHOW);

      if (record.takingTimeType == TakingTimeType::IN_BETWEEN_HOURS) {
         _itow_s(record.secondHour, buffer, BUFFER_SIZE, 10);
         SendMessage(m_TakingTimeSecondHour, WM_SETTEXT, 0, (LPARAM) buffer);
         ShowWindow(m_TakingTimeSecondHour, SW_SHOW);
      }
   }
}

void SetupRecordWnd::ClearRecord() {
   SendMessage(m_NameEdit, WM_SETTEXT, 0, (LPARAM) L"");
   SendMessage(m_IconCombo, CB_SETCURSEL, 0, 0);
   SendMessage(m_FoodCombo, CB_SETCURSEL, 0, 0);

   SendMessage(m_DoseIntegerEdit, WM_SETTEXT, 0, 0);

   SendMessage(m_DoseFractionalCheck, BM_SETCHECK, BST_UNCHECKED, 0);

   SendMessage(m_DoseNumeratorEdit, WM_SETTEXT, 0, (LPARAM) L"1");
   ShowWindow(m_DoseNumeratorEdit, SW_HIDE);

   SendMessage(m_DoseDenominatorEdit, WM_SETTEXT, 0, (LPARAM) L"2");
   ShowWindow(m_DoseDenominatorEdit, SW_HIDE);

   SendMessage(m_EndDateCheck, BM_SETCHECK, BST_UNCHECKED, 0);

   ShowWindow(m_EndDatePicker, SW_HIDE);
   SYSTEMTIME localTime = m_TimeUtils->GetCurrentLocalTime();
   localTime = m_TimeUtils->AddDays(&localTime, 1);

   SendMessage(m_EndDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);

   SendMessage(m_TakingDayCombo, CB_SETCURSEL, 0, 0);

   SendMessage(m_StartDateCheck, BM_SETCHECK, BST_CHECKED, 0);
   ShowWindow(m_StartDateCheck, SW_HIDE);

   localTime = m_TimeUtils->GetCurrentLocalTime();
   SendMessage(m_StartDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);
   ShowWindow(m_StartDatePicker, SW_HIDE);

   SendMessage(m_TakingDayPeriodEdit, WM_SETTEXT, 0, (LPARAM) L"2");
   ShowWindow(m_TakingDayPeriodEdit, SW_HIDE);

   SendMessage(m_TakingTimeCombo, CB_SETCURSEL, (WPARAM)0, 0);

   SendMessage(m_TakingTimeFirstHour, WM_SETTEXT, 0, (LPARAM) L"12");
   ShowWindow(m_TakingTimeFirstHour, SW_HIDE);

   SendMessage(m_TakingTimeSecondHour, WM_SETTEXT, 0, (LPARAM) L"14");
   ShowWindow(m_TakingTimeSecondHour, SW_HIDE);
}

void SetupRecordWnd::CommandHandle(WPARAM wParam, LPARAM lParam) {
   HWND handler = (HWND) lParam;
   if (HIWORD(wParam) == EN_UPDATE) {
      if (handler == m_DoseIntegerEdit) {
         ValidateEditText(handler, 0, 255);
      } else if (handler == m_DoseNumeratorEdit) {
         ValidateEditText(handler, 0, 255);
      } else if (handler == m_DoseDenominatorEdit) {
         ValidateEditText(handler, 1, 255);
      } else if (handler == m_TakingDayPeriodEdit) {
         ValidateEditText(handler, 1, 255);
      } else if (handler == m_TakingTimeFirstHour) {
         ValidateEditText(handler, 0, 23);
      } else if (handler == m_TakingTimeSecondHour) {
         ValidateEditText(handler, 0, 23);
      }
   } else if (HIWORD(wParam) == BN_CLICKED) {
      if (handler == m_DoseFractionalCheck) {
         bool currentCheck = UpdateCheckBox(handler);
         ShowWindow(m_DoseNumeratorEdit, currentCheck ? SW_SHOW : SW_HIDE);
         ShowWindow(m_DoseDenominatorEdit, currentCheck ? SW_SHOW : SW_HIDE);

         InvalidateRect(m_Wnd, nullptr, true);
      } else if (handler == m_EndDateCheck) {
         bool currentCheck = UpdateCheckBox(handler);
         ShowWindow(m_EndDatePicker, currentCheck ? SW_SHOW : SW_HIDE);
      } else if (handler == m_StartDateCheck) {
         bool currentCheck = UpdateCheckBox(handler);
         ShowWindow(m_StartDatePicker, currentCheck ? SW_HIDE : SW_SHOW);
      } else if (handler == m_SaveButton) {
         RecordErrorType error = TrySaveRecord();
         if (error == RecordErrorType::NONE) {
            SendMessage(m_ParentWnd, WM_END_EDIT, (WPARAM) true, (LPARAM) m_EditingRecord);
            Hide();
            return;
         } else {
            wchar_t str[512];
            swprintf_s(str, L"Failed to save record, error:\n%s", RecordErrorTypeToString(error));
            MessageBox(m_Wnd, str, L"Record saving error", MB_OK);
         }
      } else if (handler == m_CancelButton || handler == m_CloseButton) {
         if (MessageBox(m_Wnd, L"Exit record setup without saving?", L"Exit record setup", MB_YESNO) == IDYES) {
            SendMessage(m_ParentWnd, WM_END_EDIT, (WPARAM) false, (LPARAM) 0);
            SetForegroundWindow(m_ParentWnd);
            Hide();
            return;
         }
      }
   } else if (HIWORD(wParam) == CBN_SELCHANGE) {
      int index = SendMessage(handler, CB_GETCURSEL, 0, 0);
      if (handler == m_TakingDayCombo) {
         TakingDayType type = static_cast<TakingDayType>(index);
         if (type == TakingDayType::EVERY_DAY) {
            ShowWindow(m_StartDateCheck, SW_HIDE);
            ShowWindow(m_StartDatePicker, SW_HIDE);
            ShowWindow(m_TakingDayPeriodEdit, SW_HIDE);
         } else if (type == TakingDayType::EVERY_OTHER_DAY) {
            ShowWindow(m_StartDateCheck, SW_SHOW);
            if (SendMessage(m_StartDateCheck, BM_GETCHECK, 0, 0) != BST_CHECKED) {
               ShowWindow(m_StartDatePicker, SW_SHOW);
            } else {
               ShowWindow(m_StartDatePicker, SW_HIDE);
            }

            ShowWindow(m_TakingDayPeriodEdit, SW_HIDE);
         } else if (type == TakingDayType::IN_N_DAYS) {
            ShowWindow(m_StartDateCheck, SW_SHOW);
            if (SendMessage(m_StartDateCheck, BM_GETCHECK, 0, 0) != BST_CHECKED) {
               ShowWindow(m_StartDatePicker, SW_SHOW);
            } else {
               ShowWindow(m_StartDatePicker, SW_HIDE);
            }

            ShowWindow(m_TakingDayPeriodEdit, SW_SHOW);
         }

         InvalidateRect(m_Wnd, nullptr, true);
      } else if (handler == m_TakingTimeCombo) {
         TakingTimeType type = static_cast<TakingTimeType>(index);
         if (type == TakingTimeType::IN_ANY_TIME) {
            ShowWindow(m_TakingTimeFirstHour, SW_HIDE);
            ShowWindow(m_TakingTimeSecondHour, SW_HIDE);
         } else if (type == TakingTimeType::BEFORE_HOUR) {
            ShowWindow(m_TakingTimeFirstHour, SW_SHOW);
            ShowWindow(m_TakingTimeSecondHour, SW_HIDE);
         } else if (type == TakingTimeType::AFTER_HOUR) {
            ShowWindow(m_TakingTimeFirstHour, SW_SHOW);
            ShowWindow(m_TakingTimeSecondHour, SW_HIDE);
         } else if (type == TakingTimeType::IN_BETWEEN_HOURS) {
            ShowWindow(m_TakingTimeFirstHour, SW_SHOW);
            ShowWindow(m_TakingTimeSecondHour, SW_SHOW);
         } else if (type == TakingTimeType::BEFORE_BED) {
            ShowWindow(m_TakingTimeFirstHour, SW_HIDE);
            ShowWindow(m_TakingTimeSecondHour, SW_HIDE);
         }

         InvalidateRect(m_Wnd, nullptr, true);
      }
   }
}

void SetupRecordWnd::NotifyHandle(WPARAM wParam, LPARAM lParam) {
   NMHDR* pnmhdr = reinterpret_cast<NMHDR*>(lParam);
   if (pnmhdr->code == DTN_CLOSEUP) {
      if (pnmhdr->hwndFrom == m_EndDatePicker) {
         SYSTEMTIME selectedDate;
         SendMessage(m_EndDatePicker, DTM_GETSYSTEMTIME, 0, (LPARAM) &selectedDate);

         if (m_TimeUtils->IsTimeLaterThanCurrent(&selectedDate)) {
            SYSTEMTIME localTime = m_TimeUtils->GetCurrentLocalTime();
            localTime = m_TimeUtils->AddDays(&localTime, 1);

            SendMessage(m_EndDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);

            MessageBox(m_Wnd, L"End date should be later than current date!", L"Date Error", MB_OK);
         }
      } else if (pnmhdr->hwndFrom == m_StartDatePicker) {
         SYSTEMTIME selectedDate;
         SendMessage(m_StartDatePicker, DTM_GETSYSTEMTIME, 0, (LPARAM) &selectedDate);

         if (!m_TimeUtils->IsTimeLaterThanCurrent(&selectedDate)) {
            SYSTEMTIME localTime = m_TimeUtils->GetCurrentLocalTime();

            SendMessage(m_StartDatePicker, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &localTime);

            MessageBox(m_Wnd, L"Start date should be earlier than or equal current date!", L"Date Error", MB_OK);
         }
      }
   }
}

void SetupRecordWnd::Paint() {
   PAINTSTRUCT ps;
   HDC hdc = BeginPaint(m_Wnd, &ps);

   SelectObject(hdc, m_CaptionFont);

   wchar_t str[32];

   swprintf_s(str, m_Caption);

   RECT rt = {LINE_X_OFFSET, LINE_Y_OFFSET, LIST_WIDTH + LINE_X_OFFSET, LINE_Y_OFFSET + TITLE_HEIGHT};
   DrawText(hdc, str, wcslen(str), &rt, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

   SelectObject(hdc, m_TextFont);

   ResetCorret();

   DrawText(hdc, L"Name: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
   CorretNextLine();

   DrawText(hdc, L"Icon: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
   CorretNextLine();

   DrawText(hdc, L"Food Type: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
   CorretNextLine();

   DrawText(hdc, L"Dose: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
   CorretNextLine();

   if (m_DoseFractionalCheck && SendMessage(m_DoseFractionalCheck, BM_GETCHECK, 0, 0) == BST_CHECKED) {
      rt.left = WND_WIDTH - LONG_FIELD_WIDTH + CHECKBOX_WIDTH + SHORT_FIELD_WIDTH;
      rt.top = m_PaintCorret.top;
      rt.right = WND_WIDTH - LINE_X_OFFSET - SHORT_FIELD_WIDTH;
      rt.bottom = m_PaintCorret.bottom;

      DrawText(hdc, L"/", -1, &rt, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
   }
   CorretNextLine();

   DrawText(hdc, L"End Date: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
   CorretNextLine();

   DrawText(hdc, L"Taking Day: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
   CorretNextLine();
   CorretNextLine();

   if (m_TakingDayCombo) {
      int index = SendMessage(m_TakingDayCombo, CB_GETCURSEL, 0, 0);
      if (static_cast<TakingDayType>(index) == TakingDayType::IN_N_DAYS) {
         rt.left = WND_WIDTH - LONG_FIELD_WIDTH + SHORT_FIELD_WIDTH;
         rt.top = m_PaintCorret.top;
         rt.right = WND_WIDTH - LINE_X_OFFSET;
         rt.bottom = m_PaintCorret.bottom;
         DrawText(hdc, L"days", -1, &rt, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
      }
   }
   CorretNextLine();

   DrawText(hdc, L"Taking Time: ", -1, &m_PaintCorret, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
   CorretNextLine();

   if (m_TakingTimeCombo) {
      int index = SendMessage(m_TakingTimeCombo, CB_GETCURSEL, 0, 0);
      if (static_cast<TakingTimeType>(index) == TakingTimeType::IN_BETWEEN_HOURS) {
         rt.left = WND_WIDTH - LONG_FIELD_WIDTH + SHORT_FIELD_WIDTH;
         rt.top = m_PaintCorret.top;
         rt.right = WND_WIDTH - LINE_X_OFFSET - SHORT_FIELD_WIDTH;
         rt.bottom = m_PaintCorret.bottom;
         DrawText(hdc, L"to", -1, &rt, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
      }
   }

   EndPaint(m_Wnd, &ps);
}

void SetupRecordWnd::ResetCorret() {
   m_PaintCorret.left = LINE_X_OFFSET;
   m_PaintCorret.top = TITLE_HEIGHT_WITH_OFFSET;
   m_PaintCorret.right = WND_WIDTH - LINE_X_OFFSET;
   m_PaintCorret.bottom = m_PaintCorret.top + LINE_HEIGHT;
}

void SetupRecordWnd::CorretNextLine() {
   m_PaintCorret.top = m_PaintCorret.bottom + LINE_Y_OFFSET;
   m_PaintCorret.bottom = m_PaintCorret.top + LINE_HEIGHT;
}

RecordErrorType SetupRecordWnd::TrySaveRecord() {
   Record record;
   wchar_t buffer[NAME_SIZE];
   GetWindowText(m_NameEdit, buffer, NAME_SIZE);
   memcpy(record.name, &buffer, sizeof(wchar_t) * NAME_SIZE);

   int index = SendMessage(m_IconCombo, CB_GETCURSEL, 0, 0);
   record.iconType = static_cast<IconType>(index);

   index = SendMessage(m_FoodCombo, CB_GETCURSEL, 0, 0);
   record.foodType = static_cast<FoodType>(index);

   GetWindowText(m_DoseIntegerEdit, buffer, NAME_SIZE);
   record.doseInteger = _wtoi(buffer);
   record.hasFractional = SendMessage(m_DoseFractionalCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
   if (record.hasFractional) {
      GetWindowText(m_DoseNumeratorEdit, buffer, NAME_SIZE);
      record.doseNumerator = _wtoi(buffer);
      GetWindowText(m_DoseDenominatorEdit, buffer, NAME_SIZE);
      record.doseDenominator = _wtoi(buffer);
   } else {
      record.doseNumerator = 0;
      record.doseDenominator = 0;
   }

   record.hasEndDate = SendMessage(m_EndDateCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
   if (record.hasEndDate) {
      SYSTEMTIME selectedDate;
      SendMessage(m_EndDatePicker, DTM_GETSYSTEMTIME, 0, (LPARAM) &selectedDate);
      record.endDateYear = selectedDate.wYear;
      record.endDateMonth = selectedDate.wMonth;
      record.endDateDay = selectedDate.wDay;
   } else {
      record.endDateYear = 0;
      record.endDateMonth = 0;
      record.endDateDay = 0;
   }

   index = SendMessage(m_TakingDayCombo, CB_GETCURSEL, 0, 0);
   record.takingDayType = static_cast<TakingDayType>(index);
   if (record.takingDayType != TakingDayType::EVERY_DAY) {
      SYSTEMTIME selectedDate;

      if (SendMessage(m_StartDateCheck, BM_GETCHECK, 0, 0) == BST_CHECKED) {
         selectedDate = m_TimeUtils->GetCurrentLocalTime();
      } else {
         SendMessage(m_StartDatePicker, DTM_GETSYSTEMTIME, 0, (LPARAM) &selectedDate);
      }

      record.startDateYear = selectedDate.wYear;
      record.startDateMonth = selectedDate.wMonth;
      record.startDateDay = selectedDate.wDay;

      if (record.takingDayType == TakingDayType::IN_N_DAYS) {
         GetWindowText(m_TakingDayPeriodEdit, buffer, NAME_SIZE);
         record.takingDayPeriod = _wtoi(buffer);
      } else {
         record.takingDayPeriod = 0;
      }
   } else {
      record.startDateYear = 0;
      record.startDateMonth = 0;
      record.startDateDay = 0;
      record.takingDayPeriod = 0;
   }

   index = SendMessage(m_TakingTimeCombo, CB_GETCURSEL, 0, 0);
   record.takingTimeType = static_cast<TakingTimeType>(index);
   if (record.takingTimeType != TakingTimeType::IN_ANY_TIME && record.takingTimeType != TakingTimeType::BEFORE_BED) {
      GetWindowText(m_TakingTimeFirstHour, buffer, NAME_SIZE);
      record.firstHour = _wtoi(buffer);

      if (record.takingTimeType == TakingTimeType::IN_BETWEEN_HOURS) {
         GetWindowText(m_TakingTimeSecondHour, buffer, NAME_SIZE);
         record.secondHour = _wtoi(buffer);
      } else {
         record.secondHour = 0;
      }
   } else {
      record.firstHour = 0;
      record.secondHour = 0;
   }

   RecordErrorType error = ValidateRecord(&record);
   if (error == RecordErrorType::NONE) {
      *m_EditingRecord = record;
   }

   return error;
}
