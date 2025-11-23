#pragma once
#include "serializer.h"

enum class WindowCorner {
   RIGHT_DOWN = 0,
   LEFT_DOWN,
   LEFT_UP,
   RIGHT_UP,

   //Iteration helpers
   count,
   begin = 0,
   end = count - 1
};

const wchar_t* WindowCornerToString(WindowCorner corner);

struct Settings {
   WindowCorner mainWindowCorner = WindowCorner::RIGHT_DOWN;
   bool createRecordCollapsed = false;
   unsigned int updateTime = 300;
   unsigned int bedTime = 22;
   bool shouldSaveToLate = true;
   bool shouldClearDone = true;
   bool useNotification = true;
   WindowCorner notificationCorner = WindowCorner::RIGHT_DOWN;
   bool temporaryNotification = false;
   unsigned int notificationTime = 5;

   void Save(Serializer& serializer);
   void Load(Serializer& serializer);
};

