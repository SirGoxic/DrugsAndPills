#pragma once
#include "wnd_base.h"
#include "panel_wnd.h"
#include "ui_consts.h"
#include <settings_wnd.h>
#include <setup_record_wnd.h>
#include <time_utils.h>
#include <notification_wnd.h>
#include "serializer.h"

struct MainWndCreateData : public WndCreateData {

};

struct MainWndShowData : public WndShowData {

};

class MainWnd : public WndBase {
public:

   using WndBase::WndBase;
   ~MainWnd();

public:

   void Destroy(bool fromProc) override;

   void Show(const WndShowData& data) override;

   POINT GetDefaultPos() override;

private:

   HMENU m_Menu;
   HMENU m_SubMenu;
   HICON m_Icon;
   HICON m_IconWarning;
   HICON m_IconFail;

   NotificationWnd* m_NotificationWnd = nullptr;

   PanelWnd* m_PanelWnd = nullptr;
   SettingsWnd* m_SettingsWnd = nullptr;
   SetupRecordWnd* m_SetupRecordWnd = nullptr;

   HWND m_SettingsButton = nullptr;
   HWND m_CloseButton = nullptr;

   HBITMAP m_SettingsBm = nullptr;

   HFONT m_CaptionFont = nullptr;

   NOTIFYICONDATA m_IconData;

   TimeUtils m_TimeUtils;

   Serializer m_Serializer;

private:

   void BeforeWndCreate(const WndCreateData& data) override;
   DWORD GetFlags() override;
   DWORD GetExFlags() override;

   LRESULT CALLBACK MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

   void CreateView() override;

   void UpdateStatus(StatusType status);

   void SaveSettings(Settings settings);
   Settings LoadSettings();
};

