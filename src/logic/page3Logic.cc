#include <cstdlib>
#include <cstdio>

namespace {

const int kPage3ProgramCount = 16;
const int kPage3StartTimeCount = 4;
const int kPage3MaxHour = 24;
const int kPage3MaxMinute = 59;

enum EPage3Weekday {
    PAGE3_SUNDAY = 0,
    PAGE3_MONDAY,
    PAGE3_TUESDAY,
    PAGE3_WEDNESDAY,
    PAGE3_THURSDAY,
    PAGE3_FRIDAY,
    PAGE3_SATURDAY,
    PAGE3_WEEKDAY_COUNT
};

struct SPage3StartTime {
    bool enabled;
    bool hourReady;
    bool minuteReady;
    int hour;
    int minute;
};

struct SPage3Program {
    bool enabled;
    bool weekMode;
    bool weekdays[PAGE3_WEEKDAY_COUNT];
    int irrCount;
    bool intervalDaysSet;
    bool afterDaysSet;
    int intervalDays;
    int afterDays;
    SPage3StartTime startTimes[kPage3StartTimeCount];
};

SPage3Program sPage3Programs[kPage3ProgramCount];
int sPage3CurrentProgram = 0;
bool sPage3Initialized = false;
bool sPage3UpdatingControls = false;

void updatePage3Controls();

int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

int parseIntText(const std::string &text, int defaultValue) {
    if (text.empty()) {
        return defaultValue;
    }
    return atoi(text.c_str());
}

void setEditTextInt(ZKEditText* editText, int value) {
    if (editText) {
        editText->setText(value);
    }
}

void setEditTextTwoDigits(ZKEditText* editText, int value) {
    if (!editText) {
        return;
    }

    char text[8] = {0};
    snprintf(text, sizeof(text), "%02d", value);
    editText->setText(text);
}

void setEditTextText(ZKEditText* editText, const char* text) {
    if (editText) {
        editText->setText(text);
    }
}

SPage3Program& currentPage3Program() {
    return sPage3Programs[sPage3CurrentProgram];
}

bool isEveryDaySelected(const SPage3Program& program) {
    for (int i = 0; i < PAGE3_WEEKDAY_COUNT; ++i) {
        if (!program.weekdays[i]) {
            return false;
        }
    }
    return true;
}

int getEnabledStartTimeCount(const SPage3Program& program) {
    int count = 0;
    for (int i = 0; i < kPage3StartTimeCount; ++i) {
        if (program.startTimes[i].hourReady && program.startTimes[i].minuteReady) {
            ++count;
        }
    }
    return count;
}

void syncPage3StartTimeEnabled(SPage3StartTime& startTime) {
    startTime.enabled = startTime.hourReady && startTime.minuteReady;
}

void resetPage3Program(SPage3Program& program) {
    program.enabled = false;
    program.weekMode = true;
    program.irrCount = 0;
    program.intervalDaysSet = false;
    program.afterDaysSet = false;
    program.intervalDays = 1;
    program.afterDays = 1;
    for (int i = 0; i < PAGE3_WEEKDAY_COUNT; ++i) {
        program.weekdays[i] = false;
    }
    for (int i = 0; i < kPage3StartTimeCount; ++i) {
        program.startTimes[i].enabled = false;
        program.startTimes[i].hourReady = false;
        program.startTimes[i].minuteReady = false;
        program.startTimes[i].hour = 0;
        program.startTimes[i].minute = 0;
    }
}

void initPage3Programs() {
    if (sPage3Initialized) {
        return;
    }

    for (int i = 0; i < kPage3ProgramCount; ++i) {
        resetPage3Program(sPage3Programs[i]);
    }
    sPage3CurrentProgram = 0;
    sPage3Initialized = true;
}

void updatePage3WeekButtons(const SPage3Program& program) {
    if (mWeekModeButtonPtr) {
        mWeekModeButtonPtr->setSelected(program.weekMode);
    }
    if (mIntervalModeButtonPtr) {
        mIntervalModeButtonPtr->setSelected(!program.weekMode);
    }
    if (mEverDayButtonPtr) {
        mEverDayButtonPtr->setSelected(isEveryDaySelected(program));
    }
    if (mSundayButtonPtr) {
        mSundayButtonPtr->setSelected(program.weekdays[PAGE3_SUNDAY]);
    }
    if (mMondayButtonPtr) {
        mMondayButtonPtr->setSelected(program.weekdays[PAGE3_MONDAY]);
    }
    if (mTuesdayButtonPtr) {
        mTuesdayButtonPtr->setSelected(program.weekdays[PAGE3_TUESDAY]);
    }
    if (mWednesdayButtonPtr) {
        mWednesdayButtonPtr->setSelected(program.weekdays[PAGE3_WEDNESDAY]);
    }
    if (mThursdayButtonPtr) {
        mThursdayButtonPtr->setSelected(program.weekdays[PAGE3_THURSDAY]);
    }
    if (mFridayButtonPtr) {
        mFridayButtonPtr->setSelected(program.weekdays[PAGE3_FRIDAY]);
    }
    if (mSaturdayButtonPtr) {
        mSaturdayButtonPtr->setSelected(program.weekdays[PAGE3_SATURDAY]);
    }
}

void updatePage3StartTimeControls(const SPage3Program& program) {
    ZKEditText* hourEditTexts[] = {
        mStartTimeHour1EditTextPtr,
        mStartTimeHour2EditTextPtr,
        mStartTimeHour3EditTextPtr,
        mStartTimeHour4EditTextPtr,
    };
    ZKEditText* minuteEditTexts[] = {
        mStartTimeMin1EditTextPtr,
        mStartTimeMin2EditTextPtr,
        mStartTimeMin3EditTextPtr,
        mStartTimeMin4EditTextPtr,
    };

    for (int i = 0; i < kPage3StartTimeCount; ++i) {
        if (program.startTimes[i].hourReady && program.startTimes[i].minuteReady) {
            setEditTextTwoDigits(hourEditTexts[i], program.startTimes[i].hour);
            setEditTextTwoDigits(minuteEditTexts[i], program.startTimes[i].minute);
        } else {
            setEditTextText(hourEditTexts[i], "--");
            setEditTextText(minuteEditTexts[i], "--");
        }
    }
}

void updatePage3ModeEditTexts(const SPage3Program& program) {
    if (program.weekMode) {
        setEditTextText(mIntervalDayEditTextPtr, "-");
        setEditTextText(mAfterDayEditTextPtr, "-");
        return;
    }

    if (program.intervalDaysSet) {
        setEditTextInt(mIntervalDayEditTextPtr, program.intervalDays);
    } else {
        setEditTextText(mIntervalDayEditTextPtr, "-");
    }

    if (program.afterDaysSet) {
        setEditTextInt(mAfterDayEditTextPtr, program.afterDays);
    } else {
        setEditTextText(mAfterDayEditTextPtr, "-");
    }
}

void updatePage3CycleProgramControls() {
    const bool visible = sPage3CurrentProgram == 0;
    if (mButton15Ptr) {
        mButton15Ptr->setVisible(visible);
    }
    if (mCycleButtonPtr) {
        mCycleButtonPtr->setVisible(visible);
    }
}

void revealPage3StartTime(int index) {
    if (index < 0 || index >= kPage3StartTimeCount) {
        return;
    }

    SPage3Program& program = currentPage3Program();
    SPage3StartTime& startTime = program.startTimes[index];
    if (startTime.hourReady && startTime.minuteReady) {
        return;
    }

    startTime.hourReady = true;
    startTime.minuteReady = true;
    syncPage3StartTimeEnabled(startTime);
    program.irrCount = getEnabledStartTimeCount(program);
    updatePage3Controls();
}

void activatePage3RangeField(ZKEditText* editText, int defaultValue, int currentValue) {
    if (!editText) {
        return;
    }

    const std::string text = editText->getText();
    if (text == "--" || text.empty() || text == "-" || text == "1~99" || text == "1~30") {
        const int valueToShow = currentValue > 0 ? currentValue : defaultValue;
        editText->setText(valueToShow);
    }
}

void updatePage3Controls() {
    initPage3Programs();

    SPage3Program& program = currentPage3Program();
    sPage3UpdatingControls = true;
    if (mShowProgEditTextPtr) {
        char text[16] = {0};
        snprintf(text, sizeof(text), "程序%d", sPage3CurrentProgram + 1);
        mShowProgEditTextPtr->setText(text);
    }
    if (mOnOffProgButtonPtr) {
        mOnOffProgButtonPtr->setSelected(program.enabled);
    }
    if (mOnOffProgTextButtonPtr) {
        mOnOffProgTextButtonPtr->setSelected(program.enabled);
    }
    program.irrCount = getEnabledStartTimeCount(program);

    updatePage3StartTimeControls(program);
    updatePage3WeekButtons(program);
    updatePage3ModeEditTexts(program);
    updatePage3CycleProgramControls();

    sPage3UpdatingControls = false;
}

void selectPage3Program(int programIndex) {
    initPage3Programs();
    if (programIndex < 0) {
        programIndex = kPage3ProgramCount - 1;
    } else if (programIndex >= kPage3ProgramCount) {
        programIndex = 0;
    }

    sPage3CurrentProgram = programIndex;
    updatePage3Controls();
}

void setPage3Weekday(EPage3Weekday weekday, bool selected) {
    SPage3Program& program = currentPage3Program();
    program.weekMode = true;
    program.weekdays[weekday] = selected;
    updatePage3Controls();
}

void setPage3StartHour(int index, const std::string &text) {
    if (sPage3UpdatingControls || index < 0 || index >= kPage3StartTimeCount) {
        return;
    }
    if (text.empty() || text == "--") {
        SPage3Program& program = currentPage3Program();
        program.startTimes[index].hourReady = false;
        program.startTimes[index].minuteReady = false;
        program.startTimes[index].hour = 0;
        program.startTimes[index].minute = 0;
        syncPage3StartTimeEnabled(program.startTimes[index]);
        program.irrCount = getEnabledStartTimeCount(program);
        updatePage3Controls();
        return;
    }

    SPage3Program& program = currentPage3Program();
    const int parsedValue = parseIntText(text, 0);
    const int normalizedValue = clampInt(parsedValue, 0, kPage3MaxHour);
    program.startTimes[index].hourReady = true;
    program.startTimes[index].hour = normalizedValue;
    syncPage3StartTimeEnabled(program.startTimes[index]);
    program.irrCount = getEnabledStartTimeCount(program);

    if (parsedValue != normalizedValue) {
        ZKEditText* hourEditTexts[] = {
            mStartTimeHour1EditTextPtr,
            mStartTimeHour2EditTextPtr,
            mStartTimeHour3EditTextPtr,
            mStartTimeHour4EditTextPtr,
        };
        if (hourEditTexts[index]) {
            sPage3UpdatingControls = true;
            setEditTextTwoDigits(hourEditTexts[index], normalizedValue);
            sPage3UpdatingControls = false;
        }
    }
    updatePage3Controls();
}

void setPage3StartMinute(int index, const std::string &text) {
    if (sPage3UpdatingControls || index < 0 || index >= kPage3StartTimeCount) {
        return;
    }
    if (text.empty() || text == "--") {
        SPage3Program& program = currentPage3Program();
        program.startTimes[index].hourReady = false;
        program.startTimes[index].minuteReady = false;
        program.startTimes[index].hour = 0;
        program.startTimes[index].minute = 0;
        syncPage3StartTimeEnabled(program.startTimes[index]);
        program.irrCount = getEnabledStartTimeCount(program);
        updatePage3Controls();
        return;
    }

    SPage3Program& program = currentPage3Program();
    const int parsedValue = parseIntText(text, 0);
    const int normalizedValue = clampInt(parsedValue, 0, kPage3MaxMinute);
    program.startTimes[index].minuteReady = true;
    program.startTimes[index].minute = normalizedValue;
    syncPage3StartTimeEnabled(program.startTimes[index]);
    program.irrCount = getEnabledStartTimeCount(program);

    if (parsedValue != normalizedValue) {
        ZKEditText* minuteEditTexts[] = {
            mStartTimeMin1EditTextPtr,
            mStartTimeMin2EditTextPtr,
            mStartTimeMin3EditTextPtr,
            mStartTimeMin4EditTextPtr,
        };
        if (minuteEditTexts[index]) {
            sPage3UpdatingControls = true;
            setEditTextTwoDigits(minuteEditTexts[index], normalizedValue);
            sPage3UpdatingControls = false;
        }
    }
    updatePage3Controls();
}

void togglePage3StartTime(int index) {
    if (index < 0 || index >= kPage3StartTimeCount) {
        return;
    }

    SPage3Program& program = currentPage3Program();
    SPage3StartTime& startTime = program.startTimes[index];
    if (startTime.hourReady && startTime.minuteReady) {
        startTime.hourReady = false;
        startTime.minuteReady = false;
    } else {
        if (!startTime.hourReady) {
            startTime.hour = 0;
        }
        if (!startTime.minuteReady) {
            startTime.minute = 0;
        }
        startTime.hourReady = true;
        startTime.minuteReady = true;
    }
    syncPage3StartTimeEnabled(startTime);
    program.irrCount = getEnabledStartTimeCount(program);
    updatePage3Controls();
}

void activatePage3IntervalDayField() {
    activatePage3RangeField(mIntervalDayEditTextPtr, 1, currentPage3Program().intervalDays);
}

void activatePage3AfterDayField() {
    activatePage3RangeField(mAfterDayEditTextPtr, 1, currentPage3Program().afterDays);
}

bool handlePage3EditTextClick(ZKBase *pBase) {
    if (!pBase) {
        return false;
    }

    switch (pBase->getID()) {
    case ID_MAIN_StartTimeHour1EditText:
    case ID_MAIN_StartTimeMin1EditText:
        revealPage3StartTime(0);
        return true;
    case ID_MAIN_StartTimeHour2EditText:
    case ID_MAIN_StartTimeMin2EditText:
        revealPage3StartTime(1);
        return true;
    case ID_MAIN_StartTimeHour3EditText:
    case ID_MAIN_StartTimeMin3EditText:
        revealPage3StartTime(2);
        return true;
    case ID_MAIN_StartTimeHour4EditText:
    case ID_MAIN_StartTimeMin4EditText:
        revealPage3StartTime(3);
        return true;
    case ID_MAIN_IntervalDayEditText:
        activatePage3IntervalDayField();
        return true;
    case ID_MAIN_AfterDayEditText:
        activatePage3AfterDayField();
        return true;
    default:
        return false;
    }
}

}  // namespace

