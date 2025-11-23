#include "panel_wnd.h"
#include "ui_consts.h"
#include "messages.h"
#include "record_checker.h"
#include "files.h"

PanelWnd::~PanelWnd() {
   Destroy(false);

   delete m_LastDayList;
   delete m_TodayList;
   delete m_AllRecordsList;

   delete m_NewAddedRecord;
}

void PanelWnd::Destroy(bool fromProc) {
   m_LastDayList->Destroy(false);
   m_TodayList->Destroy(false);
   m_AllRecordsList->Destroy(false);

   DestroyWindow(m_ScrollBar);

   for (std::vector<Record*>::iterator it = m_Records.begin(); it != m_Records.end(); it++) {
      delete *it;
   }
   m_Records.clear();

   WndBase::Destroy(fromProc);
}

void PanelWnd::SetSettings(Settings settings) {
   m_Settings = settings;

   m_LastDayList->SetSettings(m_Settings);
   m_TodayList->SetSettings(m_Settings);
   m_AllRecordsList->SetSettings(m_Settings);

   if (!m_Settings.shouldSaveToLate && m_LastDayList->IsVisible()) {
      m_LastDayList->RemoveAllRecords();
      m_LastDayList->Hide();
      SendMessage(m_Wnd, WM_SIZE_CHANGE_LIST, 0, 0);
   }

   Update();
}

StatusType PanelWnd::GetTodayStatus() {
   return m_TodayStatus;
}

void PanelWnd::BeforeWndCreate(const WndCreateData& data) {
   auto castData = (const PanelWndCreateData&) data;

   m_LastDayList = new RecordListWnd(m_Instance, L"Last day");

   m_TodayList = new RecordListWnd(m_Instance, L"Today");

   m_AllRecordsList = new RecordListWnd(m_Instance, L"All Records");

   m_StartWidth = WND_WIDTH;
   m_StartHeight = castData.clientHeight;
   m_MainHeight = castData.mainHeight;
   m_TimeUtils = castData.timeUtils;
   m_Serializer = castData.serializer;
   m_Settings = castData.settings;

   LoadRecords();
   LoadState();
}

DWORD PanelWnd::GetFlags() {
   return WS_CHILD | WS_VISIBLE;
}

