#pragma once
#include "wnd_base.h"
#include "record_list_wnd.h"
#include "record_wnd.h"
#include "setup_record_wnd.h"
#include "record.h"
#include "ui_consts.h"
#include "settings.h"
#include "serializer.h"

struct PanelWndCreateData : public WndCreateData {
   int mainHeight = 0;
   int clientHeight = 0;
   TimeUtils* timeUtils;
   Serializer* serializer;
   Settings settings;
};

struct PanelWndShowData : public WndShowData {

};

class PanelWnd : public WndBase {
public:

   using WndBase::WndBase;
   ~PanelWnd();

public:

   void Destroy(bool fromProc) override;

   void SetSettings(Settings settings);

   StatusType GetTodayStatus();

private:

   int m_MainHeight = 0;

   Settings m_Settings;
   TimeUtils* m_TimeUtils;
   SYSTEMTIME m_LastDate;
   Serializer* m_Serializer;

   POINT m_StartOffset{LINE_X_OFFSET, 0};

   Record* m_NewAddedRecord = nullptr;

   std::vector<std::pair<int, bool>> m_LastDayRecordsLoaded;
   std::vector<std::pair<int, std::pair<StatusType, bool>>> m_TodayRecordsLoaded;
   std::vector<int> m_EndedRecordsLoaded;
   std::vector<int> m_CollapsedRecordsLoaded;
   bool m_LastDayExpandedLoaded = true;
   bool m_TodayExpandedLoaded = true;
   bool m_AllExpandedLoaded = true;

   std::vector<Record*> m_Records;
   RecordListWnd* m_LastDayList = nullptr;
   RecordListWnd* m_TodayList = nullptr;
   RecordListWnd* m_AllRecordsList = nullptr;

   StatusType m_TodayStatus = StatusType::UPCOMING;

   int m_CurrentScrollPos = 0;
   HWND m_ScrollBar = nullptr;

private:

   void BeforeWndCreate(const WndCreateData& data) override;
   DWORD GetFlags() override;

   LRESULT CALLBACK MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;

   void CreateView() override;

   void UpdateScrollSize();
   void UpdateScrollPos();

   int GetClientHeight();

   void EndEditRecord(Record* record);
   void AddRecord(Record* record);
   void DeleteRecord(Record* record);

   void Update();
   void DateUpdate();
   void TimeUpdate();

   void SaveRecords();
   void LoadRecords();

public:
   void SaveState();
private:
   void LoadState();

};

