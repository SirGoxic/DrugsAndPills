#include "record_list_wnd.h"
#include "ui_consts.h"
#include "fonts.h"
#include "messages.h"
#include "image_library.h"

#define EXPAND_SYMBOL L"\u25A1"
#define COLLAPSE_SYMBOL L"_"
#define ADD_SYMBOL L"+"

#define BTN_COLLAPSE 1
#define BTN_ADD 2
#define BTN_ALL_COLLAPSE 3
#define BTN_ALL_EXPAND 4

RecordListWnd::~RecordListWnd() {
   Destroy(false);

   for (std::pair<Record*, RecordWnd*> record : m_Records) {
      record.first = nullptr;
      delete record.second;
   }
   m_Records.clear();
}

void RecordListWnd::Create(const WndCreateData& data) {
   WndBase::Create(data);

   int totalHeight = TITLE_HEIGHT + Y_OFFSET + LINE_Y_OFFSET;
   for (std::pair<Record*, RecordWnd*> record : m_Records) {
      RecordWndCreateData recordData{};
      recordData.parentWnd = m_Wnd;
      recordData.pos = {LINE_X_OFFSET, totalHeight};
      recordData.isWorkable = m_IsWorkable;
      record.second->Create(recordData);

      RecordWndShowData recordShowData{};
      record.second->Show(recordShowData);
      record.second->SetShorted(m_IsShorted);
      totalHeight += record.second->GetHeight() + LINE_Y_OFFSET;
   }

   if (m_Records.empty()) {
      totalHeight = TITLE_HEIGHT + Y_OFFSET + LINE_Y_OFFSET * 2 + IMAGE_SIZE;
   }

   MoveWindow(m_Wnd, data.pos.x, data.pos.y, LIST_WIDTH, totalHeight, true);
}

void RecordListWnd::Destroy(bool fromProc) {
   DeleteObject(m_BorderPen);
   DeleteObject(m_NameFontBig);

   for (std::pair<Record*, RecordWnd*> record : m_Records) {
      record.second->Destroy(false);
   }

   WndBase::Destroy(fromProc);
}

void RecordListWnd::TryAddRecord(Record* record) {
   for (std::vector<std::pair<Record*, RecordWnd*>>::iterator it = m_Records.begin(); it != m_Records.end(); it++) {
      if ((*it).first == record) {
         return;
      }
   }

   RecordWnd* recordWnd = new RecordWnd(m_Instance, L"");
   recordWnd->SetRecord(record);
   m_Records.push_back(std::pair(record, recordWnd));

   if (m_Wnd != nullptr && m_ParentWnd != nullptr) {
      int totalHeight = TITLE_HEIGHT + Y_OFFSET + LINE_Y_OFFSET;
      for (std::pair<Record*, RecordWnd*> recordPair : m_Records) {
         totalHeight += recordPair.second->GetHeight() + LINE_Y_OFFSET;
      }

      RecordWndCreateData recordData{};
      recordData.parentWnd = m_Wnd;
      recordData.pos = {LINE_X_OFFSET, totalHeight};
      recordData.isWorkable = m_IsWorkable;
      recordWnd->Create(recordData);

      if (m_IsExpanded) {
         RecordWndShowData recordShowData{};
         recordWnd->Show(recordShowData);
         recordWnd->SetShorted(m_IsShorted);

         if (m_Settings.createRecordCollapsed) {
            recordWnd->Collapse();
         }
      }

      UpdateSize();
   }
}

void RecordListWnd::TryRemoveRecord(Record* record) {
   for (std::vector<std::pair<Record*, RecordWnd*>>::iterator it = m_Records.begin(); it != m_Records.end(); it++) {
      if ((*it).first == record) {
         (*it).first = nullptr;
         delete (*it).second;
         m_Records.erase(it);
         break;
      }
   }

   UpdateSize();
}

