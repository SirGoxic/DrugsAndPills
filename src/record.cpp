#include "record.h"
#include <cstring>

const wchar_t* RecordErrorTypeToString(RecordErrorType error) {
   switch (error) {
      case RecordErrorType::NONE:
         return L"None error";
         break;
      case RecordErrorType::NAME_WRONG_LENGTH:
         return L"Name is incorrect";
         break;
      case RecordErrorType::ICON_OUT_OF_BOUNDS:
         return L"Incorrect icon type";
         break;
      case RecordErrorType::FOOD_OUT_OF_BOUNDS:
         return L"Incorrect food type";
         break;
      case RecordErrorType::DOSE_NUM_INVALID:
         return L"Dose numertator should be more than 0";
         break;
      case RecordErrorType::DOSE_DEN_INVALID:
         return L"Dose denomirator should be more than 1";
         break;
      case RecordErrorType::DOSE_DEN_LESS_NUM:
         return L"Dose denomirator should be more than numerator";
         break;
      case RecordErrorType::END_DATE_MONTH_INVALID:
         return L"Incorrect end date month";
         break;
      case RecordErrorType::END_DATE_DAY_INVALID:
         return L"Incorrect end date day";
         break;
      case RecordErrorType::TAKING_DAY_OUT_OF_BOUNDS:
         return L"Incorrect Taking Day Type";
         break;
      case RecordErrorType::TAKING_DAY_PERIOD_INVALID:
         return L"Taking day period should be more than 0";
         break;
      case RecordErrorType::START_DATE_MONTH_INVALID:
         return L"Incorrect start date month";
         break;
      case RecordErrorType::START_DATE_DAY_INVALID:
         return L"Incorrect start date day";
         break;
      case RecordErrorType::TAKING_TIME_OUT_OF_BOUNDS:
         return L"Incorrect Taking Time Type";
         break;
      case RecordErrorType::TAKING_TIME_FIRST_HOUR_INVALID:
         return L"First taking time hour should be more or equal to 0 and less than 24";
         break;
      case RecordErrorType::TAKING_TIME_SECOND_HOUR_INVALID:
         return L"Second taking time hour should be more or equal to 0 and less than 24";
         break;
      case RecordErrorType::TAKING_TIME_FIRST_MORE_SECOND:
         return L"Second taking time hour should be more or equal to first hour";
         break;
      default:
         return L"Can't convert RecordErrorType to string";
   }
}

const wchar_t* IconTypeToString(IconType icon) {
   switch (icon) {
      case IconType::EMPTY:
         return L"None";
      case IconType::TABLET:
         return L"Tablet";
      case IconType::PILL:
         return L"Pill";
      case IconType::SYRINGE:
         return L"Syringe";
      case IconType::CAPSULE:
         return L"Capsule";
      case IconType::DROP:
         return L"Drop";
      default:
         return L"Can't convert IconType to string";
   }
}

const wchar_t* FoodTypeToString(FoodType food) {
   switch (food) {
      case FoodType::EMPTY:
         return L"Regardless";
      case FoodType::BEFORE_FOOD:
         return L"Before food";
      case FoodType::WITH_FOOD:
         return L"With food";
      case FoodType::AFTER_FOOD:
         return L"After food";
      default:
         return L"Can't convert FoodType to string";
   }
}

const wchar_t* TakingDayTypeToString(TakingDayType dayType) {
   switch (dayType) {
      case TakingDayType::EVERY_DAY:
         return L"Every day";
      case TakingDayType::EVERY_OTHER_DAY:
         return L"Every other day";
      case TakingDayType::IN_N_DAYS:
         return L"After certains numbers of days";
      default:
         return L"Can't convert TakingDayType to string";
   }
}

const wchar_t* TakingTimeTypeToString(TakingTimeType timeType) {
   switch (timeType) {
      case TakingTimeType::IN_ANY_TIME:
         return L"Any time";
      case TakingTimeType::BEFORE_HOUR:
         return L"Before certain hour";
      case TakingTimeType::AFTER_HOUR:
         return L"After certain hour";
      case TakingTimeType::IN_BETWEEN_HOURS:
         return L"In between hours";
      case TakingTimeType::BEFORE_BED:
         return L"Before bed";
      default:
         return L"Can't convert TakingTimeType to string";
   }
}

Record::Record(const Record& other) {
   std::memcpy(name, other.name, sizeof(wchar_t) * NAME_SIZE);
   iconType = other.iconType;
   foodType = other.foodType;
   doseInteger = other.doseInteger;
   hasFractional = other.hasFractional;
   doseNumerator = other.doseNumerator;
   doseDenominator = other.doseDenominator;
   hasEndDate = other.hasEndDate;
   endDateYear = other.endDateYear;
   endDateMonth = other.endDateMonth;
   endDateDay = other.endDateDay;
   takingDayType = other.takingDayType;
   startDateYear = other.startDateYear;
   startDateMonth = other.startDateMonth;
   startDateDay = other.startDateDay;
   takingDayPeriod = other.takingDayPeriod;
   takingTimeType = other.takingTimeType;
   firstHour = other.firstHour;
   secondHour = other.secondHour;
}

