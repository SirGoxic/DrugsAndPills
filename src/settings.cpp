#include "settings.h"

const wchar_t* WindowCornerToString(WindowCorner corner) {
   switch (corner) {
      case WindowCorner::RIGHT_DOWN:
         return L"Right Down";
      case WindowCorner::LEFT_DOWN:
         return L"Left Down";
      case WindowCorner::LEFT_UP:
         return L"Left Up";
      case WindowCorner::RIGHT_UP:
         return L"Right Up";
      default:
         return L"Can't convert WindowCorner to string";
   }
}

void Settings::Save(Serializer& serializer) {
   serializer.WRITE_ENUM(mainWindowCorner);
   serializer.WRITE_BOOL(createRecordCollapsed);
   serializer.WRITE_INT(updateTime);
   serializer.WRITE_INT(bedTime);
   serializer.WRITE_BOOL(shouldSaveToLate);
   serializer.WRITE_BOOL(shouldClearDone);
   serializer.WRITE_BOOL(useNotification);
   serializer.WRITE_ENUM(notificationCorner);
   serializer.WRITE_BOOL(temporaryNotification);
   serializer.WRITE_INT(notificationTime);
}

void Settings::Load(Serializer& serializer) {
   serializer.READ_ENUM(mainWindowCorner);
   serializer.READ_BOOL(createRecordCollapsed);
   serializer.READ_INT(updateTime);
   serializer.READ_INT(bedTime);
   serializer.READ_BOOL(shouldSaveToLate);
   serializer.READ_BOOL(shouldClearDone);
   serializer.READ_BOOL(useNotification);
   serializer.READ_ENUM(notificationCorner);
   serializer.READ_BOOL(temporaryNotification);
   serializer.READ_INT(notificationTime);
}