LRESULT PanelWnd::MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   switch (message) {
      case WM_VSCROLL:
         {
            int oldScroll = m_CurrentScrollPos;
            switch (LOWORD(wParam)) {
               case SB_THUMBPOSITION:
                  m_CurrentScrollPos = HIWORD(wParam);
                  break;
               case SB_THUMBTRACK:
                  m_CurrentScrollPos = HIWORD(wParam);
                  break;
               case SB_PAGEUP:
                  m_CurrentScrollPos -= m_StartHeight;
                  break;
               case SB_PAGEDOWN:
                  m_CurrentScrollPos += m_StartHeight;
                  break;
               case SB_LINEUP:
                  m_CurrentScrollPos -= IMAGE_SIZE;
                  break;
               case SB_LINEDOWN:
                  m_CurrentScrollPos += IMAGE_SIZE;
                  break;
            }

            int clientHeight = GetClientHeight();
            m_CurrentScrollPos = min(m_CurrentScrollPos, clientHeight - m_StartHeight);
            m_CurrentScrollPos = max(0, m_CurrentScrollPos);

            int delta = m_CurrentScrollPos - oldScroll;
            delta = -delta;
            if (delta) {
               ScrollWindow(m_Wnd, 0, delta, nullptr, nullptr);
            }

            if (LOWORD(wParam) != SB_THUMBTRACK) {
               UpdateScrollPos();
            }
         }

         break;
      case WM_KEYUP:
         switch (wParam) {
            case VK_PRIOR:
               SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, 0), 0);
               break;
            case VK_NEXT:
               SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);
               break;
            case VK_UP:
               SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
               break;
            case VK_DOWN:
               SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
               break;
         }
         break;
      case WM_MOUSEWHEEL:
         {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta > 0) {
               SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
            } else {
               SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
            }
         }
         break;
      case WM_EDIT_RECORD:
         if ((Record*) wParam) {
            SetupRecordWndShowData data{};
            data.settings = m_Settings;
            data.record = (Record*) wParam;
            data.edit = true;

            SendMessage(m_ParentWnd, WM_EDIT_RECORD, (WPARAM) &data, 0);
         }
         break;
      case WM_ADD_RECORD:
         m_NewAddedRecord = new Record();

         {
            SetupRecordWndShowData data{};
            data.settings = m_Settings;
            data.record = m_NewAddedRecord;
            data.edit = false;

            SendMessage(m_ParentWnd, WM_EDIT_RECORD, (WPARAM) &data, 0);
         }
         break;
      case WM_DELETE_RECORD:
         {
            Record* record = (Record*) wParam;
            DeleteRecord(record);
         }
         break;
      case WM_END_EDIT:
         if (m_NewAddedRecord) {
            if ((bool) wParam) {
               AddRecord(m_NewAddedRecord);
            } else {
               delete m_NewAddedRecord;
            }
            m_NewAddedRecord = nullptr;
         } else {
            if ((bool) wParam) {
               EndEditRecord((Record*) lParam);
            }
         }

         InvalidateRect(m_Wnd, nullptr, true);
         break;
      case WM_TICK_UPDATE:
         Update();
         break;
      case WM_RECORD_DONE:
         Update();
         break;
      case WM_SIZE_CHANGE_LIST:
         {
            bool isShorted = GetClientHeight() > m_StartHeight;

            if (m_TodayList->IsShorted() != isShorted) {
               if (!isShorted) {
                  ScrollWindow(m_Wnd, 0, m_CurrentScrollPos, nullptr, nullptr);
                  m_CurrentScrollPos = 0;
               }

               MoveWindow(m_Wnd, 0, m_MainHeight - m_StartHeight, (isShorted ? WND_WIDTH - IMAGE_SIZE - LINE_X_OFFSET : WND_WIDTH), m_StartHeight, true);
            }

            m_LastDayList->SetShorted(isShorted);
            m_TodayList->SetShorted(isShorted);
            m_AllRecordsList->SetShorted(isShorted);

            ShowWindow(m_ScrollBar, isShorted ? SW_SHOW : SW_HIDE);

            if (isShorted) {
               UpdateScrollSize();
            }

            int lastDayOffset = 0;
            if (m_LastDayList->IsVisible()) {
               lastDayOffset = m_LastDayList->GetSize().y + LINE_Y_OFFSET;
            }

            m_TodayList->SetPos({m_StartOffset.x, m_StartOffset.y + lastDayOffset - m_CurrentScrollPos});
            m_AllRecordsList->SetPos({m_StartOffset.x, m_StartOffset.y + m_TodayList->GetSize().y + LINE_Y_OFFSET + lastDayOffset - m_CurrentScrollPos});
         }

         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}