void Record::Save(Serializer& serializer, const wchar_t* prefix) {
   wchar_t buffer[256];

   auto CreateName = [&](const wchar_t* base) {
      wcscpy_s(buffer, prefix);
      wcscat_s(buffer, base);
   };

   CreateName(VAR_NAME(name));
   serializer.TryWriteString(buffer, name);

   CreateName(VAR_NAME(iconType));
   serializer.TryWriteInt(buffer, static_cast<int>(iconType));

   CreateName(VAR_NAME(foodType));
   serializer.TryWriteInt(buffer, static_cast<int>(foodType));

   CreateName(VAR_NAME(doseInteger));
   serializer.TryWriteChar(buffer, doseInteger);

   CreateName(VAR_NAME(hasFractional));
   serializer.TryWriteBool(buffer, hasFractional);

   CreateName(VAR_NAME(doseNumerator));
   serializer.TryWriteChar(buffer, doseNumerator);

   CreateName(VAR_NAME(doseDenominator));
   serializer.TryWriteChar(buffer, doseDenominator);

   CreateName(VAR_NAME(hasEndDate));
   serializer.TryWriteBool(buffer, hasEndDate);

   CreateName(VAR_NAME(endDateYear));
   serializer.TryWriteInt(buffer, endDateYear);

   CreateName(VAR_NAME(endDateMonth));
   serializer.TryWriteChar(buffer, endDateMonth);

   CreateName(VAR_NAME(endDateDay));
   serializer.TryWriteChar(buffer, endDateDay);

   CreateName(VAR_NAME(takingDayType));
   serializer.TryWriteInt(buffer, static_cast<int>(takingDayType));

   CreateName(VAR_NAME(startDateYear));
   serializer.TryWriteInt(buffer, startDateYear);

   CreateName(VAR_NAME(startDateMonth));
   serializer.TryWriteChar(buffer, startDateMonth);

   CreateName(VAR_NAME(startDateDay));
   serializer.TryWriteChar(buffer, startDateDay);

   CreateName(VAR_NAME(takingDayPeriod));
   serializer.TryWriteInt(buffer, takingDayPeriod);

   CreateName(VAR_NAME(takingTimeType));
   serializer.TryWriteInt(buffer, static_cast<int>(takingTimeType));

   CreateName(VAR_NAME(firstHour));
   serializer.TryWriteChar(buffer, firstHour);

   CreateName(VAR_NAME(secondHour));
   serializer.TryWriteChar(buffer, secondHour);
}

void Record::Load(Serializer& serializer, const wchar_t* prefix) {
   wchar_t buffer[256];

   auto CreateName = [&](const wchar_t* base) {
      wcscpy_s(buffer, prefix);
      wcscat_s(buffer, base);
   };

   CreateName(VAR_NAME(name));
   serializer.TryReadString(buffer, name, NAME_SIZE);

   CreateName(VAR_NAME(iconType));
   serializer.TryReadInt(buffer, (int*) &iconType);

   CreateName(VAR_NAME(foodType));
   serializer.TryReadInt(buffer, (int*) &foodType);

   CreateName(VAR_NAME(doseInteger));
   serializer.TryReadChar(buffer, (char*) &doseInteger);

   CreateName(VAR_NAME(hasFractional));
   serializer.TryReadBool(buffer, &hasFractional);

   CreateName(VAR_NAME(doseNumerator));
   serializer.TryReadChar(buffer, (char*) &doseNumerator);

   CreateName(VAR_NAME(doseDenominator));
   serializer.TryReadChar(buffer, (char*) &doseDenominator);

   CreateName(VAR_NAME(hasEndDate));
   serializer.TryReadBool(buffer, &hasEndDate);

   CreateName(VAR_NAME(endDateYear));
   serializer.TryReadInt(buffer, (int*) &endDateYear);

   CreateName(VAR_NAME(endDateMonth));
   serializer.TryReadChar(buffer, (char*) &endDateMonth);

   CreateName(VAR_NAME(endDateDay));
   serializer.TryReadChar(buffer, (char*) &endDateDay);

   CreateName(VAR_NAME(takingDayType));
   serializer.TryReadInt(buffer, (int*) &takingDayType);

   CreateName(VAR_NAME(startDateYear));
   serializer.TryReadInt(buffer, (int*) &startDateYear);

   CreateName(VAR_NAME(startDateMonth));
   serializer.TryReadChar(buffer, (char*) &startDateMonth);

   CreateName(VAR_NAME(startDateDay));
   serializer.TryReadChar(buffer, (char*) &startDateDay);

   CreateName(VAR_NAME(takingDayPeriod));
   serializer.TryReadInt(buffer, (int*) &takingDayPeriod);

   CreateName(VAR_NAME(takingTimeType));
   serializer.TryReadInt(buffer, (int*) &takingTimeType);

   CreateName(VAR_NAME(firstHour));
   serializer.TryReadChar(buffer, (char*) &firstHour);

   CreateName(VAR_NAME(secondHour));
   serializer.TryReadChar(buffer, (char*) &secondHour);
}