void RecordListWnd::RemoveAllRecords() {
   for (std::vector<std::pair<Record*, RecordWnd*>>::iterator it = m_Records.begin(); it != m_Records.end(); it++) {
      (*it).first = nullptr;
      delete (*it).second;
   }
   m_Records.clear();

   UpdateSize();
}

void RecordListWnd::TryCollapseRecord(int index) {
   if (index >= 0 && index < m_Records.size()) {
      m_Records[index].second->Collapse();
      UpdateSize();
   }
}

bool RecordListWnd::GetRecordCollapse(int index) {
   if (index >= 0 && index < m_Records.size()) {
      return !m_Records[index].second->IsExpanded();
   }

   return false;
}

Record* RecordListWnd::TryGetRecord(int index) {
   if (index >= 0 && index < m_Records.size()) {
      return m_Records[index].first;
   }

   return nullptr;
}

int RecordListWnd::TryGetRecordIndex(Record* record) {
   for (std::vector<std::pair<Record*, RecordWnd*>>::iterator it = m_Records.begin(); it != m_Records.end(); it++) {
      if ((*it).first == record) {
         return std::distance(m_Records.begin(), it);
      }
   }
   return -1;
}

int RecordListWnd::GetRecordsCount() {
   return m_Records.size();
}

void RecordListWnd::TrySetStatus(int index, StatusType status) {
   if (index >= 0 && index < m_Records.size()) {
      m_Records[index].second->SetStatus(status);
   }
}

StatusType RecordListWnd::TryGetStatus(int index) {
   if (index >= 0 && index < m_Records.size()) {
      return m_Records[index].second->GetStatus();
   }
   return StatusType::INVALID;
}