void PanelWnd::CreateView() {
   RecordListWndCreateData listData{};
   RecordListWndShowData listShowData{};
   listData.parentWnd = m_Wnd;
   listData.pos = m_StartOffset;
   listData.isWorkable = true;

   std::vector<Record*> bufferRecords;
   std::vector<bool> bufferRecordCollapses;
   if (m_LastDayRecordsLoaded.empty()) {
      listData.records = nullptr;
   } else {
      bufferRecords.reserve(m_LastDayRecordsLoaded.size());
      bufferRecordCollapses.reserve(m_LastDayRecordsLoaded.size());

      int currentIndex = 0;
      for (int i = 0; i < m_LastDayRecordsLoaded.size(); i++) {
         int recordIndex = m_LastDayRecordsLoaded[i].first;
         if (recordIndex >= 0 && recordIndex < m_Records.size()) {
            bufferRecords.emplace_back(m_Records[recordIndex]);
            bufferRecordCollapses.emplace_back(m_LastDayRecordsLoaded[i].second);
            currentIndex++;
         }
      }

      listData.records = &bufferRecords;
   }

   m_LastDayList->Create(listData);

   for (int i = 0; i < bufferRecords.size(); i++) {
      Record* record = bufferRecords[i];
      int recordIndex = m_LastDayList->TryGetRecordIndex(record);
      bool isCollapsed = bufferRecordCollapses[i];
      m_LastDayList->TrySetStatus(recordIndex, StatusType::TO_LATE);
      if (isCollapsed) {
         m_LastDayList->TryCollapseRecord(recordIndex);
      }
   }

   if (!bufferRecords.empty()) {
      m_LastDayList->Show(listShowData);
      m_LastDayList->SetExpanded(m_LastDayExpandedLoaded);

      listData.pos.y += m_LastDayList->GetSize().y + LINE_Y_OFFSET;
   }

   bufferRecords.clear();
   bufferRecordCollapses.clear();

   std::vector<std::pair<int, StatusType>> bufferStatuses;
   if (m_TodayRecordsLoaded.empty()) {
      listData.records = nullptr;
   } else {
      bufferRecords.reserve(m_TodayRecordsLoaded.size());
      bufferStatuses.reserve(m_TodayRecordsLoaded.size());
      bufferRecordCollapses.reserve(m_TodayRecordsLoaded.size());

      int currentIndex = 0;
      for (int i = 0; i < m_TodayRecordsLoaded.size(); i++) {
         int recordIndex = m_TodayRecordsLoaded[i].first;
         if (recordIndex >= 0 && recordIndex < m_Records.size()) {
            bufferRecords.emplace_back(m_Records[recordIndex]);
            bufferStatuses.emplace_back(currentIndex, m_TodayRecordsLoaded[i].second.first);
            bufferRecordCollapses.emplace_back(m_TodayRecordsLoaded[i].second.second);
            currentIndex++;
         }
      }

      listData.records = &bufferRecords;
   }

   m_TodayList->Create(listData);

   for (int i = 0; i < bufferStatuses.size(); i++) {
      int recordIndex = bufferStatuses[i].first;
      StatusType recordStatus = bufferStatuses[i].second;
      m_TodayList->TrySetStatus(recordIndex, recordStatus);
      bool isCollapsed = bufferRecordCollapses[i];
      if (isCollapsed) {
         m_TodayList->TryCollapseRecord(recordIndex);
      }
   }

   m_TodayList->Show(listShowData);
   m_TodayList->SetExpanded(m_TodayExpandedLoaded);

   bufferRecords.clear();
   bufferStatuses.clear();
   bufferRecordCollapses.clear();

   listData.pos.y += m_TodayList->GetSize().y + LINE_Y_OFFSET;
   listData.isWorkable = false;
   listData.records = m_Records.empty() ? nullptr : &m_Records;
   m_AllRecordsList->Create(listData);

   for (int i = 0; i < m_EndedRecordsLoaded.size(); i++) {
      int recordIndex = m_EndedRecordsLoaded[i];

      if (recordIndex >= 0 && recordIndex < m_Records.size()) {
         m_AllRecordsList->TrySetStatus(recordIndex, StatusType::END);
      }
   }

   for (int i = 0; i < m_CollapsedRecordsLoaded.size(); i++) {
      int recordIndex = m_CollapsedRecordsLoaded[i];
      if (recordIndex >= 0 && recordIndex < m_Records.size()) {
         m_AllRecordsList->TryCollapseRecord(recordIndex);
      }
   }

   m_AllRecordsList->Show(listShowData);
   m_AllRecordsList->SetExpanded(m_AllExpandedLoaded);

   m_LastDayRecordsLoaded.clear();
   m_TodayRecordsLoaded.clear();
   m_EndedRecordsLoaded.clear();
   m_CollapsedRecordsLoaded.clear();

   m_ScrollBar = CreateWindow(L"SCROLLBAR", L"", WS_CHILD | SBS_VERT, WND_WIDTH - IMAGE_SIZE - LINE_X_OFFSET, m_MainHeight - m_StartHeight, IMAGE_SIZE, m_StartHeight - LINE_Y_OFFSET, m_ParentWnd, nullptr, m_Instance, nullptr);
   SCROLLINFO scrollInfo{0};
   scrollInfo.cbSize = sizeof(SCROLLINFO);
   scrollInfo.nMin = 0;
   scrollInfo.nMax = m_StartHeight;
   scrollInfo.nPos = m_CurrentScrollPos;
   scrollInfo.nPage = m_StartHeight;
   scrollInfo.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
   SetScrollInfo(m_ScrollBar, SB_CTL, &scrollInfo, true);

   SendMessage(m_ScrollBar, SBM_ENABLE_ARROWS, (WPARAM) ESB_ENABLE_BOTH, 0);

   Update();
   SendMessage(m_ParentWnd, WM_STATUS_UPDATE, 0, 0);

   SendMessage(m_Wnd, WM_SIZE_CHANGE_LIST, 0, 0);
}

