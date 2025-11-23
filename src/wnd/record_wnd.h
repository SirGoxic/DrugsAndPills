#pragma once
#include "wnd_base.h"
#include "record.h"

struct RecordWndCreateData : public WndCreateData {
   bool isWorkable = false;
};

struct RecordWndShowData : public WndShowData {

};

class RecordWnd : public WndBase {
public:

   using WndBase::WndBase;
   ~RecordWnd();

public:

   void Destroy(bool fromProc) override;

   void SetShorted(bool isShorted);

   void Collapse();
   void Expand();
   bool IsExpanded();

   void SetPos(POINT pos) override;

   int GetWidth();
   int GetHeight();

   void SetRecord(Record* record);

   void SetStatus(StatusType status);
   StatusType GetStatus();

private:

   static int IconTypeToImageIndex(IconType icon);
   static int FoodTypeToImageIndex(FoodType icon);
   static COLORREF StatusToColor(StatusType status);

private:

   HPEN m_BorderPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
   HFONT m_NameFontBig;
   HFONT m_NameFontSmall;

   bool m_IsWorkable = false;
   StatusType m_Status = StatusType::UPCOMING;
   bool m_IsExpanded = true;
   bool m_IsShorted = false;

   Record* m_Record = nullptr;

   HWND m_CollapseButton = nullptr;
   HWND m_DoneButton = nullptr;
   HWND m_EditButton = nullptr;
   HWND m_DeleteButton = nullptr;

private:

   void BeforeWndCreate(const WndCreateData& data) override;
   DWORD GetFlags() override;

   LRESULT CALLBACK MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

   void CreateView() override;

   void UpdateStatus();
   void UpdateCollapse();
};