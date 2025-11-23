#include "record_checker.h"
#include <settings_wnd.h>

RecordErrorType ValidateRecord(const Record* record) {
   if (record->iconType < IconType::EMPTY || record->iconType > IconType::DROP) {
      return RecordErrorType::ICON_OUT_OF_BOUNDS;
   }

   if (record->foodType < FoodType::EMPTY || record->foodType > FoodType::AFTER_FOOD) {
      return RecordErrorType::FOOD_OUT_OF_BOUNDS;
   }

   if (record->hasFractional) {
      if (record->doseNumerator < 1) {
         return RecordErrorType::DOSE_NUM_INVALID;
      }

      if (record->doseDenominator < 2) {
         return RecordErrorType::DOSE_DEN_INVALID;
      }

      if (record->doseDenominator <= record->doseNumerator) {
         return RecordErrorType::DOSE_DEN_LESS_NUM;
      }
   }

   if (record->hasEndDate) {
      if (record->endDateMonth > 12) {
         return RecordErrorType::END_DATE_MONTH_INVALID;
      }

      if (record->endDateDay > 31 || (record->endDateMonth == 1 && record->endDateDay > 29)) {
         return RecordErrorType::END_DATE_DAY_INVALID;
      }
   }

   if (record->takingDayType < TakingDayType::EVERY_DAY || record->takingDayType > TakingDayType::IN_N_DAYS) {
      return RecordErrorType::TAKING_DAY_OUT_OF_BOUNDS;
   }

   if (record->takingDayType == TakingDayType::IN_N_DAYS && record->takingDayPeriod < 1) {
      return RecordErrorType::TAKING_DAY_PERIOD_INVALID;
   }

   if (record->startDateMonth > 12) {
      return RecordErrorType::START_DATE_MONTH_INVALID;
   }

   if (record->startDateDay > 31 || (record->startDateMonth == 1 && record->startDateDay > 29)) {
      return RecordErrorType::START_DATE_DAY_INVALID;
   }

   if (record->takingTimeType < TakingTimeType::IN_ANY_TIME || record->takingTimeType > TakingTimeType::BEFORE_BED) {
      return RecordErrorType::TAKING_TIME_OUT_OF_BOUNDS;
   }

   if (record->takingTimeType == TakingTimeType::BEFORE_HOUR || record->takingTimeType == TakingTimeType::AFTER_HOUR) {
      if (record->firstHour < 0 || record->firstHour > 24) {
         return RecordErrorType::TAKING_TIME_FIRST_HOUR_INVALID;
      }
   }

   if (record->takingTimeType == TakingTimeType::IN_BETWEEN_HOURS) {
      if (record->secondHour < 0 || record->secondHour > 24) {
         return RecordErrorType::TAKING_TIME_SECOND_HOUR_INVALID;
      }

      if (record->firstHour >= record->secondHour) {
         return RecordErrorType::TAKING_TIME_FIRST_MORE_SECOND;
      }
   }

   return RecordErrorType::NONE;
}

bool IsEndedRecord(const Record* record, const TimeUtils* timeUtils) {
   if (!record->hasEndDate) {
      return false;
   }

   SYSTEMTIME currentTime = timeUtils->GetCurrentLocalTime();

   if (currentTime.wDay > record->endDateDay && currentTime.wMonth >= record->endDateMonth && currentTime.wYear >= record->endDateYear) {
      return true;
   }

   if (currentTime.wMonth > record->endDateMonth && currentTime.wYear >= record->endDateYear) {
      return true;
   }

   if (currentTime.wYear > record->endDateYear) {
      return true;
   }

   return false;
}

bool IsActiveRecord(const Record* record, const TimeUtils* timeUtils) {
   SYSTEMTIME currentTime = timeUtils->GetCurrentLocalTime();

   if (IsEndedRecord(record, timeUtils)) {
      return false;
   }

   if (record->takingDayType != TakingDayType::EVERY_DAY) {
      SYSTEMTIME startTime = timeUtils->CreateSysTime(record->startDateYear, record->startDateMonth, record->startDateDay);
      int dayDiff = timeUtils->DaysDiff(&currentTime, &startTime);
      if (record->takingDayType == TakingDayType::EVERY_OTHER_DAY) {
         if (dayDiff % 2) {
            return false;
         }
      } else if (record->takingDayType == TakingDayType::IN_N_DAYS) {
         if (dayDiff % (record->takingDayPeriod + 1)) {
            return false;
         }
      }
   }

   return true;
}

StatusType GetActiveRecordStatus(const Record* record, const TimeUtils* timeUtils, StatusType prevStatus, Settings settings) {
   if (prevStatus == StatusType::DONE || prevStatus == StatusType::INVALID) {
      return prevStatus;
   }

   if (record->takingTimeType == TakingTimeType::IN_ANY_TIME) {
      return StatusType::CURRENT;
   }

   unsigned int currentHour = timeUtils->GetCurrentHour();

   if (record->takingTimeType == TakingTimeType::BEFORE_BED) {
      if (currentHour >= settings.bedTime) {
         return StatusType::CURRENT;
      } else {
         return StatusType::UPCOMING;
      }
   }

   if (record->takingTimeType == TakingTimeType::AFTER_HOUR) {
      if (currentHour >= record->firstHour) {
         return StatusType::CURRENT;
      } else {
         return StatusType::UPCOMING;
      }
   }

   if (record->takingTimeType == TakingTimeType::BEFORE_HOUR) {
      if (currentHour < record->firstHour) {
         return StatusType::CURRENT;
      } else {
         return StatusType::TO_LATE;
      }
   }

   if (record->takingTimeType == TakingTimeType::IN_BETWEEN_HOURS) {
      if (currentHour >= record->firstHour && currentHour <= record->secondHour) {
         return StatusType::CURRENT;
      } else if (currentHour < record->firstHour) {
         return StatusType::UPCOMING;
      } else {
         return StatusType::TO_LATE;
      }
   }

   return StatusType::INVALID;
}