void PanelWnd::UpdateScrollSize() {
   SCROLLINFO scrollInfo{0};
   scrollInfo.cbSize = sizeof(SCROLLINFO);

   int clientHeight = GetClientHeight();
   if (m_CurrentScrollPos > clientHeight - m_StartHeight) {
      int delta = m_CurrentScrollPos - (clientHeight - m_StartHeight);
      if (delta) {
         ScrollWindow(m_Wnd, 0, delta, nullptr, nullptr);
      }
      m_CurrentScrollPos = clientHeight - m_StartHeight;
   }

   scrollInfo.nMin = 0;
   scrollInfo.nMax = clientHeight;
   scrollInfo.nPos = m_CurrentScrollPos;
   scrollInfo.fMask = SIF_POS | SIF_RANGE;

   SetScrollInfo(m_ScrollBar, SB_CTL, &scrollInfo, true);
}

void PanelWnd::UpdateScrollPos() {
   SCROLLINFO scrollInfo{0};
   scrollInfo.cbSize = sizeof(SCROLLINFO);
   scrollInfo.nPos = m_CurrentScrollPos;
   scrollInfo.fMask = SIF_POS;
   SetScrollInfo(m_ScrollBar, SB_CTL, &scrollInfo, true);
}

int PanelWnd::GetClientHeight() {
   int lastDayOffset = 0;
   if (m_LastDayList->IsVisible()) {
      lastDayOffset = m_LastDayList->GetSize().y + LINE_Y_OFFSET;
   }
   return lastDayOffset + m_TodayList->GetSize().y + LINE_Y_OFFSET + m_AllRecordsList->GetSize().y + LINE_Y_OFFSET;
}

void PanelWnd::EndEditRecord(Record* record) {
   if (IsActiveRecord(record, m_TimeUtils)) {
      m_TodayList->TryAddRecord(record);
   } else {
      m_TodayList->TryRemoveRecord(record);
   }

   int recordIndex = m_AllRecordsList->TryGetRecordIndex(record);
   if (IsEndedRecord(record, m_TimeUtils)) {
      m_AllRecordsList->TrySetStatus(recordIndex, StatusType::END);
   } else {
      m_AllRecordsList->TrySetStatus(recordIndex, StatusType::UPCOMING);
   }

   Update();

   SaveRecords();
}