void RecordListWnd::SetShorted(bool isShorted) {
   if (m_IsShorted == isShorted) {
      return;
   }

   m_IsShorted = isShorted;

   RECT rect{0};
   GetWindowRect(m_Wnd, &rect);
   MapWindowPoints(HWND_DESKTOP, m_ParentWnd, (LPPOINT) &rect, 2);
   MoveWindow(m_Wnd, rect.left, rect.top, GetWidth(), rect.bottom - rect.top, true);

   int baseX = GetWidth();
   if (m_CollapseButton) {
      GetWindowRect(m_CollapseButton, &rect);
      MapWindowPoints(HWND_DESKTOP, m_Wnd, (LPPOINT) &rect, 2);
      MoveWindow(m_CollapseButton, baseX - X_OFFSET * 2 - IMAGE_SIZE, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
   }

   if (m_AllCollapseButton) {
      GetWindowRect(m_AllCollapseButton, &rect);
      MapWindowPoints(HWND_DESKTOP, m_Wnd, (LPPOINT) &rect, 2);
      MoveWindow(m_AllCollapseButton, baseX - X_OFFSET * 4 - IMAGE_SIZE * 4, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
   }

   if (m_AllExpandButton) {
      GetWindowRect(m_AllExpandButton, &rect);
      MapWindowPoints(HWND_DESKTOP, m_Wnd, (LPPOINT) &rect, 2);
      MoveWindow(m_AllExpandButton, baseX - X_OFFSET * 6 - IMAGE_SIZE * 7, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
   }

   for (std::pair<Record*, RecordWnd*> record : m_Records) {
      record.second->SetShorted(m_IsShorted);
   }
}

bool RecordListWnd::IsShorted() {
   return m_IsShorted;
}

void RecordListWnd::SetSettings(Settings settings) {
   m_Settings = settings;

   if (m_IsWorkable && m_Settings.shouldClearDone) {
      std::vector<Record*> recordsToRemove(m_Records.size());
      for (std::pair<Record*, RecordWnd*> record : m_Records) {
         if (record.second->GetStatus() == StatusType::DONE) {
            recordsToRemove.push_back(record.first);
         }
      }

      for (Record* record : recordsToRemove) {
         TryRemoveRecord(record);
      }
   }
}

bool RecordListWnd::IsExpanded() {
   return m_IsExpanded;
}

void RecordListWnd::SetExpanded(bool expanded) {
   if (m_IsExpanded != expanded) {
      SendMessage(m_Wnd, WM_COMMAND, BTN_COLLAPSE, 0);
   }
}

void RecordListWnd::BeforeWndCreate(const WndCreateData& data) {
   auto castData = (const RecordListWndCreateData&) data;
   m_IsWorkable = castData.isWorkable;

   m_NameFontBig = CreateFontIndirectW(&g_ArialBig);

   m_StartWidth = LIST_WIDTH;
   m_StartHeight = TITLE_HEIGHT + Y_OFFSET + LINE_Y_OFFSET;

   if (castData.records) {
      for (int i = 0; i < castData.records->size(); i++) {
         Record* record = (*castData.records)[i];
         RecordWnd* recordWnd = new RecordWnd(m_Instance, L"");
         recordWnd->SetRecord(record);
         m_Records.push_back(std::pair(record, recordWnd));
      }
   }
}

DWORD RecordListWnd::GetFlags() {
   return WS_BORDER | WS_CHILD;
}

LRESULT RecordListWnd::MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
   switch (message) {
      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case BTN_COLLAPSE:
               m_IsExpanded = !m_IsExpanded;

               SetWindowTextW(m_CollapseButton, m_IsExpanded ? COLLAPSE_SYMBOL : EXPAND_SYMBOL);

               if (m_IsExpanded) {
                  for (std::pair<Record*, RecordWnd*> record : m_Records) {
                     RecordWndShowData recordShowData{};
                     record.second->Show(recordShowData);
                  }
               } else {
                  for (std::pair<Record*, RecordWnd*> record : m_Records) {
                     record.second->Hide();
                  }
               }

               UpdateSize();

               break;
            case BTN_ADD:
               SendMessage(m_ParentWnd, WM_ADD_RECORD, (WPARAM) this, (LPARAM) 0);
               break;
            case BTN_ALL_COLLAPSE:
               if (!m_IsExpanded) {
                  break;
               }

               for (std::pair<Record*, RecordWnd*> record : m_Records) {
                  record.second->Collapse();
               }
               UpdateSize();
               break;
            case BTN_ALL_EXPAND:
               if (!m_IsExpanded) {
                  break;
               }

               for (std::pair<Record*, RecordWnd*> record : m_Records) {
                  record.second->Expand();
               }
               UpdateSize();
               break;
            default:
               return DefWindowProc(m_Wnd, message, wParam, lParam);
         }
         break;
      case WM_DELETE_RECORD:
         TryRemoveRecord((Record*) wParam);
         SendMessage(m_ParentWnd, WM_DELETE_RECORD, wParam, lParam);
         break;
      case WM_EDIT_RECORD:
         SendMessage(m_ParentWnd, WM_EDIT_RECORD, wParam, lParam);
         break;
      case WM_EXPAND_RECORD:
         UpdateSize();
         break;
      case WM_RECORD_DONE:
         if (m_IsWorkable) {
            if (m_Settings.shouldClearDone) {
               TryRemoveRecord((Record*) wParam);
            }

            SendMessage(m_ParentWnd, WM_RECORD_DONE, 0, 0);
         }
         break;
      case WM_PAINT:
         {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_Wnd, &ps);

            if (m_IsWorkable) {
               DrawImage(hdc, {X_OFFSET, Y_OFFSET}, {IMAGE_SIZE, IMAGE_SIZE}, TODO_IMAGE);
            }

            SelectObject(hdc, m_NameFontBig);

            wchar_t str[512];

            swprintf_s(str, L"%s", m_Caption);
            RECT rt = {X_OFFSET * 4 + IMAGE_SIZE, Y_OFFSET, (int) (GetWidth() * 0.7) - X_OFFSET * 2, TITLE_HEIGHT - Y_OFFSET * 2};
            DrawText(hdc, str, wcslen(str), &rt, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

            if (m_IsExpanded) {
               SelectObject(hdc, m_BorderPen);
               MoveToEx(hdc, 0, TITLE_HEIGHT, nullptr);
               LineTo(hdc, GetWidth(), TITLE_HEIGHT);

               if (m_Records.empty()) {
                  rt.left = LINE_X_OFFSET;
                  rt.top = TITLE_HEIGHT + Y_OFFSET + LINE_Y_OFFSET;
                  rt.right = GetWidth() - LINE_X_OFFSET;
                  rt.bottom = rt.top + IMAGE_SIZE;

                  swprintf_s(str, L"Has no records yet.");
                  DrawText(hdc, str, wcslen(str), &rt, DT_LEFT | DT_SINGLELINE | DT_CENTER | DT_VCENTER);
               }
            }
         }

         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;
}

void RecordListWnd::CreateView() {
   m_CollapseButton = CreateWindowW(L"BUTTON", m_IsExpanded ? COLLAPSE_SYMBOL : EXPAND_SYMBOL, WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_TEXT, (int) LIST_WIDTH - X_OFFSET * 2 - IMAGE_SIZE, Y_OFFSET, IMAGE_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) BTN_COLLAPSE, m_Instance, nullptr);
   m_AllCollapseButton = CreateWindowW(L"BUTTON", L"Collapse All", WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_TEXT, (int) LIST_WIDTH - X_OFFSET * 4 - IMAGE_SIZE * 4, Y_OFFSET, WIDE_BUTTON_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) BTN_ALL_COLLAPSE, m_Instance, nullptr);
   m_AllExpandButton = CreateWindowW(L"BUTTON", L"Expand All", WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_TEXT, (int) LIST_WIDTH - X_OFFSET * 6 - IMAGE_SIZE * 7, Y_OFFSET, WIDE_BUTTON_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) BTN_ALL_EXPAND, m_Instance, nullptr);
   if (!m_IsWorkable) {
      m_AddButton = CreateWindowW(L"BUTTON", ADD_SYMBOL, WS_BORDER | WS_CHILD | WS_VISIBLE | BS_CENTER | BS_TEXT, X_OFFSET, Y_OFFSET, IMAGE_SIZE, IMAGE_SIZE, m_Wnd, (HMENU) BTN_ADD, m_Instance, nullptr);
   }
}

