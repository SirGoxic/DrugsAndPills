#include "time_utils.h"
#include "timezoneapi.h"

static const long long nanoInSec = 10000000;//1e+7
static const long long nanoInMin = 60 * nanoInSec;
static const long long nanoInHour = 60 * nanoInMin;
static const long long nanoInDay = 24 * nanoInHour;

SYSTEMTIME TimeUtils::GetCurrentLocalTime() const {
#ifndef NDEBUG
   if (m_IsDebugTimeActive) {
      return m_DebugCurrentTime;
   } else {
      SYSTEMTIME localTime;
      GetLocalTime(&localTime);
      return localTime;
   }
#else
   SYSTEMTIME localTime;
   GetLocalTime(&localTime);
   return localTime;
#endif
}

SYSTEMTIME TimeUtils::AddDays(const SYSTEMTIME* timeStruct, int days) const {
   FILETIME ft;
   SystemTimeToFileTime(timeStruct, &ft);

   ULARGE_INTEGER large;
   memcpy(&large, &ft, sizeof(large));

   large.QuadPart += days * nanoInDay;

   memcpy(&ft, &large, sizeof(ft));

   SYSTEMTIME st;
   FileTimeToSystemTime(&ft, &st);
   return st;
}

SYSTEMTIME TimeUtils::CreateSysTime(int year, int month, int day) const {
   SYSTEMTIME time{year, month, 0, day, 0, 0, 0, 0};

   DOUBLE varTime;
   SystemTimeToVariantTime(&time, &varTime);
   VariantTimeToSystemTime(varTime, &time);

   return time;
}

bool TimeUtils::IsTimeLaterThanCurrent(const SYSTEMTIME* timeStruct) const {
   SYSTEMTIME localTime = GetCurrentLocalTime();

   FILETIME localFileTime, fileTime;
   SystemTimeToFileTime(&localTime, &localFileTime);
   SystemTimeToFileTime(timeStruct, &fileTime);

   return CompareFileTime(&localFileTime, &fileTime) > 0;
}

int TimeUtils::DaysDiff(const SYSTEMTIME* firstTime, const SYSTEMTIME* secondTime) const {
   FILETIME firstFile, secondFile;
   SystemTimeToFileTime(firstTime, &firstFile);
   SystemTimeToFileTime(secondTime, &secondFile);

   ULARGE_INTEGER firstLarge, secondLarge;
   memcpy(&firstLarge, &firstFile, sizeof(firstLarge));
   memcpy(&secondLarge, &secondFile, sizeof(secondLarge));

   ULARGE_INTEGER diffLarge;
   if (firstLarge.QuadPart > secondLarge.QuadPart) {
      diffLarge.QuadPart = firstLarge.QuadPart - secondLarge.QuadPart;
   } else {
      diffLarge.QuadPart = secondLarge.QuadPart - firstLarge.QuadPart;
   }

   int diff = diffLarge.QuadPart / nanoInDay;

   return firstLarge.QuadPart > secondLarge.QuadPart ? diff : -diff;
}

unsigned int TimeUtils::GetCurrentHour() const {
   SYSTEMTIME localTime = GetCurrentLocalTime();

   return localTime.wHour;
}

#ifndef NDEBUG

void TimeUtils::SetActiveDebugTime(bool debugTimeActive) {
   m_IsDebugTimeActive = debugTimeActive;
}

void TimeUtils::SetDebugTime(SYSTEMTIME debugTime) {
   m_DebugCurrentTime = debugTime;
}

#endif