void PanelWnd::AddRecord(Record* record) {
   m_Records.push_back(record);
   m_AllRecordsList->TryAddRecord(record);

   if (IsActiveRecord(record, m_TimeUtils)) {
      m_TodayList->TryAddRecord(record);

      int index = m_TodayList->GetRecordsCount() - 1;
      StatusType newStatus = GetActiveRecordStatus(record, m_TimeUtils, StatusType::UPCOMING, m_Settings);
      m_TodayList->TrySetStatus(index, newStatus);
   }

   if (IsEndedRecord(record, m_TimeUtils)) {
      m_AllRecordsList->TrySetStatus(m_AllRecordsList->GetRecordsCount() - 1, StatusType::END);
   }

   Update();

   SaveRecords();
}

void PanelWnd::DeleteRecord(Record* record) {
   m_LastDayList->TryRemoveRecord(record);
   m_TodayList->TryRemoveRecord(record);
   for (std::vector<Record*>::iterator it = m_Records.begin(); it != m_Records.end(); it++) {
      if ((*it) == record) {
         delete record;
         m_Records.erase(it);
         break;
      }
   }

   Update();

   SaveRecords();
}

void PanelWnd::Update() {
   SYSTEMTIME lastTime = m_TimeUtils->GetCurrentLocalTime();

   if (m_LastDate.wDay != lastTime.wDay || m_LastDate.wMonth != lastTime.wMonth || m_LastDate.wYear != lastTime.wYear) {
      DateUpdate();
      m_LastDate = lastTime;
      SaveState();
   }

   TimeUpdate();
}

void PanelWnd::DateUpdate() {
   if (m_Settings.shouldSaveToLate) {
      m_LastDayList->RemoveAllRecords();
      for (int i = 0; i < m_TodayList->GetRecordsCount(); i++) {
         StatusType status = m_TodayList->TryGetStatus(i);
         if (status != StatusType::INVALID && status != StatusType::DONE) {
            m_LastDayList->TryAddRecord(m_TodayList->TryGetRecord(i));
            int newIndex = m_LastDayList->GetRecordsCount() - 1;
            m_LastDayList->TrySetStatus(newIndex, StatusType::TO_LATE);
         }
      }

      if (m_LastDayList->GetRecordsCount() > 0) {
         RecordListWndShowData listShowData{};
         m_LastDayList->Show(listShowData);
      } else {
         m_LastDayList->Hide();
         SendMessage(m_Wnd, WM_SIZE_CHANGE_LIST, 0, 0);
      }
   }

   m_TodayList->RemoveAllRecords();

   for (std::vector<Record*>::iterator it = m_Records.begin(); it != m_Records.end(); it++) {
      if (IsActiveRecord((*it), m_TimeUtils)) {
         m_TodayList->TryAddRecord((*it));
      }
   }

   for (int i = 0; i < m_AllRecordsList->GetRecordsCount(); i++) {
      Record* record = m_AllRecordsList->TryGetRecord(i);
      if (IsEndedRecord(record, m_TimeUtils)) {
         m_AllRecordsList->TrySetStatus(i, StatusType::END);
      } else {
         m_AllRecordsList->TrySetStatus(i, StatusType::UPCOMING);
      }
   }
}

