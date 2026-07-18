#include <cstdlib>
#include <cstdio>

namespace {

const int kPage3ProgramCount = 16;
const int kPage3StartTimeCount = 4;
const int kPage3MaxHour = 23;
const int kPage3MaxMinute = 59;

enum EPage3ValuePickerTarget {
    PAGE3_PICKER_NONE = 0,
    PAGE3_PICKER_START_HOUR_1,
    PAGE3_PICKER_START_MINUTE_1,
    PAGE3_PICKER_START_HOUR_2,
    PAGE3_PICKER_START_MINUTE_2,
    PAGE3_PICKER_START_HOUR_3,
    PAGE3_PICKER_START_MINUTE_3,
    PAGE3_PICKER_START_HOUR_4,
    PAGE3_PICKER_START_MINUTE_4,
    PAGE3_PICKER_INTERVAL_DAY,
    PAGE3_PICKER_AFTER_DAY,
};

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
EPage3ValuePickerTarget sPage3ValuePickerTarget = PAGE3_PICKER_NONE;
int sPage3ValuePickerMin = 0;
int sPage3ValuePickerMax = 0;
int sPage3PickerHour = 0;
int sPage3PickerMinute = 0;
int sPage3PickerDay = 1;

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

void setPickerButtonValue(ZKButton* button, bool ready, int value) {
    if (!button) {
        return;
    }

    if (!ready) {
        button->setText("--");
        return;
    }
    char text[8] = {0};
    snprintf(text, sizeof(text), "%02d", value);
    button->setText(text);
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
    ZKButton* timePickerButtons[] = {
        mW3StartTime1PickerButtonPtr,
        mW3StartTime2PickerButtonPtr,
        mW3StartTime3PickerButtonPtr,
        mW3StartTime4PickerButtonPtr,
    };

    for (int i = 0; i < kPage3StartTimeCount; ++i) {
        if (program.startTimes[i].hourReady && program.startTimes[i].minuteReady) {
            setEditTextTwoDigits(hourEditTexts[i], program.startTimes[i].hour);
            setEditTextTwoDigits(minuteEditTexts[i], program.startTimes[i].minute);
        } else {
            setEditTextText(hourEditTexts[i], "--");
            setEditTextText(minuteEditTexts[i], "--");
        }
        if (timePickerButtons[i]) {
            if (program.startTimes[i].hourReady && program.startTimes[i].minuteReady) {
                char text[12] = {0};
                snprintf(text, sizeof(text), "%02d:%02d",
                         program.startTimes[i].hour, program.startTimes[i].minute);
                timePickerButtons[i]->setText(text);
            } else {
                timePickerButtons[i]->setText("--:--");
            }
        }
    }
}

void updatePage3ModeEditTexts(const SPage3Program& program) {
    if (program.weekMode) {
        setEditTextText(mIntervalDayEditTextPtr, "-");
        setEditTextText(mAfterDayEditTextPtr, "-");
        setPickerButtonValue(mW3IntervalDayPickerButtonPtr, false, 0);
        setPickerButtonValue(mW3AfterDayPickerButtonPtr, false, 0);
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
    setPickerButtonValue(mW3IntervalDayPickerButtonPtr, program.intervalDaysSet,
                         program.intervalDays);
    setPickerButtonValue(mW3AfterDayPickerButtonPtr, program.afterDaysSet,
                         program.afterDays);
}

void updatePage3CycleProgramControls() {
    const bool visible = sPage3CurrentProgram == 0;
    if (mWindow3Region1WindowPtr) {
        mWindow3Region1WindowPtr->setVisible(visible);
    }
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

void closePage3ValuePicker() {
    if (mW3TimePickerWindowPtr) {
        mW3TimePickerWindowPtr->hideWnd();
    }
    if (mW3DayPickerWindowPtr) {
        mW3DayPickerWindowPtr->hideWnd();
    }
    sPage3ValuePickerTarget = PAGE3_PICKER_NONE;
}

void openPage3DayPicker(EPage3ValuePickerTarget target, int minValue, int maxValue,
                        int currentValue) {
    sPage3ValuePickerTarget = target;
    sPage3ValuePickerMin = minValue;
    sPage3ValuePickerMax = maxValue;
    sPage3PickerDay = clampInt(currentValue, minValue, maxValue);
    if (mW3DayPickerScrollPtr) {
    }
    if (mW3DayPickerListViewPtr) {
        mW3DayPickerListViewPtr->refreshListView();
        mW3DayPickerListViewPtr->setSelection(sPage3PickerDay - minValue);
    }
    if (mW3DayPickerWindowPtr) {
        mW3DayPickerWindowPtr->showWnd();
    }
}

void openPage3TimePicker(int timeIndex) {
    if (timeIndex < 0 || timeIndex >= kPage3StartTimeCount) {
        return;
    }
    SPage3Program& program = currentPage3Program();
    sPage3ValuePickerTarget = static_cast<EPage3ValuePickerTarget>(
            PAGE3_PICKER_START_HOUR_1 + timeIndex * 2);
    sPage3PickerHour = program.startTimes[timeIndex].hourReady ? program.startTimes[timeIndex].hour : 0;
    sPage3PickerMinute = program.startTimes[timeIndex].minuteReady ? program.startTimes[timeIndex].minute : 0;
    if (mW3TimePickerHourScrollPtr) {
    }
    if (mW3TimePickerMinuteScrollPtr) {
    }
    if (mW3TimePickerHourListViewPtr) {
        mW3TimePickerHourListViewPtr->refreshListView();
        mW3TimePickerHourListViewPtr->setSelection(sPage3PickerHour);
    }
    if (mW3TimePickerMinuteListViewPtr) {
        mW3TimePickerMinuteListViewPtr->refreshListView();
        mW3TimePickerMinuteListViewPtr->setSelection(sPage3PickerMinute);
    }
    if (mW3TimePickerWindowPtr) {
        mW3TimePickerWindowPtr->showWnd();
    }
}

void confirmPage3ValuePicker() {
    if (sPage3ValuePickerTarget >= PAGE3_PICKER_START_HOUR_1
            && sPage3ValuePickerTarget <= PAGE3_PICKER_START_MINUTE_4) {
        const int timeIndex = (static_cast<int>(sPage3ValuePickerTarget) - 1) / 2;
        SPage3StartTime& startTime = currentPage3Program().startTimes[timeIndex];
        startTime.hour = sPage3PickerHour;
        startTime.minute = sPage3PickerMinute;
        startTime.hourReady = true;
        startTime.minuteReady = true;
        syncPage3StartTimeEnabled(startTime);
        currentPage3Program().irrCount = getEnabledStartTimeCount(currentPage3Program());
    } else if (sPage3ValuePickerTarget == PAGE3_PICKER_INTERVAL_DAY) {
        SPage3Program& program = currentPage3Program();
        program.weekMode = false;
        program.intervalDays = sPage3PickerDay;
        program.intervalDaysSet = true;
    } else if (sPage3ValuePickerTarget == PAGE3_PICKER_AFTER_DAY) {
        SPage3Program& program = currentPage3Program();
        program.weekMode = false;
        program.afterDays = sPage3PickerDay;
        program.afterDaysSet = true;
    }

    closePage3ValuePicker();
    updatePage3Controls();
}

int getPage3TimePickerHourItemCount(const ZKListView* pListView) {
    return kPage3MaxHour + 1;
}

int getPage3TimePickerMinuteItemCount(const ZKListView* pListView) {
    return kPage3MaxMinute + 1;
}

int getPage3DayPickerItemCount(const ZKListView* pListView) {
    return sPage3ValuePickerTarget == PAGE3_PICKER_NONE
            ? 0 : sPage3ValuePickerMax - sPage3ValuePickerMin + 1;
}

void setPage3PickerItemValue(ZKListView::ZKListItem* pListItem, int id, int value) {
    if (!pListItem) {
        return;
    }
    ZKListView::ZKListSubItem* item = pListItem->findSubItemByID(id);
    if (!item) {
        return;
    }
    char text[8] = {0};
    snprintf(text, sizeof(text), "%02d", value);
    item->setText(text);
}

void obtainPage3TimePickerHourItemData(ZKListView* pListView,
                                       ZKListView::ZKListItem* pListItem, int index) {
    setPage3PickerItemValue(pListItem, ID_MAIN_W3TimePickerHourItem, index);
}

void obtainPage3TimePickerMinuteItemData(ZKListView* pListView,
                                         ZKListView::ZKListItem* pListItem, int index) {
    setPage3PickerItemValue(pListItem, ID_MAIN_W3TimePickerMinuteItem, index);
}

void obtainPage3DayPickerItemData(ZKListView* pListView,
                                  ZKListView::ZKListItem* pListItem, int index) {
    setPage3PickerItemValue(pListItem, ID_MAIN_W3DayPickerItem,
                              sPage3ValuePickerMin + index);
}

void onPage3TimePickerHourItemClick(ZKListView* pListView, int index, int id) {
    if (index >= 0 && index <= kPage3MaxHour) {
        sPage3PickerHour = index;
        if (mW3TimePickerHourListViewPtr) {
            mW3TimePickerHourListViewPtr->setSelection(index);
        }
    }
}

void onPage3TimePickerMinuteItemClick(ZKListView* pListView, int index, int id) {
    if (index >= 0 && index <= kPage3MaxMinute) {
        sPage3PickerMinute = index;
        if (mW3TimePickerMinuteListViewPtr) {
            mW3TimePickerMinuteListViewPtr->setSelection(index);
        }
    }
}

void onPage3DayPickerItemClick(ZKListView* pListView, int index, int id) {
    if (index >= 0 && index < getPage3DayPickerItemCount(pListView)) {
        sPage3PickerDay = sPage3ValuePickerMin + index;
        if (mW3DayPickerListViewPtr) {
            mW3DayPickerListViewPtr->setSelection(index);
        }
    }
}

bool handlePage3ButtonClick_W3ValuePickerField(ZKButton* pButton) {
    if (!pButton) {
        return false;
    }
    switch (pButton->getID()) {
    case ID_MAIN_W3StartTime1PickerButton:
        openPage3TimePicker(0); break;
    case ID_MAIN_W3StartTime2PickerButton:
        openPage3TimePicker(1); break;
    case ID_MAIN_W3StartTime3PickerButton:
        openPage3TimePicker(2); break;
    case ID_MAIN_W3StartTime4PickerButton:
        openPage3TimePicker(3); break;
    case ID_MAIN_W3IntervalDayPickerButton:
        openPage3DayPicker(PAGE3_PICKER_INTERVAL_DAY, 1, 99,
                            currentPage3Program().intervalDays); break;
    case ID_MAIN_W3AfterDayPickerButton:
        openPage3DayPicker(PAGE3_PICKER_AFTER_DAY, 1, 30,
                            currentPage3Program().afterDays); break;
    default:
        return false;
    }
    return false;
}

bool handlePage3ButtonClick_W3PickerOption(ZKButton* pButton) {
    if (!pButton) {
        return false;
    }
    const int id = pButton->getID();
    if (id >= 21100 && id <= 21123) {
        sPage3PickerHour = id - 21100;
    } else if (id >= 21200 && id <= 21259) {
        sPage3PickerMinute = id - 21200;
    } else if (id >= 21301 && id <= 21399) {
        sPage3PickerDay = id - 21300;
    }
    return false;
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