static void onPage3Show() {
    updatePage3Controls();
}

static void onPage3Hide() {
}

static bool handlePage3ButtonClick_Button10(ZKButton *pButton) {
    togglePage3StartTime(0);
    return false;
}

static bool handlePage3ButtonClick_Button11(ZKButton *pButton) {
    togglePage3StartTime(1);
    return false;
}

static bool handlePage3ButtonClick_Button12(ZKButton *pButton) {
    togglePage3StartTime(2);
    return false;
}

static bool handlePage3ButtonClick_Button13(ZKButton *pButton) {
    togglePage3StartTime(3);
    return false;
}

static bool handlePage3ButtonClick_Button14(ZKButton *pButton) {
    return false;
}

static bool handlePage3ButtonClick_Button15(ZKButton *pButton) {
    return false;
}

static bool handlePage3ButtonClick_Button26(ZKButton *pButton) {
    return false;
}

static bool handlePage3ButtonClick_Button27(ZKButton *pButton) {
    return false;
}

static bool handlePage3ButtonClick_PervProgButton(ZKButton *pButton) {
    selectPage3Program(sPage3CurrentProgram - 1);
    return false;
}

static bool handlePage3ButtonClick_NextProgButton(ZKButton *pButton) {
    selectPage3Program(sPage3CurrentProgram + 1);
    return false;
}