void PanelWnd::TimeUpdate() {
   if (m_Settings.shouldSaveToLate) {
      if (m_LastDayList->GetRecordsCount() == 0) {
         m_LastDayList->Hide();
         SendMessage(m_Wnd, WM_SIZE_CHANGE_LIST, 0, 0);
      }
   }

   bool shouldSaveState = false;
   StatusType newTodayStatus = StatusType::UPCOMING;
   for (int i = 0; i < m_TodayList->GetRecordsCount(); i++) {
      StatusType status = m_TodayList->TryGetStatus(i);
      if (status == StatusType::INVALID) {
         continue;
      }

      StatusType newStatus = GetActiveRecordStatus(m_TodayList->TryGetRecord(i), m_TimeUtils, status, m_Settings);

      if (!shouldSaveState && newStatus != status) {
         shouldSaveState = true;
      }

      m_TodayList->TrySetStatus(i, newStatus);

      if (newStatus == StatusType::CURRENT && newTodayStatus != StatusType::TO_LATE) {
         newTodayStatus = StatusType::CURRENT;
      } else if (newStatus == StatusType::TO_LATE) {
         newTodayStatus = StatusType::TO_LATE;
      }
   }

   if (newTodayStatus != StatusType::TO_LATE) {
      for (int i = 0; i < m_LastDayList->GetRecordsCount(); i++) {
         StatusType status = m_LastDayList->TryGetStatus(i);
         if (status == StatusType::INVALID) {
            continue;
         }

         if (status == StatusType::TO_LATE) {
            newTodayStatus = StatusType::TO_LATE;
            break;
         }
      }
   }

   if (m_TodayStatus != newTodayStatus) {
      m_TodayStatus = newTodayStatus;
      SendMessage(m_ParentWnd, WM_STATUS_UPDATE, 0, 0);
      shouldSaveState = true;
   }

   if (shouldSaveState) {
      SaveState();
   }
}

void PanelWnd::SaveRecords() {
   m_Serializer->TryOpenForSerialize(RECORDS_SAVE);

   int recordsCount = m_Records.size();
   m_Serializer->WRITE_INT(recordsCount);

   wchar_t prefix[128];
   for (int i = 0; i < recordsCount; i++) {
      Record* record = m_Records[i];
      wcscpy_s(prefix, L"record[");
      wcscat_s(prefix, std::to_wstring(i).c_str());
      wcscat_s(prefix, L"].");
      record->Save(*m_Serializer, prefix);
   }

   m_Serializer->Close();
}

void PanelWnd::LoadRecords() {
   if (!m_Records.empty()) {
      m_LastDayList->RemoveAllRecords();
      m_TodayList->RemoveAllRecords();
      for (std::vector<Record*>::iterator it = m_Records.begin(); it != m_Records.end(); it++) {
         delete *it;
      }
      m_Records.clear();
   }

   m_Serializer->TryOpenForDeserialize(RECORDS_SAVE);

   int recordsCount = 0;
   m_Serializer->READ_INT(recordsCount);

   wchar_t prefix[128];
   m_Records.reserve(recordsCount);
   for (int i = 0; i < recordsCount; i++) {
      Record* record = new Record();
      wcscpy_s(prefix, L"record[");
      wcscat_s(prefix, std::to_wstring(i).c_str());
      wcscat_s(prefix, L"].");
      record->Load(*m_Serializer, prefix);
      m_Records.emplace_back(record);
   }

   m_Serializer->Close();
}

