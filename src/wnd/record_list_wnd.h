#pragma once
#include "wnd_base.h"
#include "record.h"
#include "record_wnd.h"
#include <vector>
#include "settings.h"

struct RecordListWndCreateData : public WndCreateData {
   bool isWorkable = false;
   std::vector<Record*>* records = nullptr;
};

struct RecordListWndShowData : public WndShowData {
};

class RecordListWnd : public WndBase {
public:

   using WndBase::WndBase;
   ~RecordListWnd();

public:

   void Create(const WndCreateData& data) override;
   void Destroy(bool fromProc) override;

   void TryAddRecord(Record* record);
   void TryRemoveRecord(Record* record);
   void RemoveAllRecords();

   void TryCollapseRecord(int index);
   bool GetRecordCollapse(int index);
   Record* TryGetRecord(int index);
   int TryGetRecordIndex(Record* record);
   int GetRecordsCount();

   void TrySetStatus(int index, StatusType status);
   StatusType TryGetStatus(int index);

   void SetShorted(bool isShorted);
   bool IsShorted();

   void SetSettings(Settings settings);

   bool IsExpanded();
   void SetExpanded(bool expanded);

private:

   bool m_IsWorkable = false;

   Settings m_Settings;

   std::vector<std::pair<Record*, RecordWnd*>> m_Records;

   HPEN m_BorderPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
   HFONT m_NameFontBig;

   bool m_IsExpanded = true;
   bool m_IsShorted = false;

   HWND m_CollapseButton = nullptr;
   HWND m_AllCollapseButton = nullptr;
   HWND m_AllExpandButton = nullptr;
   HWND m_AddButton = nullptr;

private:

   void BeforeWndCreate(const WndCreateData& data) override;
   DWORD GetFlags() override;

   LRESULT CALLBACK MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
   
   void CreateView() override;

   void UpdateSize();
   int GetWidth();

   POINT RecalculateSize();
};