void RecordListWnd::UpdateSize() {
   POINT newSize = RecalculateSize();
   RECT rect{0};
   GetWindowRect(m_Wnd, &rect);
   MapWindowPoints(HWND_DESKTOP, m_ParentWnd, (LPPOINT) &rect, 2);
   MoveWindow(m_Wnd, rect.left, rect.top, newSize.x, newSize.y, true);

   SendMessage(m_ParentWnd, WM_SIZE_CHANGE_LIST, 0, 0);
}

int RecordListWnd::GetWidth() {
   return m_IsShorted ? LIST_WIDTH_SHORTED : LIST_WIDTH;
}

POINT RecordListWnd::RecalculateSize() {
   POINT size{0};
   if (m_IsExpanded) {
      int totalHeight = TITLE_HEIGHT + 1 + LINE_Y_OFFSET;
      for (std::pair<Record*, RecordWnd*> record : m_Records) {
         record.second->SetPos({LINE_X_OFFSET, totalHeight});
         totalHeight += record.second->GetHeight() + LINE_Y_OFFSET;
      }

      if (m_Records.empty()) {
         totalHeight = TITLE_HEIGHT_WITH_OFFSET + IMAGE_SIZE;
      }

      size.x = GetWidth();
      size.y = totalHeight + Y_OFFSET;
   } else {
      size.x = GetWidth();
      size.y = TITLE_HEIGHT + Y_OFFSET;
   }

   return size;
}