void PanelWnd::SaveState() {
   m_Serializer->TryOpenForSerialize(STATE_SAVE);

   int lastYear = m_LastDate.wYear;
   int lastMonth = m_LastDate.wMonth;
   int lastDay = m_LastDate.wDay;
   m_Serializer->WRITE_INT(lastYear);
   m_Serializer->WRITE_INT(lastMonth);
   m_Serializer->WRITE_INT(lastDay);

   m_Serializer->WRITE_ENUM(m_TodayStatus);

   wchar_t prefix[128];
   wchar_t varName[128];
   auto CreateName = [&](const wchar_t* base) {
      wcscpy_s(varName, prefix);
      wcscat_s(varName, base);
   };

   if (m_Settings.shouldSaveToLate) {
      int lastDayRecordsCount = m_LastDayList->GetRecordsCount();
      m_Serializer->WRITE_INT(lastDayRecordsCount);

      for (int i = 0; i < lastDayRecordsCount; i++) {
         wcscpy_s(prefix, L"lastDayRecord[");
         wcscat_s(prefix, std::to_wstring(i).c_str());
         wcscat_s(prefix, L"].");

         Record* record = m_LastDayList->TryGetRecord(i);
         auto it = std::find(m_Records.begin(), m_Records.end(), record);
         int recordIndex = it - m_Records.begin();

         CreateName(VAR_NAME(recordIndex));
         m_Serializer->TryWriteInt(varName, recordIndex);

         bool isCollapsed = m_LastDayList->GetRecordCollapse(i);
         CreateName(VAR_NAME(isCollapsed));
         m_Serializer->TryWriteBool(varName, isCollapsed);
      }

      bool lastDayExpanded = m_LastDayList->IsExpanded();
      m_Serializer->WRITE_BOOL(lastDayExpanded);
   }

   int todayRecordsCount = m_TodayList->GetRecordsCount();
   m_Serializer->WRITE_INT(todayRecordsCount);

   for (int i = 0; i < todayRecordsCount; i++) {
      wcscpy_s(prefix, L"todayRecord[");
      wcscat_s(prefix, std::to_wstring(i).c_str());
      wcscat_s(prefix, L"].");

      Record* record = m_TodayList->TryGetRecord(i);
      auto it = std::find(m_Records.begin(), m_Records.end(), record);
      int recordIndex = it - m_Records.begin();

      CreateName(VAR_NAME(recordIndex));
      m_Serializer->TryWriteInt(varName, recordIndex);

      StatusType recordStatus = m_TodayList->TryGetStatus(i);
      CreateName(VAR_NAME(recordStatus));
      m_Serializer->TryWriteInt(varName, static_cast<int>(recordStatus));

      bool isCollapsed = m_TodayList->GetRecordCollapse(i);
      CreateName(VAR_NAME(isCollapsed));
      m_Serializer->TryWriteBool(varName, isCollapsed);
   }

   bool todayExpanded = m_TodayList->IsExpanded();
   m_Serializer->WRITE_BOOL(todayExpanded);

   int endedRecordsCount = 0;
   int collapsedRecordsCount = 0;
   for (int i = 0; i < m_AllRecordsList->GetRecordsCount(); i++) {
      if (m_AllRecordsList->TryGetStatus(i) == StatusType::END) {
         wcscpy_s(prefix, L"endedRecord[");
         wcscat_s(prefix, std::to_wstring(endedRecordsCount).c_str());
         wcscat_s(prefix, L"]");

         m_Serializer->TryWriteInt(prefix, i);
         endedRecordsCount++;
      }

      if (m_AllRecordsList->GetRecordCollapse(i)) {
         wcscpy_s(prefix, L"collapsedRecord[");
         wcscat_s(prefix, std::to_wstring(collapsedRecordsCount).c_str());
         wcscat_s(prefix, L"]");

         m_Serializer->TryWriteInt(prefix, i);
         collapsedRecordsCount++;
      }
   }
   m_Serializer->WRITE_INT(endedRecordsCount);
   m_Serializer->WRITE_INT(collapsedRecordsCount);

   bool allExpanded = m_AllRecordsList->IsExpanded();
   m_Serializer->WRITE_BOOL(allExpanded);

   m_Serializer->Close();
}