static bool handlePage3ButtonClick_OnOffProgButton(ZKButton *pButton) {
    SPage3Program& program = currentPage3Program();
    if (!validatePage3ProgramBeforeEnable()) {
        updatePage3Controls();
        return false;
    }

    program.enabled = !program.enabled;
    updatePage3Controls();
    return false;
}

static bool handlePage3ButtonClick_WeekModeButton(ZKButton *pButton) {
    SPage3Program& program = currentPage3Program();
    program.weekMode = true;
    updatePage3Controls();
    return false;
}

static bool handlePage3ButtonClick_IntervalModeButton(ZKButton *pButton) {
    SPage3Program& program = currentPage3Program();
    program.weekMode = false;
    updatePage3Controls();
    return false;
}

static bool handlePage3ButtonClick_EverDayButton(ZKButton *pButton) {
    SPage3Program& program = currentPage3Program();
    program.weekMode = true;
    const bool selected = !isEveryDaySelected(program);
    for (int i = 0; i < PAGE3_WEEKDAY_COUNT; ++i) {
        program.weekdays[i] = selected;
    }
    updatePage3Controls();
    return false;
}

static bool handlePage3ButtonClick_SundayButton(ZKButton *pButton) {
    setPage3Weekday(PAGE3_SUNDAY, !currentPage3Program().weekdays[PAGE3_SUNDAY]);
    return false;
}

