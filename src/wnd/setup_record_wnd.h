#pragma once
#include "wnd_base.h"
#include "record.h"
#include "ui_consts.h"
#include "time_utils.h"
#include "settings.h"

struct SetupRecordWndCreateData : public WndCreateData {
   TimeUtils* timeUtils;
};

struct SetupRecordWndShowData : public WndShowData {
   Settings settings{};
   Record* record = nullptr;
   bool edit = false;
};

class SetupRecordWnd : public WndBase {
public:

   using WndBase::WndBase;
   ~SetupRecordWnd();

public:

   void Destroy(bool fromProc) override;

   void Show(const WndShowData& data) override;
   void Hide() override;

   POINT GetDefaultPos() override;

private:

   Settings m_Settings{};

   RECT m_PaintCorret{};

   TimeUtils* m_TimeUtils;

   HFONT m_CaptionFont = nullptr;
   HFONT m_TextFont = nullptr;

   Record* m_EditingRecord = nullptr;
   bool m_Edit = false;

   HWND m_NameEdit = nullptr;

   HWND m_IconCombo = nullptr;
   HWND m_FoodCombo = nullptr;

   HWND m_DoseIntegerEdit = nullptr;

   HWND m_DoseFractionalCheck = nullptr;
   HWND m_DoseNumeratorEdit = nullptr;
   HWND m_DoseDenominatorEdit = nullptr;

   HWND m_EndDateCheck = nullptr;
   HWND m_EndDatePicker = nullptr;

   HWND m_TakingDayCombo = nullptr;
   HWND m_StartDateCheck = nullptr;
   HWND m_StartDatePicker = nullptr;
   HWND m_TakingDayPeriodEdit = nullptr;

   HWND m_TakingTimeCombo = nullptr;
   HWND m_TakingTimeFirstHour = nullptr;
   HWND m_TakingTimeSecondHour = nullptr;

   HWND m_SaveButton = nullptr;
   HWND m_CancelButton = nullptr;
   HWND m_CloseButton = nullptr;

private:

   void BeforeWndCreate(const WndCreateData& data) override;
   DWORD GetFlags() override;

   LRESULT CALLBACK MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

   void CreateView() override;

   void LoadRecord();
   void ClearRecord();

   void CommandHandle(WPARAM wParam, LPARAM lParam);
   void NotifyHandle(WPARAM wParam, LPARAM lParam);
   void Paint();

   void ResetCorret();
   void CorretNextLine();

   RecordErrorType TrySaveRecord();
};

