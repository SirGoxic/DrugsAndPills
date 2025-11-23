#include "serializer.h"
#include <cwchar>

static const int BUFFER_SIZE = 256;

Serializer::~Serializer() {
   Close();
}

bool Serializer::TryOpenForSerialize(const wchar_t* file) {
   std::filesystem::path fullPath = file;
   CheckExisting(&fullPath);

   Close();

   m_SerializeStream = new std::wofstream(fullPath);

   if (m_SerializeStream->is_open()) {
      return true;
   } else {
      Close();
      return false;
   }
}

bool Serializer::TryOpenForDeserialize(const wchar_t* file) {
   std::filesystem::path fullPath = file;
   CheckExisting(&fullPath);

   Close();

   m_DeserializeStream = new std::wifstream(fullPath);

   if (m_DeserializeStream->is_open()) {
      CollectDeserializeMap();
      return true;
   } else {
      Close();
      return false;
   }
}

void Serializer::Close() {
   if (m_SerializeStream) {
      m_SerializeStream->close();
      delete m_SerializeStream;
      m_SerializeStream = nullptr;
   }
   
   if (m_DeserializeStream) {
      m_DeserializeStream->close();
      delete m_DeserializeStream;
      m_DeserializeStream = nullptr;

      m_DeserializeMap.clear();
   }
}

void Serializer::TryWriteInt(const wchar_t* name, int value) {
   if (!m_SerializeStream || !m_SerializeStream->is_open()) {
      return;
   }

   WriteNameValue(name, std::to_wstring(value).c_str());
}

void Serializer::TryWriteChar(const wchar_t* name, char value) {
   if (!m_SerializeStream || !m_SerializeStream->is_open()) {
      return;
   }

   WriteNameValue(name, std::to_wstring(value).c_str());
}

void Serializer::TryWriteBool(const wchar_t* name, bool value) {
   if (!m_SerializeStream || !m_SerializeStream->is_open()) {
      return;
   }

   WriteNameValue(name, std::to_wstring(value).c_str());
}

void Serializer::TryWriteString(const wchar_t* name, const wchar_t* value) {
   if (!m_SerializeStream || !m_SerializeStream->is_open()) {
      return;
   }

   WriteNameValue(name, value);
}

void Serializer::TryReadInt(const wchar_t* name, int* value) {
   if (m_DeserializeMap.empty() || !m_DeserializeMap.count(name)) {
      return;
   }

   if (IsNumber(m_DeserializeMap[name])) {
      *value = std::stoi(m_DeserializeMap[name]);
   }
}

void Serializer::TryReadChar(const wchar_t* name, char* value) {
   if (m_DeserializeMap.empty() || !m_DeserializeMap.count(name)) {
      return;
   }

   if (IsNumber(m_DeserializeMap[name])) {
      *value = (char)std::stoi(m_DeserializeMap[name]);
   }
}

void Serializer::TryReadBool(const wchar_t* name, bool* value) {
   if (m_DeserializeMap.empty() || !m_DeserializeMap.count(name)) {
      return;
   }

   if (IsNumber(m_DeserializeMap[name])) {
      *value = std::stoi(m_DeserializeMap[name]);
   }
}

void Serializer::TryReadString(const wchar_t* name, wchar_t* value, size_t maxCount) {
   if (m_DeserializeMap.empty() || !m_DeserializeMap.count(name)) {
      return;
   }

   wcscpy_s(value, maxCount, m_DeserializeMap[name].c_str());
}

void Serializer::CheckExisting(const std::filesystem::path* path) {
   if (!std::filesystem::is_directory(path->parent_path()) && !path->parent_path().empty()) {
      std::filesystem::create_directory(path->parent_path());
   }

   if (!std::filesystem::exists(*path)) {
      std::wofstream tempStream(*path);
   }
}

void Serializer::CollectDeserializeMap() {
   if (!m_DeserializeStream || !m_DeserializeStream->is_open()) {
      return;
   }

   m_DeserializeMap.clear();

   wchar_t str[BUFFER_SIZE];
   while (!m_DeserializeStream->eof()) {
      m_DeserializeStream->getline(str, BUFFER_SIZE - 1);
      size_t strLength = m_DeserializeStream->gcount();
      if (!strLength) {
         continue;
      }

      if (m_DeserializeStream->eof()) {
         strLength++;
      }

      wchar_t* equalPos = wcschr(str, L'=');
      if (!equalPos) {
         continue;
      }

      size_t equalIndex = std::distance(str, equalPos);

      wchar_t nameBuffer[BUFFER_SIZE];
      wcsncpy_s(nameBuffer, str, equalIndex);

      wchar_t* newEnd = std::remove(nameBuffer, std::end(nameBuffer), L' ');
      size_t removedCount = BUFFER_SIZE - std::distance(nameBuffer, newEnd);
      nameBuffer[equalIndex - removedCount] = L'\0';

      if (!wcsnlen_s(nameBuffer, BUFFER_SIZE)) {
         continue;
      }

      for (int i = 0; i < strLength - equalIndex - 1; i++) {
         str[i] = str[i + equalIndex + 1];
      }

      strLength = strLength - equalIndex - 1;

      if (!strLength) {
         continue;
      }

      wchar_t valueBuffer[BUFFER_SIZE];
      wcsncpy_s(valueBuffer, str, strLength);
      valueBuffer[strLength - 1] = L'\0';

      size_t spaceCount = 0;
      for (int i = 0; i < strLength; i++) {
         if (valueBuffer[i] == L' ') {
            spaceCount++;
         } else {
            break;
         }
      }
      for (int i = 0; i < strLength - spaceCount - 1; i++) {
         valueBuffer[i] = valueBuffer[i + spaceCount];
      }

      strLength -= spaceCount;

      spaceCount = 0;
      for (int i = strLength - 2; i >= 0; i--) {
         if (valueBuffer[i] == L' ') {
            spaceCount++;
         } else {
            break;
         }
      }

      strLength -= spaceCount;

      if (!strLength) {
         continue;
      }

      valueBuffer[strLength - 1] = L'\0';

      if (!wcsnlen_s(valueBuffer, BUFFER_SIZE)) {
         continue;
      }

      m_DeserializeMap.insert({nameBuffer, valueBuffer});
   }
}

void Serializer::WriteNameValue(const wchar_t* name, const wchar_t* value) {
   wchar_t result[BUFFER_SIZE];

   wcscpy_s(result, BUFFER_SIZE - 1, name);
   wcscat_s(result, BUFFER_SIZE - 1, L" = ");
   wcscat_s(result, BUFFER_SIZE - 1, value);

   size_t len = wcslen(result);
   result[len] = L'\n';

   m_SerializeStream->write(result, len + 1);
}

bool Serializer::IsNumber(const std::wstring& s) {
   return std::find_if(s.begin(), s.end(), [](wchar_t c) { return !std::isdigit(c); }) == s.end();
}