static bool handlePage3ButtonClick_MondayButton(ZKButton *pButton) {
    setPage3Weekday(PAGE3_MONDAY, !currentPage3Program().weekdays[PAGE3_MONDAY]);
    return false;
}

static bool handlePage3ButtonClick_TuesdayButton(ZKButton *pButton) {
    setPage3Weekday(PAGE3_TUESDAY, !currentPage3Program().weekdays[PAGE3_TUESDAY]);
    return false;
}

static bool handlePage3ButtonClick_WednesdayButton(ZKButton *pButton) {
    setPage3Weekday(PAGE3_WEDNESDAY, !currentPage3Program().weekdays[PAGE3_WEDNESDAY]);
    return false;
}

static bool handlePage3ButtonClick_ThursdayButton(ZKButton *pButton) {
    setPage3Weekday(PAGE3_THURSDAY, !currentPage3Program().weekdays[PAGE3_THURSDAY]);
    return false;
}

static bool handlePage3ButtonClick_FridayButton(ZKButton *pButton) {
    setPage3Weekday(PAGE3_FRIDAY, !currentPage3Program().weekdays[PAGE3_FRIDAY]);
    return false;
}

static bool handlePage3ButtonClick_SaturdayButton(ZKButton *pButton) {
    setPage3Weekday(PAGE3_SATURDAY, !currentPage3Program().weekdays[PAGE3_SATURDAY]);
    return false;
}

