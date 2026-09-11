// Page6 cycle spray program logic.
#include "PersistentStorage.h"

namespace {

const int kPage6CycleRangeCount = 2;
const int kPage6MaxClockHour = 23;
const int kPage6MaxMinuteSecond = 59;
const int kPage6MaxDurationHour = 99;
const int kPage6DefaultIntervalDays = 0;
const int kPage6MinIntervalDays = 0;
const int kPage6MaxIntervalDays = 255;
const int kPage6DefaultCycleCount = 3;
const int kPage6MinCycleCount = 1;
const int kPage6MaxCycleCount = 99;
const char* kPage6IntervalDayText = "天";
const char* kPage6IntervalEverydayText = "天 每天";

enum EPage6CycleTimeField {
    PAGE6_START_HOUR = 0,
    PAGE6_START_MINUTE,
    PAGE6_END_HOUR,
    PAGE6_END_MINUTE,
};

enum EPage6DurationField {
    PAGE6_DURATION_HOUR = 0,
    PAGE6_DURATION_MINUTE,
    PAGE6_DURATION_SECOND,
};

struct SPage6CycleRange {
    bool enabled;
    bool startHourSet;
    bool startMinuteSet;
    bool endHourSet;
    bool endMinuteSet;
    int startHour;
    int startMinute;
    int endHour;
    int endMinute;
};

struct SPage6Duration {
    int hour;
    int minute;
    int second;
};

struct SPage6Program {
    bool cycleEnabled;
    SPage6CycleRange ranges[kPage6CycleRangeCount];
    int cycleCount;
    SPage6Duration irrigation;
    SPage6Duration soak;
    bool intervalDaysSet;
    int intervalDays;
};

SPage6Program sPage6Program;
bool sPage6Initialized = false;
bool sPage6UpdatingControls = false;
bool sPage6CycleTipVisible = false;

void page6AppendInt(std::string& text, int value) {
    char buffer[32] = {0};
    snprintf(buffer, sizeof(buffer), "%d", value);
    text += buffer;
}

void appendPage6ProgramTextInternal(std::string& text) {
    text += "version\t1\nprogram\t";
    text += sPage6Program.cycleEnabled ? "1" : "0";
    text += "\t";
    page6AppendInt(text, sPage6Program.cycleCount);
    text += "\t";
    text += sPage6Program.intervalDaysSet ? "1" : "0";
    text += "\t";
    page6AppendInt(text, sPage6Program.intervalDays);
    text += "\n";

    const SPage6Duration* durations[] = {
        &sPage6Program.irrigation,
        &sPage6Program.soak,
    };
    const char* durationNames[] = {"irrigation", "soak"};
    for (int i = 0; i < 2; ++i) {
        text += "duration\t";
        text += durationNames[i];
        text += "\t";
        page6AppendInt(text, durations[i]->hour);
        text += "\t";
        page6AppendInt(text, durations[i]->minute);
        text += "\t";
        page6AppendInt(text, durations[i]->second);
        text += "\n";
    }

    for (int i = 0; i < kPage6CycleRangeCount; ++i) {
        const SPage6CycleRange& range = sPage6Program.ranges[i];
        text += "range\t";
        page6AppendInt(text, i);
        text += "\t";
        text += range.enabled ? "1" : "0";
        text += "\t";
        text += range.startHourSet ? "1" : "0";
        text += "\t";
        text += range.startMinuteSet ? "1" : "0";
        text += "\t";
        text += range.endHourSet ? "1" : "0";
        text += "\t";
        text += range.endMinuteSet ? "1" : "0";
        text += "\t";
        page6AppendInt(text, range.startHour);
        text += "\t";
        page6AppendInt(text, range.startMinute);
        text += "\t";
        page6AppendInt(text, range.endHour);
        text += "\t";
        page6AppendInt(text, range.endMinute);
        text += "\n";
    }
}

bool loadPage6ProgramTextInternal(const std::string& text) {
    size_t start = 0;
    while (start <= text.size()) {
        const size_t end = text.find('\n', start);
        std::string line = text.substr(start, end == std::string::npos
                ? std::string::npos : end - start);
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.resize(line.size() - 1);
        }
        if (!line.empty()) {
            const std::vector<std::string> fields = cj96_persist::splitTabLine(line);
            if (fields.size() >= 5 && fields[0] == "program") {
                sPage6Program.cycleEnabled =
                        cj96_persist::parseBool(fields[1], sPage6Program.cycleEnabled);
                sPage6Program.cycleCount = cj96_persist::parseInt(
                        fields[2], sPage6Program.cycleCount,
                        kPage6MinCycleCount, kPage6MaxCycleCount);
                sPage6Program.intervalDaysSet =
                        cj96_persist::parseBool(fields[3], sPage6Program.intervalDaysSet);
                sPage6Program.intervalDays = cj96_persist::parseInt(
                        fields[4], sPage6Program.intervalDays,
                        kPage6MinIntervalDays, kPage6MaxIntervalDays);
            } else if (fields.size() >= 5 && fields[0] == "duration") {
                SPage6Duration* duration = NULL;
                if (fields[1] == "irrigation") {
                    duration = &sPage6Program.irrigation;
                } else if (fields[1] == "soak") {
                    duration = &sPage6Program.soak;
                }
                if (duration) {
                    duration->hour = cj96_persist::parseInt(
                            fields[2], duration->hour, 0, kPage6MaxDurationHour);
                    duration->minute = cj96_persist::parseInt(
                            fields[3], duration->minute, 0, kPage6MaxMinuteSecond);
                    duration->second = cj96_persist::parseInt(
                            fields[4], duration->second, 0, kPage6MaxMinuteSecond);
                }
            } else if (fields.size() >= 11 && fields[0] == "range") {
                const int index = cj96_persist::parseInt(
                        fields[1], -1, 0, kPage6CycleRangeCount - 1);
                if (index >= 0) {
                    SPage6CycleRange& range = sPage6Program.ranges[index];
                    range.enabled = cj96_persist::parseBool(fields[2], range.enabled);
                    range.startHourSet = cj96_persist::parseBool(fields[3], range.startHourSet);
                    range.startMinuteSet = cj96_persist::parseBool(fields[4], range.startMinuteSet);
                    range.endHourSet = cj96_persist::parseBool(fields[5], range.endHourSet);
                    range.endMinuteSet = cj96_persist::parseBool(fields[6], range.endMinuteSet);
                    range.startHour = cj96_persist::parseInt(
                            fields[7], range.startHour, 0, kPage6MaxClockHour);
                    range.startMinute = cj96_persist::parseInt(
                            fields[8], range.startMinute, 0, kPage6MaxMinuteSecond);
                    range.endHour = cj96_persist::parseInt(
                            fields[9], range.endHour, 0, kPage6MaxClockHour);
                    range.endMinute = cj96_persist::parseInt(
                            fields[10], range.endMinute, 0, kPage6MaxMinuteSecond);
                }
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return true;
}

bool loadPage6ProgramFromStorage() {
    std::string text;
    if (!cj96_persist::readTextFile(
            cj96_persist::configPath("page6_cycle.tsv"), text)) {
        return false;
    }
    return loadPage6ProgramTextInternal(text);
}

void persistPage6Program() {
    // The unified settings checkpoint owns disk writes. Editing callbacks only
    // update the in-memory cycle program.
}

int page6ClampInt(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

int page6ParseIntText(const std::string &text, int defaultValue) {
    if (text.empty()) {
        return defaultValue;
    }
    return atoi(text.c_str());
}

void page6SetEditTextText(ZKEditText* editText, const char* text) {
    if (editText) {
        editText->setText(text);
    }
}

void page6SetEditTextInt(ZKEditText* editText, int value) {
    if (editText) {
        editText->setText(value);
    }
}

void page6SetEditTextTwoDigits(ZKEditText* editText, int value) {
    if (!editText) {
        return;
    }

    char text[8] = {0};
    snprintf(text, sizeof(text), "%02d", value);
    editText->setText(text);
}

void page6InitProgram() {
    if (sPage6Initialized) {
        return;
    }

    sPage6Program.cycleEnabled = false;
    for (int i = 0; i < kPage6CycleRangeCount; ++i) {
        sPage6Program.ranges[i].enabled = false;
        sPage6Program.ranges[i].startHourSet = false;
        sPage6Program.ranges[i].startMinuteSet = false;
        sPage6Program.ranges[i].endHourSet = false;
        sPage6Program.ranges[i].endMinuteSet = false;
        sPage6Program.ranges[i].startHour = 0;
        sPage6Program.ranges[i].startMinute = 0;
        sPage6Program.ranges[i].endHour = 0;
        sPage6Program.ranges[i].endMinute = 0;
    }
    sPage6Program.cycleCount = kPage6DefaultCycleCount;
    sPage6Program.irrigation.hour = 0;
    sPage6Program.irrigation.minute = 0;
    sPage6Program.irrigation.second = 0;
    sPage6Program.soak.hour = 0;
    sPage6Program.soak.minute = 0;
    sPage6Program.soak.second = 0;
    sPage6Program.intervalDaysSet = true;
    sPage6Program.intervalDays = kPage6DefaultIntervalDays;
    (void)loadPage6ProgramFromStorage();
    sPage6Initialized = true;
}

SPage6CycleRange& page6CycleRange(int index) {
    if (index <= 0) {
        return sPage6Program.ranges[0];
    }
    return sPage6Program.ranges[1];
}

ZKEditText* page6CycleRangeEditText(int index, EPage6CycleTimeField field) {
    if (index <= 0) {
        switch (field) {
        case PAGE6_START_HOUR: return mCycle1HourStartEditTextPtr;
        case PAGE6_START_MINUTE: return mCycle1MinStartEditTextPtr;
        case PAGE6_END_HOUR: return mCycle1HourEndEditTextPtr;
        case PAGE6_END_MINUTE: return mCycle1MinEndEditTextPtr;
        default: return NULL;
        }
    }

    switch (field) {
    case PAGE6_START_HOUR: return mCycle2HourStartEditTextPtr;
    case PAGE6_START_MINUTE: return mCycle2MinStartEditTextPtr;
    case PAGE6_END_HOUR: return mCycle2HourEndEditTextPtr;
    case PAGE6_END_MINUTE: return mCycle2MinEndEditTextPtr;
    default: return NULL;
    }
}

int& page6CycleRangeFieldValue(SPage6CycleRange& range, EPage6CycleTimeField field) {
    switch (field) {
    case PAGE6_START_HOUR: return range.startHour;
    case PAGE6_START_MINUTE: return range.startMinute;
    case PAGE6_END_HOUR: return range.endHour;
    case PAGE6_END_MINUTE: return range.endMinute;
    default: return range.startHour;
    }
}

bool& page6CycleRangeFieldSet(SPage6CycleRange& range, EPage6CycleTimeField field) {
    switch (field) {
    case PAGE6_START_HOUR: return range.startHourSet;
    case PAGE6_START_MINUTE: return range.startMinuteSet;
    case PAGE6_END_HOUR: return range.endHourSet;
    case PAGE6_END_MINUTE: return range.endMinuteSet;
    default: return range.startHourSet;
    }
}

int page6CycleRangeFieldMax(EPage6CycleTimeField field) {
    return (field == PAGE6_START_HOUR || field == PAGE6_END_HOUR)
            ? kPage6MaxClockHour
            : kPage6MaxMinuteSecond;
}

bool page6CycleRangeHasAnyValue(const SPage6CycleRange& range) {
    return range.startHourSet
            || range.startMinuteSet
            || range.endHourSet
            || range.endMinuteSet;
}

void page6SyncCycleRangeFieldFromControl(int index, EPage6CycleTimeField field) {
    SPage6CycleRange& range = page6CycleRange(index);
    ZKEditText* editText = page6CycleRangeEditText(index, field);
    if (!editText) {
        return;
    }

    const std::string text = editText->getText();
    int& value = page6CycleRangeFieldValue(range, field);
    bool& isSet = page6CycleRangeFieldSet(range, field);
    if (text.empty()) {
        value = 0;
        isSet = false;
    } else {
        value = page6ClampInt(page6ParseIntText(text, value), 0, page6CycleRangeFieldMax(field));
        isSet = true;
    }
}

void page6SyncCycleRangeFromControls(int index) {
    for (int i = PAGE6_START_HOUR; i <= PAGE6_END_MINUTE; ++i) {
        page6SyncCycleRangeFieldFromControl(index, static_cast<EPage6CycleTimeField>(i));
    }
    page6CycleRange(index).enabled = page6CycleRangeHasAnyValue(page6CycleRange(index));
}

void page6UpdateCycleRangeControls(int index) {
    SPage6CycleRange& range = page6CycleRange(index);
    ZKButton* iconButton = (index <= 0) ? mButton25Ptr : mButton29Ptr;
    if (iconButton) {
        iconButton->setSelected(range.enabled);
    }

    for (int i = PAGE6_START_HOUR; i <= PAGE6_END_MINUTE; ++i) {
        const EPage6CycleTimeField field = static_cast<EPage6CycleTimeField>(i);
        if (page6CycleRangeFieldSet(range, field)) {
            page6SetEditTextTwoDigits(page6CycleRangeEditText(index, field),
                                      page6CycleRangeFieldValue(range, field));
        } else {
            page6SetEditTextText(page6CycleRangeEditText(index, field), "");
        }
    }
}

void page6UpdateDurationControls() {
    page6SetEditTextInt(mCycleCountEditTextPtr, sPage6Program.cycleCount);
    page6SetEditTextInt(mIrrigationHourEditTextPtr, sPage6Program.irrigation.hour);
    page6SetEditTextInt(mIrrigationMinEditTextPtr, sPage6Program.irrigation.minute);
    page6SetEditTextInt(mIrrigationSecEditTextPtr, sPage6Program.irrigation.second);
    page6SetEditTextInt(mSoakHourEditTextPtr, sPage6Program.soak.hour);
    page6SetEditTextInt(mSoakMinEditTextPtr, sPage6Program.soak.minute);
    page6SetEditTextInt(mSoakSecEditTextPtr, sPage6Program.soak.second);
    if (sPage6Program.intervalDaysSet) {
        page6SetEditTextInt(mIntervalEditTextPtr, sPage6Program.intervalDays);
    } else {
        page6SetEditTextText(mIntervalEditTextPtr, "");
    }
    if (mButton39Ptr) {
        mButton39Ptr->setText(
                sPage6Program.intervalDaysSet && sPage6Program.intervalDays == 0
                        ? kPage6IntervalEverydayText
                        : kPage6IntervalDayText);
    }
}

void page6SyncDurationFromControls(SPage6Duration& duration,
                                   ZKEditText* hourEditText,
                                   ZKEditText* minuteEditText,
                                   ZKEditText* secondEditText) {
    if (hourEditText) {
        duration.hour = page6ClampInt(page6ParseIntText(hourEditText->getText(), 0),
                                      0,
                                      kPage6MaxDurationHour);
    }
    if (minuteEditText) {
        duration.minute = page6ClampInt(page6ParseIntText(minuteEditText->getText(), 0),
                                        0,
                                        kPage6MaxMinuteSecond);
    }
    if (secondEditText) {
        duration.second = page6ClampInt(page6ParseIntText(secondEditText->getText(), 0),
                                        0,
                                        kPage6MaxMinuteSecond);
    }
}

void page6SyncProgramFromControls() {
    page6InitProgram();
    for (int i = 0; i < kPage6CycleRangeCount; ++i) {
        page6SyncCycleRangeFromControls(i);
    }

    if (mCycleCountEditTextPtr) {
        sPage6Program.cycleCount = page6ClampInt(
                page6ParseIntText(mCycleCountEditTextPtr->getText(), kPage6DefaultCycleCount),
                kPage6MinCycleCount,
                kPage6MaxCycleCount);
    }

    page6SyncDurationFromControls(sPage6Program.irrigation,
                                  mIrrigationHourEditTextPtr,
                                  mIrrigationMinEditTextPtr,
                                  mIrrigationSecEditTextPtr);
    page6SyncDurationFromControls(sPage6Program.soak,
                                  mSoakHourEditTextPtr,
                                  mSoakMinEditTextPtr,
                                  mSoakSecEditTextPtr);

    if (mIntervalEditTextPtr) {
        const std::string text = mIntervalEditTextPtr->getText();
        sPage6Program.intervalDaysSet = !text.empty();
        if (sPage6Program.intervalDaysSet) {
            sPage6Program.intervalDays = page6ClampInt(
                    page6ParseIntText(text, kPage6DefaultIntervalDays),
                    kPage6MinIntervalDays,
                    kPage6MaxIntervalDays);
        }
    }
}

void page6UpdateControls() {
    page6InitProgram();
    sPage6UpdatingControls = true;
    if (mCycleButtonPtr) {
        mCycleButtonPtr->setSelected(sPage6Program.cycleEnabled);
    }
    page6UpdateCycleRangeControls(0);
    page6UpdateCycleRangeControls(1);
    page6UpdateDurationControls();
    sPage6UpdatingControls = false;
}

void page6HideCycleTip() {
    if (mCycleTipTextViewPtr) {
        mCycleTipTextViewPtr->setVisible(false);
    }
    sPage6CycleTipVisible = false;
}

void page6ShowCycleTip(const char* text) {
    if (mCycleTipTextViewPtr) {
        mCycleTipTextViewPtr->setText(text);
        mCycleTipTextViewPtr->setVisible(true);
        sPage6CycleTipVisible = true;
    }
}

bool page6RangeHasInvalidTimeOrder(const SPage6CycleRange& range) {
    if (!range.startHourSet || !range.endHourSet) {
        return false;
    }

    const int startMinute = range.startHour * 60
            + (range.startMinuteSet ? range.startMinute : 0);
    const int endMinute = range.endHour * 60
            + (range.endMinuteSet ? range.endMinute : 0);
    return endMinute <= startMinute;
}

void page6ShowRangeOrderTip(int index) {
    char tip[64] = {0};
    snprintf(tip, sizeof(tip), "程序%d：开始时间要早于结束时间", index + 1);
    page6ShowCycleTip(tip);
}

bool hideCycleTipIfVisible() {
    if (!sPage6CycleTipVisible) {
        return false;
    }

    page6HideCycleTip();
    return true;
}

void page6AutoFillDurationByCycleCount();

void page6SetCycleRangeEnabled(int index, bool enabled) {
    page6InitProgram();
    SPage6CycleRange& range = page6CycleRange(index);
    range.enabled = enabled;
    page6AutoFillDurationByCycleCount();
    persistPage6Program();
    page6UpdateControls();
}

void page6ToggleCycleRange(int index) {
    page6InitProgram();
    page6SetCycleRangeEnabled(index, !page6CycleRange(index).enabled);
}

void page6HandleCycleRangeTextChanged(int index, EPage6CycleTimeField field,
                                      const std::string &text) {
    if (sPage6UpdatingControls) {
        return;
    }

    page6InitProgram();
    page6HideCycleTip();
    SPage6CycleRange& range = page6CycleRange(index);
    if (text.empty()) {
        page6CycleRangeFieldValue(range, field) = 0;
        page6CycleRangeFieldSet(range, field) = false;
        range.enabled = page6CycleRangeHasAnyValue(range);
    } else {
        int& value = page6CycleRangeFieldValue(range, field);
        value = page6ClampInt(page6ParseIntText(text, value), 0, page6CycleRangeFieldMax(field));
        page6CycleRangeFieldSet(range, field) = true;
        range.enabled = true;
        if (field == PAGE6_START_HOUR && !range.startMinuteSet) {
            range.startMinute = 0;
            range.startMinuteSet = true;
        }
        if (field == PAGE6_END_HOUR && !range.endMinuteSet) {
            range.endMinute = 0;
            range.endMinuteSet = true;
        }
    }
    if (page6RangeHasInvalidTimeOrder(range)) {
        page6ShowRangeOrderTip(index);
    }
    page6AutoFillDurationByCycleCount();
    persistPage6Program();
    page6UpdateControls();
}

int page6RangeSecondsForAutoFill(const SPage6CycleRange& range) {
    if (!range.startHourSet || !range.endHourSet) {
        return 0;
    }

    const int startMinute = range.startHour * 60 + (range.startMinuteSet ? range.startMinute : 0);
    const int endMinute = range.endHour * 60 + (range.endMinuteSet ? range.endMinute : 0);
    if (endMinute <= startMinute) {
        return 0;
    }

    return (endMinute - startMinute) * 60;
}

void page6SecondsToDuration(int seconds, SPage6Duration& duration) {
    if (seconds < 0) {
        seconds = 0;
    }
    duration.hour = page6ClampInt(seconds / 3600, 0, kPage6MaxDurationHour);
    seconds %= 3600;
    duration.minute = page6ClampInt(seconds / 60, 0, kPage6MaxMinuteSecond);
    duration.second = page6ClampInt(seconds % 60, 0, kPage6MaxMinuteSecond);
}

int page6DurationSeconds(const SPage6Duration& duration);

void page6AutoFillDurationByCycleCount() {
    int totalCycleSeconds = 0;
    for (int i = 0; i < kPage6CycleRangeCount; ++i) {
        totalCycleSeconds += page6RangeSecondsForAutoFill(sPage6Program.ranges[i]);
    }

    if (totalCycleSeconds <= 0 || sPage6Program.cycleCount <= 0) {
        return;
    }

    const int oneCycleSeconds = totalCycleSeconds / sPage6Program.cycleCount;
    if (oneCycleSeconds <= 0) {
        return;
    }

    const int irrigationSeconds = oneCycleSeconds / 2;
    const int soakSeconds = oneCycleSeconds - irrigationSeconds;
    page6SecondsToDuration(irrigationSeconds, sPage6Program.irrigation);
    page6SecondsToDuration(soakSeconds, sPage6Program.soak);
}

void page6SyncCycleCountByDuration() {
    int totalCycleSeconds = 0;
    for (int i = 0; i < kPage6CycleRangeCount; ++i) {
        totalCycleSeconds += page6RangeSecondsForAutoFill(sPage6Program.ranges[i]);
    }

    const int oneCycleSeconds = page6DurationSeconds(sPage6Program.irrigation)
            + page6DurationSeconds(sPage6Program.soak);
    if (totalCycleSeconds <= 0 || oneCycleSeconds <= 0) {
        return;
    }

    // A partial final interval is not a complete spray cycle.
    sPage6Program.cycleCount = page6ClampInt(totalCycleSeconds / oneCycleSeconds,
                                              kPage6MinCycleCount,
                                              kPage6MaxCycleCount);
}

void page6HandleCycleCountTextChanged(const std::string &text) {
    if (sPage6UpdatingControls) {
        return;
    }

    page6InitProgram();
    page6HideCycleTip();
    sPage6Program.cycleCount = page6ClampInt(
            page6ParseIntText(text, kPage6DefaultCycleCount),
            kPage6MinCycleCount,
            kPage6MaxCycleCount);
    page6AutoFillDurationByCycleCount();
    persistPage6Program();
    page6UpdateControls();
}

int& page6DurationFieldValue(SPage6Duration& duration, EPage6DurationField field) {
    switch (field) {
    case PAGE6_DURATION_HOUR: return duration.hour;
    case PAGE6_DURATION_MINUTE: return duration.minute;
    case PAGE6_DURATION_SECOND: return duration.second;
    default: return duration.hour;
    }
}

void page6HandleDurationTextChanged(SPage6Duration& duration, EPage6DurationField field,
                                    const std::string &text) {
    if (sPage6UpdatingControls) {
        return;
    }

    page6InitProgram();
    page6HideCycleTip();
    int& value = page6DurationFieldValue(duration, field);
    const int maxValue = (field == PAGE6_DURATION_HOUR)
            ? kPage6MaxDurationHour
            : kPage6MaxMinuteSecond;
    value = page6ClampInt(page6ParseIntText(text, 0), 0, maxValue);
    page6SyncCycleCountByDuration();
    persistPage6Program();
    page6UpdateControls();
}

void page6HandleIntervalTextChanged(const std::string &text) {
    if (sPage6UpdatingControls) {
        return;
    }

    page6InitProgram();
    page6HideCycleTip();
    if (text.empty()) {
        sPage6Program.intervalDaysSet = false;
    } else {
        sPage6Program.intervalDaysSet = true;
        sPage6Program.intervalDays = page6ClampInt(
                page6ParseIntText(text, kPage6DefaultIntervalDays),
                kPage6MinIntervalDays,
                kPage6MaxIntervalDays);
    }
    persistPage6Program();
    page6UpdateControls();
}

int page6DurationSeconds(const SPage6Duration& duration) {
    return ((duration.hour * 60) + duration.minute) * 60 + duration.second;
}

bool page6ValidateRangeForOk(int index, int &totalSeconds) {
    SPage6CycleRange& range = page6CycleRange(index);
    if (!page6CycleRangeHasAnyValue(range)) {
        range.enabled = false;
        return true;
    }

    const int programNo = index + 1;
    const bool hasStart = range.startHourSet || range.startMinuteSet;
    const bool hasEnd = range.endHourSet || range.endMinuteSet;
    char tip[64] = {0};
    if (!hasStart) {
        snprintf(tip, sizeof(tip), "请设置程序%d的开始时间", programNo);
        page6ShowCycleTip(tip);
        return false;
    }
    if (!hasEnd) {
        snprintf(tip, sizeof(tip), "请设置程序%d的结束时间", programNo);
        page6ShowCycleTip(tip);
        return false;
    }
    if (!range.startHourSet) {
        snprintf(tip, sizeof(tip), "请设置程序%d的开始时间", programNo);
        page6ShowCycleTip(tip);
        return false;
    }
    if (!range.endHourSet) {
        snprintf(tip, sizeof(tip), "请设置程序%d的结束时间", programNo);
        page6ShowCycleTip(tip);
        return false;
    }

    if (!range.startMinuteSet) {
        range.startMinute = 0;
        range.startMinuteSet = true;
    }
    if (!range.endMinuteSet) {
        range.endMinute = 0;
        range.endMinuteSet = true;
    }

    if (page6RangeHasInvalidTimeOrder(range)) {
        page6ShowRangeOrderTip(index);
        return false;
    }

    range.enabled = true;
    const int startMinute = range.startHour * 60 + range.startMinute;
    const int endMinute = range.endHour * 60 + range.endMinute;
    totalSeconds += (endMinute - startMinute) * 60;
    return true;
}

bool page6ValidateProgramForOk() {
    page6InitProgram();
    page6SyncProgramFromControls();

    int totalCycleSeconds = 0;
    for (int i = 0; i < kPage6CycleRangeCount; ++i) {
        if (!page6ValidateRangeForOk(i, totalCycleSeconds)) {
            page6UpdateControls();
            return false;
        }
    }

    if (totalCycleSeconds <= 0) {
        page6UpdateControls();
        page6ShowCycleTip("请设置开始和结束时间");
        return false;
    }

    const int irrigationSeconds = page6DurationSeconds(sPage6Program.irrigation);
    const int soakSeconds = page6DurationSeconds(sPage6Program.soak);
    const int workSeconds = irrigationSeconds + soakSeconds;
    if (irrigationSeconds <= 0 && soakSeconds <= 0) {
        page6UpdateControls();
        page6ShowCycleTip("请设置灌溉和浸泡时间");
        return false;
    }
    if (irrigationSeconds <= 0) {
        page6UpdateControls();
        page6ShowCycleTip("请设置灌溉时间");
        return false;
    }
    if (soakSeconds <= 0) {
        page6UpdateControls();
        page6ShowCycleTip("请设置浸泡时间");
        return false;
    }
    if (workSeconds > totalCycleSeconds) {
        page6UpdateControls();
        page6ShowCycleTip("灌溉和浸泡时间不能超过开始和结束时间");
        return false;
    }

    if (!sPage6Program.intervalDaysSet) {
        page6UpdateControls();
        page6ShowCycleTip("喷雾间隔不能为空。");
        return false;
    }

    if (sPage6Program.intervalDays < kPage6MinIntervalDays) {
        sPage6Program.intervalDays = kPage6DefaultIntervalDays;
    }

    sPage6Program.cycleEnabled = true;
    persistPage6Program();
    page6UpdateControls();
    page6HideCycleTip();
    return true;
}

}  // namespace

static void appendPage6SettingsText(std::string& text) {
    appendPage6ProgramTextInternal(text);
}

static bool loadPage6SettingsText(const std::string& text) {
    return loadPage6ProgramTextInternal(text);
}

static void disableCycleWindowProgram() {
    page6InitProgram();
    sPage6Program.cycleEnabled = false;
    persistPage6Program();
    page6UpdateControls();
    page6HideCycleTip();
}

static bool isCycleWindowProgramEnabled() {
    page6InitProgram();
    return sPage6Program.cycleEnabled;
}

static void onCycleWindowShow() {
    page6UpdateControls();
    page6HideCycleTip();
}

static void onCycleWindowHide() {
    page6HideCycleTip();
}

// Window6 is not present yet. Keep these as empty placeholders for future Window6 logic.
static void onPage6Show() {
}

static void onPage6Hide() {
}

static bool handleCycleWindowOkButton() {
    return page6ValidateProgramForOk();
}

static bool handlePage6ButtonClick_Button23(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button25(ZKButton *pButton) {
    page6ToggleCycleRange(0);
    return false;
}

static bool handlePage6ButtonClick_Button29(ZKButton *pButton) {
    page6ToggleCycleRange(1);
    return false;
}

static bool handlePage6ButtonClick_Button30(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button24(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button31(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button32(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button33(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button34(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button35(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button36(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button37(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button38(ZKButton *pButton) {
    return false;
}

static bool handlePage6ButtonClick_Button39(ZKButton *pButton) {
    return false;
}

static void handlePage6EditTextChanged_Cycle1HourStartEditText(const std::string &text) {
    page6HandleCycleRangeTextChanged(0, PAGE6_START_HOUR, text);
}

static void handlePage6EditTextChanged_Cycle1MinStartEditText(const std::string &text) {
    page6HandleCycleRangeTextChanged(0, PAGE6_START_MINUTE, text);
}

static void handlePage6EditTextChanged_Cycle1HourEndEditText(const std::string &text) {
    page6HandleCycleRangeTextChanged(0, PAGE6_END_HOUR, text);
}

static void handlePage6EditTextChanged_Cycle1MinEndEditText(const std::string &text) {
    page6HandleCycleRangeTextChanged(0, PAGE6_END_MINUTE, text);
}

static void handlePage6EditTextChanged_Cycle2HourStartEditText(const std::string &text) {
    page6HandleCycleRangeTextChanged(1, PAGE6_START_HOUR, text);
}

static void handlePage6EditTextChanged_Cycle2MinStartEditText(const std::string &text) {
    page6HandleCycleRangeTextChanged(1, PAGE6_START_MINUTE, text);
}

static void handlePage6EditTextChanged_Cycle2HourEndEditText(const std::string &text) {
    page6HandleCycleRangeTextChanged(1, PAGE6_END_HOUR, text);
}

static void handlePage6EditTextChanged_Cycle2MinEndEditText(const std::string &text) {
    page6HandleCycleRangeTextChanged(1, PAGE6_END_MINUTE, text);
}

static void handlePage6EditTextChanged_IrrigationHourEditText(const std::string &text) {
    page6HandleDurationTextChanged(sPage6Program.irrigation, PAGE6_DURATION_HOUR, text);
}

static void handlePage6EditTextChanged_IrrigationMinEditText(const std::string &text) {
    page6HandleDurationTextChanged(sPage6Program.irrigation, PAGE6_DURATION_MINUTE, text);
}

static void handlePage6EditTextChanged_IrrigationSecEditText(const std::string &text) {
    page6HandleDurationTextChanged(sPage6Program.irrigation, PAGE6_DURATION_SECOND, text);
}

static void handlePage6EditTextChanged_SoakHourEditText(const std::string &text) {
    page6HandleDurationTextChanged(sPage6Program.soak, PAGE6_DURATION_HOUR, text);
}

static void handlePage6EditTextChanged_SoakMinEditText(const std::string &text) {
    page6HandleDurationTextChanged(sPage6Program.soak, PAGE6_DURATION_MINUTE, text);
}

static void handlePage6EditTextChanged_SoakSecEditText(const std::string &text) {
    page6HandleDurationTextChanged(sPage6Program.soak, PAGE6_DURATION_SECOND, text);
}

static void handlePage6EditTextChanged_IntervalEditText(const std::string &text) {
    page6HandleIntervalTextChanged(text);
}

static void handlePage6EditTextChanged_CycleCountEditText(const std::string &text) {
    page6HandleCycleCountTextChanged(text);
}
