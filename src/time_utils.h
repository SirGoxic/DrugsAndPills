#pragma once
#include <Windows.h>

class TimeUtils {
public:
   TimeUtils() = default;
   ~TimeUtils() = default;

   SYSTEMTIME GetCurrentLocalTime() const;
   SYSTEMTIME AddDays(const SYSTEMTIME* timeStruct, int days) const;
   SYSTEMTIME CreateSysTime(int year, int month, int day) const;
   bool IsTimeLaterThanCurrent(const SYSTEMTIME* timeStruct) const;
   int DaysDiff(const SYSTEMTIME* firstTime, const SYSTEMTIME* secondTime) const;
   unsigned int GetCurrentHour() const;

#ifndef NDEBUG
public:

   void SetActiveDebugTime(bool debugTimeActive);
   void SetDebugTime(SYSTEMTIME debugTime);

private:

   bool m_IsDebugTimeActive;
   SYSTEMTIME m_DebugCurrentTime;

#endif
};