#pragma once

#include "record.h"
#include "time_utils.h"
#include "settings.h"

RecordErrorType ValidateRecord(const Record* record);

bool IsEndedRecord(const Record* record, const TimeUtils* timeUtils);
bool IsActiveRecord(const Record* record, const TimeUtils* timeUtils);
StatusType GetActiveRecordStatus(const Record* record, const TimeUtils* timeUtils, StatusType prevStatus, Settings settings);