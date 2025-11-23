#pragma once
#include <serializer.h>

#define NAME_SIZE 36

enum class RecordErrorType {
   NONE = 0,

   NAME_WRONG_LENGTH,

   ICON_OUT_OF_BOUNDS,

   FOOD_OUT_OF_BOUNDS,

   DOSE_NUM_INVALID,
   DOSE_DEN_INVALID,
   DOSE_DEN_LESS_NUM,

   END_DATE_MONTH_INVALID,
   END_DATE_DAY_INVALID,

   TAKING_DAY_OUT_OF_BOUNDS,
   TAKING_DAY_PERIOD_INVALID,
   START_DATE_MONTH_INVALID,
   START_DATE_DAY_INVALID,

   TAKING_TIME_OUT_OF_BOUNDS,
   TAKING_TIME_FIRST_HOUR_INVALID,
   TAKING_TIME_SECOND_HOUR_INVALID,
   TAKING_TIME_FIRST_MORE_SECOND
};

const wchar_t* RecordErrorTypeToString(RecordErrorType error);

enum class IconType {
   EMPTY = 0,
   TABLET,
   PILL,
   SYRINGE,
   CAPSULE,
   DROP,

   //Iteration helpers
   count,
   begin = 0,
   end = count - 1
};

const wchar_t* IconTypeToString(IconType icon);

enum class FoodType {
   EMPTY = 0,
   BEFORE_FOOD,
   WITH_FOOD,
   AFTER_FOOD,

   //Iteration helpers
   count,
   begin = 0,
   end = count - 1
};

const wchar_t* FoodTypeToString(FoodType food);

enum class TakingDayType {
   EVERY_DAY = 0,
   EVERY_OTHER_DAY,
   IN_N_DAYS,

   //Iteration helpers
   count,
   begin = 0,
   end = count - 1
};

const wchar_t* TakingDayTypeToString(TakingDayType dayType);

enum class TakingTimeType {
   IN_ANY_TIME = 0,
   BEFORE_HOUR,
   AFTER_HOUR,
   IN_BETWEEN_HOURS,
   BEFORE_BED,

   //Iteration helpers
   count,
   begin = 0,
   end = count - 1
};

const wchar_t* TakingTimeTypeToString(TakingTimeType timeType);

enum class StatusType {
   INVALID = -1,
   UPCOMING = 0,
   CURRENT,
   TO_LATE,
   DONE,
   END
};

struct Record {
   wchar_t name[NAME_SIZE] = L"\0";
   
   IconType iconType = IconType::EMPTY;
   
   FoodType foodType = FoodType::EMPTY;
   
   unsigned char doseInteger = 0;
   
   bool hasFractional = false;
   unsigned char doseNumerator = 0;
   unsigned char doseDenominator = 0;

   bool hasEndDate = false;
   unsigned int endDateYear = 0;
   unsigned char endDateMonth = 0;
   unsigned char endDateDay = 0;

   TakingDayType takingDayType = TakingDayType::EVERY_DAY;
   unsigned int startDateYear = 0;
   unsigned char startDateMonth = 0;
   unsigned char startDateDay = 0;
   unsigned int takingDayPeriod = 0;

   TakingTimeType takingTimeType = TakingTimeType::IN_ANY_TIME;
   unsigned char firstHour = 0;
   unsigned char secondHour = 0;

   Record() = default;
   Record(const Record& other);

   void Save(Serializer& serializer, const wchar_t* prefix);
   void Load(Serializer& serializer, const wchar_t* prefix);
};