#pragma once
#include "wnd_base.h"
#include "ui_consts.h"
#include "time_utils.h"
#include "settings.h"

struct SettingsWndCreateData : public WndCreateData {
   TimeUtils* timeUtils;
   Settings settings;
};

struct SettingsWndShowData : public WndShowData {

};

class SettingsWnd : public WndBase {
public:

   using WndBase::WndBase;
   ~SettingsWnd();

public:

   void Destroy(bool fromProc) override;

   void Show(const WndShowData& data) override;
   void Hide() override;

   Settings GetSettings();

   POINT GetDefaultPos() override;

private:

   Settings m_Settings{};

   TimeUtils* m_TimeUtils;

   RECT m_PaintCorret{};

   HFONT m_CaptionFont = nullptr;
   HFONT m_TextFont = nullptr;

   HWND m_CloseButton = nullptr;

   HWND m_WindowCornerCombo = nullptr;

   HWND m_CreateRecordCollapsedCheck = nullptr;

   HWND m_UpdateTimeEdit = nullptr;
   HWND m_BedTimeEdit = nullptr;
   HWND m_SaveToLateCheck = nullptr;
   HWND m_ClearDoneCheck = nullptr;

   HWND m_UseNotificationCheck = nullptr;
   HWND m_TemporaryNotificationCheck = nullptr;
   HWND m_NotificationTimeEdit = nullptr;
   HWND m_NotificationCornerCombo = nullptr;

#ifndef NDEBUG

   HWND m_DebugTimeCheck = nullptr;
   HWND m_DebugDatePicker = nullptr;
   HWND m_DebugTimePicker = nullptr;

#endif

private:

   void BeforeWndCreate(const WndCreateData& data) override;
   DWORD GetFlags() override;

   LRESULT CALLBACK MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

   void CreateView() override;

   void ResetCorret();
   void CorretNextLine();

   void SaveSettings();
};

