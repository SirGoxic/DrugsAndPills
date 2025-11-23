#pragma once
#include "wnd_base.h"
#include "settings.h"

struct NotificationWndCreateData : public WndCreateData {

};

struct NotificationWndShowData : public WndShowData {

   Settings settings = {};
   bool isWarning = true;

};

class NotificationWnd : public WndBase {
public:

   using WndBase::WndBase;
   ~NotificationWnd();

public:

   void Destroy(bool fromProc) override;

   void Show(const WndShowData& data) override;
   void Hide() override;

   void SetIsWarning(bool isWarning);
   void ResetTimer(unsigned int time);

   POINT GetDefaultPos() override;

private:

   Settings m_Settings;

   bool m_IsWarning = false;

   HFONT m_CaptionFont = nullptr;
   HFONT m_TextFont = nullptr;

   HWND m_OkButton = nullptr;

private:

   void BeforeWndCreate(const WndCreateData& data) override;
   DWORD GetFlags() override;

   LRESULT CALLBACK MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

   void CreateView() override;
};

