#pragma once
#include <map>
#include <iostream>
#include <fstream>
#include "filesystem"
#include <string>

#define VAR_NAME(v) L#v

#define WRITE_INT(var)    TryWriteInt(VAR_NAME(var), var)
#define WRITE_CHAR(var)   TryWriteChar(VAR_NAME(var), var)
#define WRITE_BOOL(var)   TryWriteBool(VAR_NAME(var), var)
#define WRITE_STRING(var) TryWriteString(VAR_NAME(var), var)
#define WRITE_ENUM(var)   TryWriteInt(VAR_NAME(var), static_cast<int>(var))

#define READ_INT(var)           TryReadInt(VAR_NAME(var), (int*)&var)
#define READ_CHAR(var)          TryReadChar(VAR_NAME(var), (char*)&var)
#define READ_BOOL(var)          TryReadBool(VAR_NAME(var), &var)
#define READ_STRING(var, count) TryReadString(VAR_NAME(var), &var, count)
#define READ_ENUM(var)          TryReadInt(VAR_NAME(var), (int*)&var)

class Serializer {
public:

   Serializer() = default;
   ~Serializer();

   bool TryOpenForSerialize(const wchar_t* file);
   bool TryOpenForDeserialize(const wchar_t* file);
   void Close();

   void TryWriteInt(const wchar_t* name, int value);
   void TryWriteChar(const wchar_t* name, char value);
   void TryWriteBool(const wchar_t* name, bool value);
   void TryWriteString(const wchar_t* name, const wchar_t* value);

   void TryReadInt(const wchar_t* name, int* value);
   void TryReadChar(const wchar_t* name, char* value);
   void TryReadBool(const wchar_t* name, bool* value);
   void TryReadString(const wchar_t* name, wchar_t* value, size_t maxCount);

private:

   std::wofstream* m_SerializeStream = nullptr;
   std::wifstream* m_DeserializeStream = nullptr;

   std::map<std::wstring, std::wstring> m_DeserializeMap{};

private:

   void CheckExisting(const std::filesystem::path* path);

   void CollectDeserializeMap();

   void WriteNameValue(const wchar_t* name, const wchar_t* value);

   bool IsNumber(const std::wstring& s);
};