static void handlePage3EditTextChanged_ShowProgEditText(const std::string &text) {
    if (sPage3UpdatingControls) {
        return;
    }

    int programIndex = parseIntText(text, sPage3CurrentProgram + 1) - 1;
    programIndex = clampInt(programIndex, 0, kPage3ProgramCount - 1);
    selectPage3Program(programIndex);
}

static void handlePage3EditTextChanged_IrrTimerEditText(const std::string &text) {
    if (sPage3UpdatingControls) {
        return;
    }

    SPage3Program& program = currentPage3Program();
    int targetCount = clampInt(parseIntText(text, 0), 0, kPage3StartTimeCount);
    for (int i = 0; i < kPage3StartTimeCount; ++i) {
        SPage3StartTime& startTime = program.startTimes[i];
        const bool active = i < targetCount;
        if (active) {
            startTime.hourReady = true;
            startTime.minuteReady = true;
        } else {
            startTime.hourReady = false;
            startTime.minuteReady = false;
        }
        syncPage3StartTimeEnabled(startTime);
    }
    program.irrCount = getEnabledStartTimeCount(program);
    updatePage3Controls();
}

static void handlePage3EditTextChanged_StartTimeHour1EditText(const std::string &text) {
    setPage3StartHour(0, text);
}

static void handlePage3EditTextChanged_StartTimeMin1EditText(const std::string &text) {
    setPage3StartMinute(0, text);
}

static void handlePage3EditTextChanged_StartTimeHour2EditText(const std::string &text) {
    setPage3StartHour(1, text);
}

static void handlePage3EditTextChanged_StartTimeMin2EditText(const std::string &text) {
    setPage3StartMinute(1, text);
}

static void handlePage3EditTextChanged_StartTimeHour3EditText(const std::string &text) {
    setPage3StartHour(2, text);
}

static void handlePage3EditTextChanged_StartTimeMin3EditText(const std::string &text) {
    setPage3StartMinute(2, text);
}

static void handlePage3EditTextChanged_StartTimeHour4EditText(const std::string &text) {
    setPage3StartHour(3, text);
}

static void handlePage3EditTextChanged_StartTimeMin4EditText(const std::string &text) {
    setPage3StartMinute(3, text);
}

static void handlePage3EditTextChanged_IntervalDayEditText(const std::string &text) {
    if (sPage3UpdatingControls) {
        return;
    }

    SPage3Program& program = currentPage3Program();
    if (text.empty() || text == "-" || text == "1~99" || text == "--") {
        program.intervalDaysSet = false;
        program.intervalDays = 1;
    } else {
        program.intervalDaysSet = true;
        program.intervalDays = clampInt(parseIntText(text, 1), 1, 99);
    }
    updatePage3Controls();
}

static void handlePage3EditTextChanged_AfterDayEditText(const std::string &text) {
    if (sPage3UpdatingControls) {
        return;
    }

    SPage3Program& program = currentPage3Program();
    if (text.empty() || text == "-" || text == "1~30" || text == "--") {
        program.afterDaysSet = false;
        program.afterDays = 1;
    } else {
        program.afterDaysSet = true;
        program.afterDays = clampInt(parseIntText(text, 1), 1, 30);
    }
    updatePage3Controls();
}