void PanelWnd::LoadState() {
   m_Serializer->TryOpenForDeserialize(STATE_SAVE);

   int lastYear, lastMonth, lastDay;
   m_Serializer->READ_INT(lastYear);
   m_Serializer->READ_INT(lastMonth);
   m_Serializer->READ_INT(lastDay);

   m_LastDate = m_TimeUtils->CreateSysTime(lastYear, lastMonth, lastDay);

   m_Serializer->READ_ENUM(m_TodayStatus);

   m_TodayList->RemoveAllRecords();

   m_LastDayRecordsLoaded.clear();
   m_TodayRecordsLoaded.clear();
   m_EndedRecordsLoaded.clear();
   m_CollapsedRecordsLoaded.clear();

   wchar_t prefix[128];
   wchar_t varName[128];
   auto CreateName = [&](const wchar_t* base) {
      wcscpy_s(varName, prefix);
      wcscat_s(varName, base);
   };
   if (m_Settings.shouldSaveToLate) {
      int lastDayRecordsCount = 0;
      m_Serializer->READ_INT(lastDayRecordsCount);
      m_LastDayRecordsLoaded.reserve(lastDayRecordsCount);

      for (int i = 0; i < lastDayRecordsCount; i++) {
         wcscpy_s(prefix, L"lastDayRecord[");
         wcscat_s(prefix, std::to_wstring(i).c_str());
         wcscat_s(prefix, L"].");

         int recordIndex = -1;

         CreateName(VAR_NAME(recordIndex));
         m_Serializer->TryReadInt(varName, &recordIndex);
         if (recordIndex >= 0) {
            bool isCollapsed = false;
            CreateName(VAR_NAME(isCollapsed));
            m_Serializer->TryReadBool(varName, &isCollapsed);
            m_LastDayRecordsLoaded.emplace_back(recordIndex, isCollapsed);
         }
      }

      bool lastDayExpanded = true;
      m_Serializer->READ_BOOL(lastDayExpanded);
      m_LastDayExpandedLoaded = lastDayExpanded;
   }

   int todayRecordsCount = 0;
   m_Serializer->READ_INT(todayRecordsCount);
   m_TodayRecordsLoaded.reserve(todayRecordsCount);

   for (int i = 0; i < todayRecordsCount; i++) {
      wcscpy_s(prefix, L"todayRecord[");
      wcscat_s(prefix, std::to_wstring(i).c_str());
      wcscat_s(prefix, L"].");

      int recordIndex = -1;

      CreateName(VAR_NAME(recordIndex));
      m_Serializer->TryReadInt(varName, &recordIndex);
      if (recordIndex >= 0) {
         StatusType recordStatus = StatusType::INVALID;
         CreateName(VAR_NAME(recordStatus));
         m_Serializer->TryReadInt(varName, (int*) &recordStatus);

         if (recordStatus != StatusType::INVALID) {
            bool isCollapsed = false;
            CreateName(VAR_NAME(isCollapsed));
            m_Serializer->TryReadBool(varName, &isCollapsed);
            m_TodayRecordsLoaded.emplace_back(recordIndex, std::pair(recordStatus, isCollapsed));
         }
      }
   }

   bool todayExpanded = true;
   m_Serializer->READ_BOOL(todayExpanded);
   m_TodayExpandedLoaded = todayExpanded;

   int endedRecordsCount = 0;
   m_Serializer->READ_INT(endedRecordsCount);
   m_EndedRecordsLoaded.reserve(endedRecordsCount);
   for (int i = 0; i < endedRecordsCount; i++) {
      wcscpy_s(prefix, L"endedRecord[");
      wcscat_s(prefix, std::to_wstring(i).c_str());
      wcscat_s(prefix, L"]");

      int recordIndex = -1;
      m_Serializer->TryReadInt(prefix, &recordIndex);
      if (recordIndex >= 0) {
         m_EndedRecordsLoaded.emplace_back(recordIndex);
      }
   }

   int collapsedRecordsCount = 0;
   m_Serializer->READ_INT(collapsedRecordsCount);
   m_CollapsedRecordsLoaded.reserve(collapsedRecordsCount);
   for (int i = 0; i < collapsedRecordsCount; i++) {
      wcscpy_s(prefix, L"collapsedRecord[");
      wcscat_s(prefix, std::to_wstring(i).c_str());
      wcscat_s(prefix, L"]");

      int recordIndex = -1;
      m_Serializer->TryReadInt(prefix, &recordIndex);
      if (recordIndex >= 0) {
         m_CollapsedRecordsLoaded.emplace_back(recordIndex);
      }
   }

   bool allExpanded = true;
   m_Serializer->READ_BOOL(allExpanded);
   m_AllExpandedLoaded = allExpanded;

   m_Serializer->Close();
}
