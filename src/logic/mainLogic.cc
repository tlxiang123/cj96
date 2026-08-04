#pragma once
#include "uart/ProtocolSender.h"
#include "uart/Rs485Bus.h"
#include "net/NetManager.h"
#include "net/WifiCtrl.h"
#include "mainLogic.h"
#include "DeviceDataStore.h"
#include "DisplayPowerManager.h"
#include "utils/BrightnessHelper.h"
#include "utils/ScreenHelper.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <ctime>
#include <unistd.h>

#define WIFIMANAGER            NETMANAGER->getWifiManager()
#define ETHERNETMANAGER        NETMANAGER->getEthernetManager()
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
#define LTE4GMANAGER           NETMANAGER->getLTE4GManager()
#endif

static const char* kScreenshotSavePath = "/mnt/extsd/cj96_screenshot.bmp";
static const char* kDebugOpenMarkerPath = "/tmp/cj96_open_debug_page";
static const char* kOverviewOpenMarkerPath = "/tmp/cj96_open_overview_page";
static const char* kNetworkStatusEthernetPic = "network_status_ethernet_100.png";
static const char* kNetworkStatusWifiPic = "network_status_wifi_100.png";
static const char* kNetworkStatus4GPic = "network_status_4g_100.png";
static const char* kNetworkStatusNonePic = "network_status_none_100.png";
static const char* sCurrentNetworkStatusPic = "";
static const char* kPumpIconStaticPic = "window7_pump_icon.png";

static int collectMainWifiDnsServers(char servers[][16], int maxCount);
static const char* kPumpIcon1AnimFrames[] = {
    "window7_pump_icon_anim_00.png",
    "window7_pump_icon_anim_01.png",
    "window7_pump_icon_anim_02.png",
    "window7_pump_icon_anim_03.png",
    "window7_pump_icon_anim_04.png",
    "window7_pump_icon_anim_05.png",
    "window7_pump_icon_anim_06.png",
    "window7_pump_icon_anim_07.png",
    "window7_pump_icon_anim_08.png",
    "window7_pump_icon_anim_09.png",
    "window7_pump_icon_anim_10.png",
    "window7_pump_icon_anim_11.png",
    "window7_pump_icon_anim_12.png",
    "window7_pump_icon_anim_13.png",
    "window7_pump_icon_anim_14.png",
    "window7_pump_icon_anim_15.png",
    "window7_pump_icon_anim_16.png",
    "window7_pump_icon_anim_17.png",
    "window7_pump_icon_anim_18.png",
    "window7_pump_icon_anim_19.png",
};
static int sPumpIcon1AnimFrame = 0;
static const char* sPumpIcon1CurrentPic = "";
static const char* sPumpIcon2CurrentPic = "";
static const int kHomeRainSensorAddress = 8;
static const int kHomeHumiditySensorAddress = 6;

#ifndef ID_MAIN_Button48
#define ID_MAIN_Button48 20084
#endif
#ifndef ID_MAIN_Button49
#define ID_MAIN_Button49 20085
#endif
#ifndef ID_MAIN_TextView19
#define ID_MAIN_TextView19 50005
#endif
#ifndef ID_MAIN_TextView18
#define ID_MAIN_TextView18 50004
#endif
#ifndef ID_MAIN_RainDelayWindow
#define ID_MAIN_RainDelayWindow 110090
#define ID_MAIN_RainDelayTouchButton 20210
#define ID_MAIN_RainDelayCancelButton 20212
#define ID_MAIN_RainDelayOkButton 20213
#define ID_MAIN_RainDelayDaysEditText 51045
#endif
#ifndef ID_MAIN_HumidityThresholdWindow
#define ID_MAIN_HumidityThresholdWindow 110091
#define ID_MAIN_HumidityThresholdTouchButton 20214
#define ID_MAIN_HumidityThresholdCancelButton 20216
#define ID_MAIN_HumidityThresholdOkButton 20217
#define ID_MAIN_HumidityThresholdEditText 51046
#endif
#ifndef ID_MAIN_TextViewClockDate
#define ID_MAIN_TextViewClockDate 50266
#endif
#ifndef ID_MAIN_TextViewClockWeek
#define ID_MAIN_TextViewClockWeek 50267
#endif
#ifndef ID_MAIN_PumpAdvanceSecEditText
#define ID_MAIN_PumpAdvanceSecEditText 51043
#endif

static ZKButton* sHomeRainIconPtr = NULL;
static ZKButton* sHomeHumidityIconPtr = NULL;
static ZKTextView* sHomeRainTextPtr = NULL;
static ZKTextView* sHomeHumidityTextPtr = NULL;
static ZKTextView* sMainClockDateTextPtr = NULL;
static ZKTextView* sMainClockWeekTextPtr = NULL;
static char sMainClockDateLastText[16] = "";
static char sMainClockWeekLastText[16] = "";

/*
 *此文件由GUI工具生成
 *文件功能：用于处理用户的逻辑相应代码
 *功能说明：
 *========================onButtonClick_XXXX
 当页面中的按键按下后系统会调用对应的函数，XXX代表GUI工具里面的[ID值]名称，
 如Button1,当返回值为false的时候系统将不再处理这个按键，返回true的时候系统将会继续处理此按键。比如SYS_BACK.
 *========================onSlideWindowItemClick_XXXX(int index)
 当页面中存在滑动窗口并且用户点击了滑动窗口的图标后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
 如slideWindow1;index 代表按下图标的偏移值
 *========================onSeekBarChange_XXXX(int progress)
 当页面中存在滑动条并且用户改变了进度后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
 如SeekBar1;progress 代表当前的进度值
 *========================ogetListItemCount_XXXX()
 当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表的总数目,XXX代表GUI工具里面的[ID值]名称，
 如List1;返回值为当前列表的总条数
 *========================oobtainListItemData_XXXX(ZKListView::ZKListItem *pListItem, int index)
 当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表当前条目下的内容信息,XXX代表GUI工具里面的[ID值]名称，
 如List1;pListItem 是贴图中的单条目对象，index是列表总目的偏移量。具体见函数说明
 *========================常用接口===============
 *LOGD(...)  打印调试信息的接口
 *mTextXXXPtr->setText("****") 在控件TextXXX上显示文字****
 *mButton1Ptr->setSelected(true); 将控件mButton1设置为选中模式，图片会切换成选中图片，按钮文字会切换为选中后的颜色
 *mSeekBarPtr->setProgress(12) 在控件mSeekBar上将进度调整到12
 *mListView1Ptr->refreshListView() 让mListView1 重新刷新，当列表数据变化后调用
 *mDashbroadView1Ptr->setTargetAngle(120) 在控件mDashbroadView1上指针显示角度调整到120度
 *
 * 在Eclipse编辑器中  使用 “alt + /”  快捷键可以打开智能提示
 */

static bool validatePage3ProgramBeforeEnable();
static void refreshWindow4ListViews();
static bool requestWindow5DeviceState(int deviceIndex);
static bool requestWindow5ValveState(int deviceIndex, bool open);
static bool requestWindow5GroupValveState(int groupNo, bool open);
static bool requestWindow5GroupPumpState(int groupNo, bool open);
static bool requestWindow5GroupIrrigationState(int groupNo, bool open);
static bool isWindow5ValveCommandBusy();
static bool blockWindow5ValveCommandTouchIfBusy();
static long long getWindow5NowMs();
static void refreshWindow8IrrigationState();
static void refreshRunStatusValueText();
static void checkPage3CurrentStartTimeConflictAfterEdit(int startTimeIndex);
static void resetPage3ProgramStartTimeLastFiredDay(int programIndex, int startTimeIndex);
static const SDATA* findHomeSensorByAddress(int address);
bool requestWindow5DeviceDiscovery();
bool isWindow5DeviceDiscoveryRunning();
void updateWindow5DeviceStatePolling();
static bool requestWindow5CheckConfigForW2Add(int address, bool sensor,
                                              char *pMessage, size_t messageSize);

#include <control/ZKRadioGroup.h>

#ifndef ID_MAIN_W2AddDeviceWindow
#define ID_MAIN_W2AddDeviceWindow 110080
#define ID_MAIN_W2AddDeviceTitleText 50280
#define ID_MAIN_W2AddDeviceStatusText 50281
#define ID_MAIN_W2AddDeviceAddressLabelText 50282
#define ID_MAIN_W2AddDeviceTypeLabelText 50283
#define ID_MAIN_W2AddDeviceAddressEditText 51044
#define ID_MAIN_W2AddDeviceSensorButton 20200
#define ID_MAIN_W2AddDeviceValveButton 20201
#define ID_MAIN_W2AddDeviceAddressPrevButton 20202
#define ID_MAIN_W2AddDeviceAddressNextButton 20203
#define ID_MAIN_W2AddDeviceCancelButton 20204
#define ID_MAIN_W2AddDeviceOkButton 20205
#endif

#ifndef ID_MAIN_SenserRadioButton
#define ID_MAIN_SenserRadioButton 22001
#define ID_MAIN_ValueRadioButton 22002
static ZKRadioGroup *mRadioGroup1Ptr = NULL;
static ZKWindow *mTestAdressTipsWindowPtr = NULL;
#endif

#include "page1Logic.cc"
#include "page2Logic.cc"
#include "page3Logic.cc"
#include "page4Logic.cc"
#include "page5Logic.cc"
#include "page6Logic.cc"
#include "page7Logic.cc"
#include "page8Logic.cc"
#include "pageNavigationLogic.cc"

static bool blockWindow5ValveCommandTouchIfBusy() {
    if (!isWindow5ValveCommandBusy()) {
        return false;
    }
    showWindow5ValveWaitTip();
    return true;
}

struct SRunTimeItem {
    int groupNo;
    int hour;
    int minute;
    int second;
};

static std::vector<SRunTimeItem> sRunTimeItems;
static int sRunTimeEditingIndex = -1;
static int sRunTimeScopePendingIndex = -1;
static bool sRunTimeApplyAllGroups = false;
static bool sRunTimeWindowOpen = false;
static bool sRunTimeUpdatingEditTexts = false;
static SRunTimeItem sRunTimeEditValue = {0, 0, 0, 0};
static bool sW3TipWindowVisible = false;
static bool sMainWifiTipWindowVisible = false;
static const int kRunTimeMinDisplayRows = 2;
static int sWindow8TrackedRunningGroup = -1;
static time_t sWindow8StopTime = 0;
static bool sPage3ScheduleActive = false;
static int sPage3ScheduleProgramIndex = -1;
static int sPage3ScheduleStartTimeIndex = -1;
static int sPage3ScheduleGroupIndex = -1;
static bool sPage3ScheduleGroupOpen = false;
static bool sPage3ScheduleGroupClosing = false;
static long long sPage3ScheduleCloseAtMs = 0;
static long long sPage3ScheduleSwitchAtMs = 0;
static time_t sPage3ScheduleGroupEndTime = 0;
static time_t sPage3ScheduleEndTime = 0;
static time_t sPage3TodayCompletedTime = 0;
static bool sPage3SchedulePumpPreOpened = false;
static time_t sPage3ScheduleValveStartTime = 0;
static long long sPage3ScheduleValveStartAtMs = 0;
static std::vector<int> sPage3ScheduleGroups;
static std::vector<long long> sPage3ScheduleDurationsMs;
static int sPage3LastFiredDay[kPage3ProgramCount][kPage3StartTimeCount];
static const long long kPage3ValveReplyPollMs = 10LL;
static const long long kPage3CloseActionReserveMs = 1000LL;
static bool sRainDelayWindowVisible = false;
static int sRainDelayDays = 1;
static int sRainDelayLastTriggerDayId = -1;
static int sRainDelayAllowDayId = -1;
static bool sHumidityThresholdWindowVisible = false;
static int sHumidityTriggerThresholdPercent = 80;

struct SPage3PendingScheduleInfo {
    int groupNo;
    time_t startTime;
};

static bool findPage3PendingScheduleToday(time_t now, SPage3PendingScheduleInfo *pInfo);

static void setWindow8GroupNumber(ZKTextView* textView, int groupNo) {
    if (!textView) {
        return;
    }
    if (groupNo <= 0) {
        textView->setText("--");
        return;
    }
    char text[16] = {0};
    snprintf(text, sizeof(text), "%d", groupNo);
    textView->setText(text);
}

static void updateWindow8IrrigationDisplay(int completedGroup,
                                           int runningGroup,
                                           int waitingGroup,
                                           int remainingSeconds,
                                           int stopHour,
                                           int stopMinute,
                                           bool showStopTime) {
    setWindow8GroupNumber(mWindow8CompletedGroupTextPtr, completedGroup);
    setWindow8GroupNumber(mWindow8RunningGroupTextPtr, runningGroup);
    setWindow8GroupNumber(mWindow8WaitingGroupTextPtr, waitingGroup);

    const int remainingMinutes = remainingSeconds / 60;
    const int remainingSecondPart = remainingSeconds % 60;
    char line1[80] = {0};
    char line2[80] = {0};
    char line3[80] = {0};
    snprintf(line1, sizeof(line1), "阀组[%d]正在运行", runningGroup);
    snprintf(line2, sizeof(line2), "剩余%d分%02d秒",
             remainingMinutes, remainingSecondPart);
    if (showStopTime) {
        snprintf(line3, sizeof(line3), "当天结束时间%02d:%02d",
                 stopHour, stopMinute);
    }
    if (mWindow8StatusLine1TextPtr) mWindow8StatusLine1TextPtr->setText(line1);
    if (mWindow8StatusLine2TextPtr) mWindow8StatusLine2TextPtr->setText(line2);
    if (mWindow8StatusLine3TextPtr) mWindow8StatusLine3TextPtr->setText(line3);
}

static void updateWindow8ManualIrrigationDisplay(int completedGroup,
                                                 int runningGroup,
                                                 int waitingGroup) {
    setWindow8GroupNumber(mWindow8CompletedGroupTextPtr, completedGroup);
    setWindow8GroupNumber(mWindow8RunningGroupTextPtr, runningGroup);
    setWindow8GroupNumber(mWindow8WaitingGroupTextPtr, waitingGroup);

    char line1[80] = {0};
    snprintf(line1, sizeof(line1), "阀组[%d]灌溉中", runningGroup);
    if (mWindow8StatusLine1TextPtr) mWindow8StatusLine1TextPtr->setText(line1);
    if (mWindow8StatusLine2TextPtr) mWindow8StatusLine2TextPtr->setText("手动灌溉中");
    if (mWindow8StatusLine3TextPtr) mWindow8StatusLine3TextPtr->setText("");
}

static void clearWindow8IrrigationDisplay() {
    setWindow8GroupNumber(mWindow8CompletedGroupTextPtr, 1);
    setWindow8GroupNumber(mWindow8RunningGroupTextPtr, 2);
    setWindow8GroupNumber(mWindow8WaitingGroupTextPtr, 3);
    if (mWindow8StatusLine1TextPtr) mWindow8StatusLine1TextPtr->setText("");
    if (mWindow8StatusLine2TextPtr) mWindow8StatusLine2TextPtr->setText("");
    if (mWindow8StatusLine3TextPtr) mWindow8StatusLine3TextPtr->setText("");
}

static void updateWindow8PendingScheduleDisplay(int groupNo, time_t startTime) {
    setWindow8GroupNumber(mWindow8CompletedGroupTextPtr, 0);
    setWindow8GroupNumber(mWindow8RunningGroupTextPtr, groupNo);
    setWindow8GroupNumber(mWindow8WaitingGroupTextPtr, 0);

    struct tm startValue;
    memset(&startValue, 0, sizeof(startValue));
    localtime_r(&startTime, &startValue);

    char line1[80] = {0};
    char line2[80] = {0};
    snprintf(line1, sizeof(line1), "阀组[%d]", groupNo);
    snprintf(line2, sizeof(line2), "%02d:%02d即将开阀",
             startValue.tm_hour, startValue.tm_min);
    if (mWindow8StatusLine1TextPtr) mWindow8StatusLine1TextPtr->setText(line1);
    if (mWindow8StatusLine2TextPtr) mWindow8StatusLine2TextPtr->setText(line2);
    if (mWindow8StatusLine3TextPtr) mWindow8StatusLine3TextPtr->setText("");
}

static void updateWindow8TodayCompletedDisplay(time_t completedTime) {
    setWindow8GroupNumber(mWindow8CompletedGroupTextPtr, 0);
    setWindow8GroupNumber(mWindow8RunningGroupTextPtr, 0);
    setWindow8GroupNumber(mWindow8WaitingGroupTextPtr, 0);

    struct tm completedValue;
    memset(&completedValue, 0, sizeof(completedValue));
    localtime_r(&completedTime, &completedValue);

    char line2[80] = {0};
    snprintf(line2, sizeof(line2), "已于%02d:%02d分完成",
             completedValue.tm_hour, completedValue.tm_min);
    if (mWindow8StatusLine1TextPtr) mWindow8StatusLine1TextPtr->setText("今日灌溉");
    if (mWindow8StatusLine2TextPtr) mWindow8StatusLine2TextPtr->setText(line2);
    if (mWindow8StatusLine3TextPtr) mWindow8StatusLine3TextPtr->setText("");
}

static bool consumeDebugOpenMarker() {
	FILE* fp = fopen(kDebugOpenMarkerPath, "r");
	if (!fp) {
		return false;
	}
	fclose(fp);
	remove(kDebugOpenMarkerPath);
	return true;
}

static bool consumeOverviewOpenMarker() {
	FILE* fp = fopen(kOverviewOpenMarkerPath, "r");
	if (!fp) {
		return false;
	}
	fclose(fp);
	remove(kOverviewOpenMarkerPath);
	remove(kDebugOpenMarkerPath);
	return true;
}

static int clampRunTimeValue(int value, int minValue, int maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

static int parseRunTimeEditText(const std::string &text, int defaultValue, int minValue, int maxValue) {
    if (text.empty()) {
        return defaultValue;
    }
    return clampRunTimeValue(atoi(text.c_str()), minValue, maxValue);
}

static bool isRunTimeValidGroupNo(int groupNo) {
    return groupNo > 0 && groupNo <= 128;
}

static int findRunTimeItemIndex(int groupNo) {
    for (int i = 0; i < static_cast<int>(sRunTimeItems.size()); ++i) {
        if (sRunTimeItems[i].groupNo == groupNo) {
            return i;
        }
    }
    return -1;
}

static bool isIrrGroupHasValve(int groupNo) {
    if (!isRunTimeValidGroupNo(groupNo)) {
        return false;
    }

    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data) {
            continue;
        }
        if ((strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0) &&
                DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            return true;
        }
    }
    return false;
}

static long long getPage3ScheduleCloseLeadMs(int groupNo, long long durationMs) {
    const int valveCount = getIrrGroupValveCount(groupNo);
    if (valveCount <= 0 || durationMs <= 0) {
        return 0;
    }

    long long leadMs = static_cast<long long>(valveCount) *
            WINDOW5_VALVE_RSP_WAIT_LOOPS * kPage3ValveReplyPollMs;
    leadMs += kPage3CloseActionReserveMs;
    if (leadMs > durationMs) {
        return durationMs;
    }
    return leadMs;
}

static void syncRunTimeItemsWithValveGroups() {
    std::vector<SRunTimeItem> syncedItems;
    for (int groupNo = 1; groupNo <= 128; ++groupNo) {
        if (!isIrrGroupHasValve(groupNo)) {
            continue;
        }

        const int oldIndex = findRunTimeItemIndex(groupNo);
        if (oldIndex >= 0) {
            syncedItems.push_back(sRunTimeItems[oldIndex]);
        } else {
            SRunTimeItem item = {groupNo, 0, 0, 0};
            syncedItems.push_back(item);
        }
    }

    sRunTimeItems.swap(syncedItems);
    if (sRunTimeEditingIndex >= static_cast<int>(sRunTimeItems.size())) {
        sRunTimeEditingIndex = -1;
    }
}

static bool isWindow8GroupRunning(int groupNo) {
    if (!isRunTimeValidGroupNo(groupNo)) {
        return false;
    }
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && data->connected && data->stateKnown && data->state &&
            (strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0) &&
            DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            return true;
        }
    }
    return false;
}

static bool isAnyWindow8GroupRunning() {
    syncRunTimeItemsWithValveGroups();
    for (int i = 0; i < static_cast<int>(sRunTimeItems.size()); ++i) {
        if (isWindow8GroupRunning(sRunTimeItems[i].groupNo)) {
            return true;
        }
    }
    return false;
}

static void refreshRunStatusValueText() {
    if (!mRunStatusValueTextPtr) {
        return;
    }
    if (sPage3ScheduleActive || findPage3PendingScheduleToday(time(NULL), NULL)) {
        mRunStatusValueTextPtr->setText("自动");
    } else if (isWindow4RoundIrrigationEnabled()) {
        mRunStatusValueTextPtr->setText("手动");
    } else if (isAnyWindow8GroupRunning()) {
        mRunStatusValueTextPtr->setText("手动");
    } else {
        mRunStatusValueTextPtr->setText("待机");
    }
}

static void refreshWindow8IrrigationState() {
    syncRunTimeItemsWithValveGroups();

    int roundCompletedGroup = 0;
    int roundRunningGroup = 0;
    int roundWaitingGroup = 0;
    int roundRemainingSeconds = 0;
    time_t roundStopTime = 0;
    if (getWindow4RoundIrrigationDisplayState(&roundCompletedGroup,
                                              &roundRunningGroup,
                                              &roundWaitingGroup,
                                              &roundRemainingSeconds,
                                              &roundStopTime)) {
        struct tm roundEndTimeValue;
        memset(&roundEndTimeValue, 0, sizeof(roundEndTimeValue));
        localtime_r(&roundStopTime, &roundEndTimeValue);
        updateWindow8IrrigationDisplay(roundCompletedGroup,
                                       roundRunningGroup,
                                       roundWaitingGroup,
                                       roundRemainingSeconds,
                                       roundEndTimeValue.tm_hour,
                                       roundEndTimeValue.tm_min,
                                       false);
        if (mWindow8Ptr) {
            mWindow8Ptr->showWnd();
        }
        return;
    }

    int runningIndex = -1;
    for (int i = 0; i < static_cast<int>(sRunTimeItems.size()); ++i) {
        if (isWindow8GroupRunning(sRunTimeItems[i].groupNo)) {
            runningIndex = i;
            break;
        }
    }

    if (sPage3ScheduleActive && sPage3ScheduleGroupOpen
            && sPage3ScheduleGroupIndex >= 0
            && sPage3ScheduleGroupIndex < static_cast<int>(sPage3ScheduleGroups.size())) {
        const int scheduledGroup = sPage3ScheduleGroups[sPage3ScheduleGroupIndex];
        for (int i = 0; i < static_cast<int>(sRunTimeItems.size()); ++i) {
            if (sRunTimeItems[i].groupNo == scheduledGroup) {
                runningIndex = i;
                break;
            }
        }
    }

    if (runningIndex < 0 && sPage3ScheduleActive && !sPage3ScheduleGroupOpen) {
        const int nextGroupIndex = sPage3ScheduleGroupIndex + 1;
        if (nextGroupIndex >= 0 && nextGroupIndex < static_cast<int>(sPage3ScheduleGroups.size())) {
            updateWindow8PendingScheduleDisplay(sPage3ScheduleGroups[nextGroupIndex], time(NULL));
            if (mWindow8Ptr) {
                mWindow8Ptr->showWnd();
            }
            return;
        }
    }

    if (runningIndex < 0) {
        SPage3PendingScheduleInfo pending;
        if (findPage3PendingScheduleToday(time(NULL), &pending)) {
            sWindow8TrackedRunningGroup = -1;
            sWindow8StopTime = 0;
            updateWindow8PendingScheduleDisplay(pending.groupNo, pending.startTime);
            if (mWindow8Ptr) {
                mWindow8Ptr->showWnd();
            }
            return;
        }
    }

    if (runningIndex < 0 && sPage3TodayCompletedTime > 0) {
        const time_t now = time(NULL);
        struct tm nowValue;
        struct tm completedValue;
        memset(&nowValue, 0, sizeof(nowValue));
        memset(&completedValue, 0, sizeof(completedValue));
        localtime_r(&now, &nowValue);
        localtime_r(&sPage3TodayCompletedTime, &completedValue);
        if (nowValue.tm_year == completedValue.tm_year
                && nowValue.tm_yday == completedValue.tm_yday) {
            sWindow8TrackedRunningGroup = -1;
            sWindow8StopTime = 0;
            updateWindow8TodayCompletedDisplay(sPage3TodayCompletedTime);
            if (mWindow8Ptr) {
                mWindow8Ptr->showWnd();
            }
            return;
        }
    }

    if (runningIndex < 0) {
        sWindow8TrackedRunningGroup = -1;
        sWindow8StopTime = 0;
        clearWindow8IrrigationDisplay();
        if (mWindow8Ptr) {
            mWindow8Ptr->showWnd();
        }
        return;
    }

    const SRunTimeItem& runningItem = sRunTimeItems[runningIndex];
    const time_t now = time(NULL);
    const int manualDurationSeconds = runningItem.hour * 3600 +
                                      runningItem.minute * 60 +
                                      runningItem.second;
    if (sWindow8TrackedRunningGroup != runningItem.groupNo) {
        sWindow8TrackedRunningGroup = runningItem.groupNo;
        sWindow8StopTime = now + (manualDurationSeconds > 0 ? manualDurationSeconds : 0);
    }

    const int completedGroup = runningIndex > 0 ?
                               sRunTimeItems[runningIndex - 1].groupNo : 0;
    const int waitingGroup = runningIndex + 1 < static_cast<int>(sRunTimeItems.size()) ?
                             sRunTimeItems[runningIndex + 1].groupNo : 0;
    if (!sPage3ScheduleActive && manualDurationSeconds <= 0) {
        updateWindow8ManualIrrigationDisplay(completedGroup,
                                             runningItem.groupNo,
                                             waitingGroup);
        if (mWindow8Ptr) {
            mWindow8Ptr->showWnd();
        }
        return;
    }

    int remainingSeconds = sWindow8StopTime > now ?
                           static_cast<int>(sWindow8StopTime - now) : 0;
    if (sPage3ScheduleActive && sPage3ScheduleGroupOpen
            && sPage3ScheduleGroupIndex >= 0
            && sPage3ScheduleGroupIndex < static_cast<int>(sPage3ScheduleGroups.size())
            && sPage3ScheduleGroups[sPage3ScheduleGroupIndex] == runningItem.groupNo) {
        if (sPage3ScheduleGroupEndTime > now) {
            remainingSeconds = static_cast<int>(sPage3ScheduleGroupEndTime - now);
        } else {
            remainingSeconds = 0;
        }
    }
    const time_t displayEndTime =
            (sPage3ScheduleActive && sPage3ScheduleEndTime > 0) ?
            sPage3ScheduleEndTime : sWindow8StopTime;
    struct tm endTimeValue;
    memset(&endTimeValue, 0, sizeof(endTimeValue));
    localtime_r(&displayEndTime, &endTimeValue);
    updateWindow8IrrigationDisplay(completedGroup,
                                   runningItem.groupNo,
                                   waitingGroup,
                                   remainingSeconds,
                                   endTimeValue.tm_hour,
                                   endTimeValue.tm_min,
                                   sPage3ScheduleActive);
    if (mWindow8Ptr) {
        mWindow8Ptr->showWnd();
    }
}

static void setW3TipText(const char* text) {
    if (mW3TipTextViewPtr) {
        mW3TipTextViewPtr->setText(text);
    }
}

static const int MAIN_WIFI_TIP_COLOR_NEUTRAL = static_cast<int>(0x168BFFU);
static const int MAIN_WIFI_TIP_COLOR_SUCCESS = static_cast<int>(0x248A3DU);
static const int MAIN_WIFI_TIP_COLOR_FAILURE = static_cast<int>(0xD92D20U);
static long long sMainWifiTipAutoHideDeadlineMs = 0;

static void setMainWifiTipText(const char* text) {
    if (mMainWifiTipTextViewPtr) {
        mMainWifiTipTextViewPtr->setText(text);
    }
}

static void hideW3TipWindowOnly() {
    if (mW3TipWindowPtr) {
        mW3TipWindowPtr->hideWnd();
    }
    sW3TipWindowVisible = false;
}

static void hideMainWifiTipWindowOnly() {
    if (mMainWifiTipWindowPtr) {
        mMainWifiTipWindowPtr->hideWnd();
    }
    sMainWifiTipAutoHideDeadlineMs = 0;
    sMainWifiTipWindowVisible = false;
}

static bool hideW3TipWindowIfVisible() {
    if (!sW3TipWindowVisible) {
        return false;
    }

    hideW3TipWindowOnly();
    return true;
}

static bool hideMainWifiTipWindowIfVisible() {
    if (!sMainWifiTipWindowVisible) {
        return false;
    }

    hideMainWifiTipWindowOnly();
    return true;
}

static void showW3TipWindow(const char* text) {
    setW3TipText(text);
    if (mW3TipTextViewPtr) {
        mW3TipTextViewPtr->setTextColor(MAIN_WIFI_TIP_COLOR_NEUTRAL);
    }
    if (mW3TipWindowPtr) {
        mW3TipWindowPtr->showWnd();
        sW3TipWindowVisible = true;
    }
}

static void showMainWifiTipWindow(const char *text, int color,
        int autoHideMs) {
    setMainWifiTipText(text);
    if (mMainWifiTipTextViewPtr) {
        mMainWifiTipTextViewPtr->setTextColor(color);
    }
    sMainWifiTipAutoHideDeadlineMs = autoHideMs > 0
            ? getWindow5NowMs() + autoHideMs : 0;
    if (mMainWifiTipWindowPtr) {
        mMainWifiTipWindowPtr->showWnd();
        sMainWifiTipWindowVisible = true;
    }
}

static void updateMainWifiTipAutoHide() {
    if (sMainWifiTipAutoHideDeadlineMs <= 0) {
        return;
    }
    if (getWindow5NowMs() >= sMainWifiTipAutoHideDeadlineMs) {
        hideMainWifiTipWindowOnly();
    }
}

static volatile bool sMainWifiInternetCheckRunning = false;
static int sMainWifiInternetPendingTip = 0;
static bool sMainWifiInternetFailurePending = false;
static std::string sMainWifiInternetFailureSsid;
static std::string sMainWifiInternetPendingTipSsid;
static std::string sMainWifiLastCheckedSsid;
static std::string sMainWifiValidatedSsid;
static pthread_mutex_t sMainWifiInternetMutex = PTHREAD_MUTEX_INITIALIZER;
static const char *MAIN_WIFI_INTERFACE_NAME = "wlan0";
static const char *MAIN_WIFI_BAD_SSID_PATH = "/mnt/extsd/cj96_wifi_bad_ssid.txt";
static const char *MAIN_WIFI_MANUAL_RETRY_PATH = "/mnt/extsd/cj96_wifi_manual_retry_ssid.txt";
static const char *MAIN_WIFI_INTERNET_FAILURE_TEXT =
        "\xE6\x97\xA0\xE6\xB3\x95\xE8\xBF\x9E\xE6\x8E\xA5\xE5\xA4\x96\xE7\xBD\x91";
static const int MAIN_WIFI_QUALITY_STAGE_TIMEOUT_MS = 8000;

static std::string normalizeMainWifiSsidText(const char *ssid) {
    std::string text = ssid ? ssid : "";
    if (text.length() >= 2 && text[0] == '"'
            && text[text.length() - 1] == '"') {
        text = text.substr(1, text.length() - 2);
    }
    return text;
}

static bool getMainInterfaceIpv4Address(const char *interfaceName,
        struct in_addr *address) {
    if (!interfaceName || !address) {
        return false;
    }

    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ - 1);
    const bool success = ioctl(fd, SIOCGIFADDR, &ifr) == 0
            && ifr.ifr_addr.sa_family == AF_INET;
    if (success) {
        *address = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    }
    close(fd);
    return success;
}

static bool bindMainSocketToInterfaceIpv4(int fd, const char *interfaceName) {
    if (!interfaceName || interfaceName[0] == '\0') {
        return true;
    }

    struct in_addr localAddress;
    if (!getMainInterfaceIpv4Address(interfaceName, &localAddress)) {
        return false;
    }

    struct sockaddr_in bindAddress;
    memset(&bindAddress, 0, sizeof(bindAddress));
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_addr = localAddress;
    bindAddress.sin_port = 0;
    if (bind(fd, (struct sockaddr *)&bindAddress, sizeof(bindAddress)) != 0) {
        return false;
    }

#ifdef SO_BINDTODEVICE
    setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
            interfaceName, strlen(interfaceName));
#endif
    return true;
}

static int appendMainDnsName(unsigned char *packet, int offset, int maxLength,
        const char *domain) {
    const char *segment = domain;
    while (segment && *segment) {
        const char *dot = strchr(segment, '.');
        const int length = dot ? (dot - segment) : strlen(segment);
        if (length <= 0 || length > 63 || offset + 1 + length >= maxLength) {
            return -1;
        }
        packet[offset++] = static_cast<unsigned char>(length);
        memcpy(packet + offset, segment, length);
        offset += length;
        segment = dot ? dot + 1 : NULL;
    }
    if (offset >= maxLength) {
        return -1;
    }
    packet[offset++] = 0;
    return offset;
}

static bool requestMainWifiDnsProbeOnInterface(const char *interfaceName,
        const char *serverIp, const char *domain, struct in_addr *resolvedIp) {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return false;
    }

    bool success = false;
    if (!bindMainSocketToInterfaceIpv4(fd, interfaceName)) {
        close(fd);
        return false;
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 800000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in remoteAddress;
    memset(&remoteAddress, 0, sizeof(remoteAddress));
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(53);
    if (inet_pton(AF_INET, serverIp, &remoteAddress.sin_addr) == 1) {
        unsigned char packet[96];
        memset(packet, 0, sizeof(packet));
        const unsigned short queryId = 0x4A96;
        packet[0] = static_cast<unsigned char>((queryId >> 8) & 0xFF);
        packet[1] = static_cast<unsigned char>(queryId & 0xFF);
        packet[2] = 0x01;
        packet[5] = 0x01;
        int length = appendMainDnsName(packet, 12, sizeof(packet), domain);
        if (length > 0 && length + 4 <= static_cast<int>(sizeof(packet))) {
            packet[length++] = 0x00;
            packet[length++] = 0x01;
            packet[length++] = 0x00;
            packet[length++] = 0x01;
            const ssize_t sent = sendto(fd, packet, length, 0,
                    (struct sockaddr *)&remoteAddress, sizeof(remoteAddress));
            if (sent == length) {
                unsigned char response[512];
                const ssize_t received = recvfrom(fd, response,
                        sizeof(response), 0, NULL, NULL);
                const int answerCount = (received >= 12)
                        ? ((response[6] << 8) | response[7]) : 0;
                if (received >= 12
                        && response[0] == packet[0]
                        && response[1] == packet[1]
                        && (response[2] & 0x80)
                        && ((response[3] & 0x0F) == 0)
                        && answerCount > 0) {
                    success = true;
                    if (resolvedIp) {
                        int offset = length;
                        for (int i = 0; i < answerCount && offset + 12 <= received; ++i) {
                            if ((response[offset] & 0xC0) == 0xC0) {
                                offset += 2;
                            } else {
                                while (offset < received && response[offset] != 0) {
                                    offset += response[offset] + 1;
                                }
                                offset += 1;
                            }
                            if (offset + 10 > received) {
                                break;
                            }
                            const int type = (response[offset] << 8) | response[offset + 1];
                            const int classCode = (response[offset + 2] << 8) | response[offset + 3];
                            const int rdLength = (response[offset + 8] << 8) | response[offset + 9];
                            offset += 10;
                            if (type == 1 && classCode == 1 && rdLength == 4
                                    && offset + 4 <= received) {
                                memcpy(&resolvedIp->s_addr, response + offset, 4);
                                break;
                            }
                            offset += rdLength;
                        }
                    }
                }
            }
        }
    }

    close(fd);
    return success;
}

static bool resolveMainWifiDomainOnInterface(const char *interfaceName,
        const char *domain, struct in_addr *resolvedIp) {
    char dnsProbeIps[4][16];
    const int count = collectMainWifiDnsServers(dnsProbeIps,
            sizeof(dnsProbeIps) / sizeof(dnsProbeIps[0]));
    for (int i = 0; i < count; ++i) {
        if (requestMainWifiDnsProbeOnInterface(interfaceName,
                dnsProbeIps[i], domain, resolvedIp)) {
            return true;
        }
    }
    return false;
}

static bool canMainWifiResolvePublicDomain() {
    static const char *domains[] = {
        "www.microsoft.com",
        "www.apple.com",
        "www.baidu.com",
        "www.qq.com",
    };
    const int count = sizeof(domains) / sizeof(domains[0]);
    for (int i = 0; i < count; ++i) {
        if (resolveMainWifiDomainOnInterface(MAIN_WIFI_INTERFACE_NAME,
                domains[i], NULL)) {
            return true;
        }
    }
    return false;
}

static bool requestMainWifiHttpProbeOnInterface(const char *interfaceName,
        const char *host, const char *path, const char *expectedText) {
    struct in_addr remoteIp;
    memset(&remoteIp, 0, sizeof(remoteIp));
    if (!resolveMainWifiDomainOnInterface(interfaceName, host, &remoteIp)
            || remoteIp.s_addr == 0) {
        return false;
    }

    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return false;
    }

    bool success = false;
    if (!bindMainSocketToInterfaceIpv4(fd, interfaceName)) {
        close(fd);
        return false;
    }

    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    struct sockaddr_in remoteAddress;
    memset(&remoteAddress, 0, sizeof(remoteAddress));
    remoteAddress.sin_family = AF_INET;
    remoteAddress.sin_port = htons(80);
    remoteAddress.sin_addr = remoteIp;

    const int ret = connect(fd, (struct sockaddr *)&remoteAddress,
            sizeof(remoteAddress));
    if (ret == 0 || errno == EINPROGRESS) {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(fd, &writeSet);
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        bool connected = ret == 0;
        if (!connected && select(fd + 1, NULL, &writeSet, NULL, &timeout) > 0
                && FD_ISSET(fd, &writeSet)) {
            int error = 0;
            socklen_t errorLength = sizeof(error);
            connected = getsockopt(fd, SOL_SOCKET, SO_ERROR,
                    &error, &errorLength) == 0 && error == 0;
        }
        if (connected) {
            if (flags >= 0) {
                fcntl(fd, F_SETFL, flags);
            }
            char request[256];
            snprintf(request, sizeof(request),
                    "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
                    path, host);
            const int requestLength = strlen(request);
            const ssize_t sent = send(fd, request, requestLength, 0);
            if (sent == requestLength) {
                char response[512];
                memset(response, 0, sizeof(response));
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                const ssize_t received = recv(fd, response, sizeof(response) - 1, 0);
                if (received > 0 && strstr(response, expectedText) != NULL) {
                    success = true;
                }
            }
        }
    }

    close(fd);
    return success;
}

static bool canMainWifiReachInternet() {
    return (canMainWifiResolvePublicDomain()
            && requestMainWifiHttpProbeOnInterface(MAIN_WIFI_INTERFACE_NAME,
                    "www.msftconnecttest.com",
                    "/connecttest.txt",
                    "Microsoft Connect Test"))
            || requestMainWifiHttpProbeOnInterface(MAIN_WIFI_INTERFACE_NAME,
                    "captive.apple.com",
                    "/hotspot-detect.html",
                    "Success");
}

static void trimMainLineEnd(char *text) {
    if (!text) {
        return;
    }
    text[strcspn(text, "\r\n")] = '\0';
}

static bool readMainOneLineFile(const char *path, std::string &value) {
    value.clear();
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return false;
    }
    char line[128];
    const bool ok = fgets(line, sizeof(line), fp) != NULL;
    fclose(fp);
    if (!ok) {
        return false;
    }
    trimMainLineEnd(line);
    value = line;
    return !value.empty();
}

static void writeMainOneLineFile(const char *path, const std::string &value) {
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        return;
    }
    fprintf(fp, "%s\n", value.c_str());
    fclose(fp);
}

static void addMainWifiDnsServer(char servers[][16], int &count, int maxCount,
        const char *serverIp) {
    if (!serverIp || serverIp[0] == '\0' || count >= maxCount) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        if (strcmp(servers[i], serverIp) == 0) {
            return;
        }
    }
    strncpy(servers[count], serverIp, 15);
    servers[count][15] = '\0';
    ++count;
}

static int collectMainWifiDnsServers(char servers[][16], int maxCount) {
    int count = 0;
    FILE *fp = fopen("/proc/net/route", "r");
    if (fp != NULL) {
        char line[256];
        (void)fgets(line, sizeof(line), fp);
        while (fgets(line, sizeof(line), fp) != NULL) {
            char iface[32];
            unsigned long destination = 0;
            unsigned long gateway = 0;
            if (sscanf(line, "%31s %lx %lx", iface, &destination, &gateway) == 3
                    && strcmp(iface, MAIN_WIFI_INTERFACE_NAME) == 0
                    && destination == 0 && gateway != 0) {
                struct in_addr address;
                address.s_addr = static_cast<in_addr_t>(gateway);
                char text[16];
                if (inet_ntop(AF_INET, &address, text, sizeof(text))) {
                    addMainWifiDnsServer(servers, count, maxCount, text);
                }
                break;
            }
        }
        fclose(fp);
    }
    addMainWifiDnsServer(servers, count, maxCount, "223.5.5.5");
    addMainWifiDnsServer(servers, count, maxCount, "114.114.114.114");
    addMainWifiDnsServer(servers, count, maxCount, "1.1.1.1");
    return count;
}

static void removeMainFile(const char *path) {
    unlink(path);
}

static bool isMainBadWifiSsid(const std::string &ssid) {
    std::string badSsid;
    return !ssid.empty()
            && readMainOneLineFile(MAIN_WIFI_BAD_SSID_PATH, badSsid)
            && badSsid == ssid;
}

static bool isMainManualRetryWifiSsid(const std::string &ssid) {
    std::string retrySsid;
    return !ssid.empty()
            && readMainOneLineFile(MAIN_WIFI_MANUAL_RETRY_PATH, retrySsid)
            && retrySsid == ssid;
}

static void markMainBadWifiSsid(const std::string &ssid) {
    if (!ssid.empty()) {
        writeMainOneLineFile(MAIN_WIFI_BAD_SSID_PATH, ssid);
    }
    removeMainFile(MAIN_WIFI_MANUAL_RETRY_PATH);
}

static void clearMainBadWifiSsid(const std::string &ssid) {
    std::string badSsid;
    if (readMainOneLineFile(MAIN_WIFI_BAD_SSID_PATH, badSsid)
            && badSsid == ssid) {
        removeMainFile(MAIN_WIFI_BAD_SSID_PATH);
    }
    std::string retrySsid;
    if (readMainOneLineFile(MAIN_WIFI_MANUAL_RETRY_PATH, retrySsid)
            && retrySsid == ssid) {
        removeMainFile(MAIN_WIFI_MANUAL_RETRY_PATH);
    }
}

static void forgetMainWifiNetworkIfKnown(int networkId) {
    if (networkId < 0) {
        return;
    }
    WifiCtrl ctrl(MAIN_WIFI_INTERFACE_NAME);
    if (!ctrl.connectToSupplicant()) {
        return;
    }
    ctrl.removeNetwork(networkId);
    ctrl.saveConfig();
    ctrl.closeSupplicantConnection();
}

static void queueMainWifiInternetFailure(const std::string &ssid) {
    pthread_mutex_lock(&sMainWifiInternetMutex);
    sMainWifiInternetFailureSsid = ssid;
    sMainWifiInternetFailurePending = true;
    sMainWifiInternetPendingTipSsid = ssid;
    sMainWifiInternetPendingTip = 3;
    if (sMainWifiValidatedSsid == ssid) {
        sMainWifiValidatedSsid.clear();
    }
    pthread_mutex_unlock(&sMainWifiInternetMutex);
}

static void markMainWifiInternetValidationSuccess(const std::string &ssid) {
    pthread_mutex_lock(&sMainWifiInternetMutex);
    sMainWifiValidatedSsid = ssid;
    sMainWifiInternetPendingTipSsid = ssid;
    sMainWifiInternetPendingTip = 2;
    pthread_mutex_unlock(&sMainWifiInternetMutex);
}

static void queueMainWifiInternetChecking(const std::string &ssid) {
    pthread_mutex_lock(&sMainWifiInternetMutex);
    sMainWifiInternetPendingTipSsid = ssid;
    sMainWifiInternetPendingTip = 1;
    pthread_mutex_unlock(&sMainWifiInternetMutex);
}

struct MainWifiInternetCheckRequest {
    std::string ssid;
    int networkId;
};

static void* mainWifiInternetCheckWorker(void *arg) {
    MainWifiInternetCheckRequest *request =
            static_cast<MainWifiInternetCheckRequest *>(arg);
    const std::string ssid = request ? request->ssid : "";
    const int networkId = request ? request->networkId : -1;
    delete request;

    usleep(300 * 1000);
    bool stillSameWifi = false;
    if (WIFIMANAGER && WIFIMANAGER->isConnected()) {
        const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
        if (connectionInfo) {
            const std::string connectedSsid =
                    normalizeMainWifiSsidText(connectionInfo->getSsid().c_str());
            stillSameWifi = !ssid.empty() && connectedSsid == ssid;
        }
    }

    if (stillSameWifi) {
        const long long startMs = getWindow5NowMs();
        bool reachable = false;
        while ((getWindow5NowMs() - startMs) <= MAIN_WIFI_QUALITY_STAGE_TIMEOUT_MS) {
            if (canMainWifiReachInternet()) {
                reachable = true;
                break;
            }
            usleep(700 * 1000);
            if (!WIFIMANAGER || !WIFIMANAGER->isConnected()) {
                break;
            }
            const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
            const std::string connectedSsid = connectionInfo
                    ? normalizeMainWifiSsidText(connectionInfo->getSsid().c_str()) : "";
            if (connectedSsid != ssid) {
                break;
            }
        }
        if (reachable) {
            markMainWifiInternetValidationSuccess(ssid);
            clearMainBadWifiSsid(ssid);
        } else {
            WIFIMANAGER->disconnect();
            forgetMainWifiNetworkIfKnown(networkId);
            markMainBadWifiSsid(ssid);
            queueMainWifiInternetFailure(ssid);
            pthread_mutex_lock(&sMainWifiInternetMutex);
            sMainWifiLastCheckedSsid.clear();
            pthread_mutex_unlock(&sMainWifiInternetMutex);
        }
    }

    sMainWifiInternetCheckRunning = false;
    return NULL;
}

static void startMainWifiInternetCheck(const char *ssid, int networkId) {
    if (sMainWifiInternetCheckRunning || !WIFIMANAGER) {
        return;
    }

    std::string checkSsid = normalizeMainWifiSsidText(ssid);
    if (WIFIMANAGER->isConnected()) {
        const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
        if (connectionInfo && !connectionInfo->getSsid().empty()) {
            checkSsid = normalizeMainWifiSsidText(connectionInfo->getSsid().c_str());
        }
    }
    pthread_mutex_lock(&sMainWifiInternetMutex);
    const bool alreadyChecked = checkSsid == sMainWifiLastCheckedSsid;
    pthread_mutex_unlock(&sMainWifiInternetMutex);
    if (checkSsid.empty() || alreadyChecked) {
        return;
    }
    if (isMainBadWifiSsid(checkSsid)
            && !isMainManualRetryWifiSsid(checkSsid)) {
        WIFIMANAGER->disconnect();
        queueMainWifiInternetFailure(checkSsid);
        return;
    }

    MainWifiInternetCheckRequest *request = new MainWifiInternetCheckRequest;
    request->ssid = checkSsid;
    request->networkId = networkId;
    pthread_mutex_lock(&sMainWifiInternetMutex);
    sMainWifiLastCheckedSsid = checkSsid;
    pthread_mutex_unlock(&sMainWifiInternetMutex);
    queueMainWifiInternetChecking(checkSsid);
    sMainWifiInternetCheckRunning = true;
    pthread_t worker;
    if (pthread_create(&worker, NULL, mainWifiInternetCheckWorker, request) != 0) {
        delete request;
        pthread_mutex_lock(&sMainWifiInternetMutex);
        sMainWifiLastCheckedSsid.clear();
        pthread_mutex_unlock(&sMainWifiInternetMutex);
        sMainWifiInternetCheckRunning = false;
    } else {
        pthread_detach(worker);
    }
}

static void checkCurrentMainWifiIfConnected() {
    if (!WIFIMANAGER || !WIFIMANAGER->isConnected()) {
        pthread_mutex_lock(&sMainWifiInternetMutex);
        sMainWifiLastCheckedSsid.clear();
        sMainWifiValidatedSsid.clear();
        pthread_mutex_unlock(&sMainWifiInternetMutex);
        return;
    }

    const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
    if (!connectionInfo) {
        return;
    }
    startMainWifiInternetCheck(connectionInfo->getSsid().c_str(), -1);
}

static void showPendingMainWifiInternetStatusIfNeeded() {
    pthread_mutex_lock(&sMainWifiInternetMutex);
    if (sMainWifiInternetPendingTip == 0 && !sMainWifiInternetFailurePending) {
        pthread_mutex_unlock(&sMainWifiInternetMutex);
        return;
    }
    sMainWifiInternetPendingTip = 0;
    sMainWifiInternetPendingTipSsid.clear();
    sMainWifiInternetFailurePending = false;
    sMainWifiInternetFailureSsid.clear();
    pthread_mutex_unlock(&sMainWifiInternetMutex);
}

static bool isMainWifiInternetValidated() {
    if (!WIFIMANAGER || !WIFIMANAGER->isConnected()) {
        return false;
    }
    struct in_addr wifiAddress;
    if (!getMainInterfaceIpv4Address(MAIN_WIFI_INTERFACE_NAME, &wifiAddress)) {
        return false;
    }
    const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
    if (!connectionInfo) {
        return false;
    }
    const std::string ssid =
            normalizeMainWifiSsidText(connectionInfo->getSsid().c_str());
    pthread_mutex_lock(&sMainWifiInternetMutex);
    const bool validated = !ssid.empty() && ssid == sMainWifiValidatedSsid;
    pthread_mutex_unlock(&sMainWifiInternetMutex);
    return validated;
}

class MainWifiInternetListener : public WifiManager::IWifiListener {
public:
    virtual void handleWifiEnable(E_WIFI_ENABLE event, int args) {
        if (event == E_WIFI_ENABLE_DISABLE || event == E_WIFI_ENABLE_UNKNOW) {
            pthread_mutex_lock(&sMainWifiInternetMutex);
            sMainWifiLastCheckedSsid.clear();
            sMainWifiValidatedSsid.clear();
            pthread_mutex_unlock(&sMainWifiInternetMutex);
        }
    }

    virtual void handleWifiConnect(E_WIFI_CONNECT event, int args) {
        checkCurrentMainWifiIfConnected();
    }

    virtual void handleWifiErrorCode(E_WIFI_ERROR_CODE code) {
    }

    virtual void handleWifiScanResult(std::vector<WifiInfo>* wifiInfos) {
    }

    virtual void notifySupplicantStateChange(int networkid,
            const char* ssid, const char* bssid,
            E_SUPPLICATION_STATE newState) {
        if (newState == COMPLETED) {
            startMainWifiInternetCheck(ssid, networkid);
        } else if (newState == DISCONNECTED
                || newState == INTERFACE_DISABLED
                || newState == INACTIVE
                || newState == UNINITIALIZED
                || newState == INVALID) {
            pthread_mutex_lock(&sMainWifiInternetMutex);
            const std::string disconnectedSsid = normalizeMainWifiSsidText(ssid);
            sMainWifiLastCheckedSsid.clear();
            if (sMainWifiValidatedSsid == disconnectedSsid) {
                sMainWifiValidatedSsid.clear();
            }
            pthread_mutex_unlock(&sMainWifiInternetMutex);
        }
    }
};

static MainWifiInternetListener sMainWifiInternetListener;

static bool hasPage3WeekdaySelection(const SPage3Program& program) {
    for (int i = 0; i < PAGE3_WEEKDAY_COUNT; ++i) {
        if (program.weekdays[i]) {
            return true;
        }
    }
    return false;
}

static bool hasPage3ModeSelection(const SPage3Program& program) {
    if (program.weekMode) {
        return hasPage3WeekdaySelection(program);
    }
    return program.intervalDaysSet;
}

static bool hasPage3RunTimeValue() {
    syncRunTimeItemsWithValveGroups();
    for (int i = 0; i < static_cast<int>(sRunTimeItems.size()); ++i) {
        const SRunTimeItem& item = sRunTimeItems[i];
        if (item.hour > 0 || item.minute > 0 || item.second > 0) {
            return true;
        }
    }
    return false;
}

struct SPage3ScheduleEvent {
    long long startSecond;
    long long endSecond;
};

static int getPage3ProgramRuntimeSeconds() {
    syncRunTimeItemsWithValveGroups();
    int totalSeconds = 0;
    for (int i = 0; i < static_cast<int>(sRunTimeItems.size()); ++i) {
        const SRunTimeItem& item = sRunTimeItems[i];
        const int durationSeconds = item.hour * 3600 + item.minute * 60 + item.second;
        if (durationSeconds > 0) {
            totalSeconds += durationSeconds;
        }
    }
    return totalSeconds;
}

static bool isPage3StartTimeReady(const SPage3Program& program, int startTimeIndex) {
    if (startTimeIndex < 0 || startTimeIndex >= kPage3StartTimeCount) {
        return false;
    }
    const SPage3StartTime& startTime = program.startTimes[startTimeIndex];
    return startTime.hourReady && startTime.minuteReady;
}

static time_t getPage3TodayMidnight(time_t now) {
    struct tm value;
    memset(&value, 0, sizeof(value));
    localtime_r(&now, &value);
    value.tm_hour = 0;
    value.tm_min = 0;
    value.tm_sec = 0;
    return mktime(&value);
}

static int getPage3DayId(time_t now) {
    const time_t midnight = getPage3TodayMidnight(now);
    return static_cast<int>(midnight / (24 * 3600));
}

static int clampRainDelayDays(int value) {
    if (value < 1) {
        return 1;
    }
    if (value > 30) {
        return 30;
    }
    return value;
}

static int clampHumidityThresholdPercent(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

static ZKWindow* getRainDelayWindow() {
    if (!mWindow1Ptr) {
        return NULL;
    }
    return (ZKWindow*)mWindow1Ptr->findControlByID(ID_MAIN_RainDelayWindow);
}

static ZKEditText* getRainDelayDaysEditText() {
    ZKWindow* window = getRainDelayWindow();
    if (!window) {
        return NULL;
    }
    return (ZKEditText*)window->findControlByID(ID_MAIN_RainDelayDaysEditText);
}

static ZKWindow* getHumidityThresholdWindow() {
    if (!mWindow1Ptr) {
        return NULL;
    }
    return (ZKWindow*)mWindow1Ptr->findControlByID(ID_MAIN_HumidityThresholdWindow);
}

static ZKEditText* getHumidityThresholdEditText() {
    ZKWindow* window = getHumidityThresholdWindow();
    if (!window) {
        return NULL;
    }
    return (ZKEditText*)window->findControlByID(ID_MAIN_HumidityThresholdEditText);
}

static bool isHomeRainSnowDetected();

static void updateRainDelayDaysEditText() {
    ZKEditText* editText = getRainDelayDaysEditText();
    if (editText) {
        editText->setText(sRainDelayDays);
    }
}

static void hideRainDelayWindowOnly() {
    ZKWindow* window = getRainDelayWindow();
    if (window) {
        window->hideWnd();
    }
    sRainDelayWindowVisible = false;
}

static void updateHumidityThresholdEditText() {
    ZKEditText* editText = getHumidityThresholdEditText();
    if (editText) {
        editText->setText(sHumidityTriggerThresholdPercent);
    }
}

static void hideHumidityThresholdWindowOnly() {
    ZKWindow* window = getHumidityThresholdWindow();
    if (window) {
        window->hideWnd();
    }
    sHumidityThresholdWindowVisible = false;
}

static void openRainDelayWindow() {
    updateRainDelayDaysEditText();
    ZKWindow* window = getRainDelayWindow();
    if (window) {
        window->showWnd();
    }
    sRainDelayWindowVisible = true;
}

static void openHumidityThresholdWindow() {
    updateHumidityThresholdEditText();
    ZKWindow* window = getHumidityThresholdWindow();
    if (window) {
        window->showWnd();
    }
    sHumidityThresholdWindowVisible = true;
}

static void confirmRainDelayWindow() {
    ZKEditText* editText = getRainDelayDaysEditText();
    if (editText) {
        sRainDelayDays = clampRainDelayDays(std::atoi(editText->getText().c_str()));
    }
    const time_t now = time(NULL);
    const int todayId = getPage3DayId(now);
    if (isHomeRainSnowDetected() || sRainDelayAllowDayId > todayId) {
        sRainDelayLastTriggerDayId = todayId;
        sRainDelayAllowDayId = todayId + sRainDelayDays;
    }
    updateRainDelayDaysEditText();
    hideRainDelayWindowOnly();
}

static void confirmHumidityThresholdWindow() {
    ZKEditText* editText = getHumidityThresholdEditText();
    if (editText) {
        sHumidityTriggerThresholdPercent =
                clampHumidityThresholdPercent(std::atoi(editText->getText().c_str()));
    }
    updateHumidityThresholdEditText();
    hideHumidityThresholdWindowOnly();
}

static bool handleRainDelayWindowClick(ZKBase *pBase) {
    if (!pBase) {
        return false;
    }
    const int id = pBase->getID();
    if (id == ID_MAIN_RainDelayTouchButton || id == ID_MAIN_Button48 || id == ID_MAIN_TextView19) {
        openRainDelayWindow();
        return true;
    }
    if (!sRainDelayWindowVisible) {
        return false;
    }
    if (id == ID_MAIN_RainDelayCancelButton) {
        hideRainDelayWindowOnly();
        return true;
    }
    if (id == ID_MAIN_RainDelayOkButton) {
        confirmRainDelayWindow();
        return true;
    }
    if (id == ID_MAIN_RainDelayDaysEditText) {
        return false;
    }
    return true;
}

static bool handleHumidityThresholdWindowClick(ZKBase *pBase) {
    if (!pBase) {
        return false;
    }
    const int id = pBase->getID();
    if (id == ID_MAIN_HumidityThresholdTouchButton ||
            id == ID_MAIN_Button49 ||
            id == ID_MAIN_TextView18) {
        openHumidityThresholdWindow();
        return true;
    }
    if (!sHumidityThresholdWindowVisible) {
        return false;
    }
    if (id == ID_MAIN_HumidityThresholdCancelButton) {
        hideHumidityThresholdWindowOnly();
        return true;
    }
    if (id == ID_MAIN_HumidityThresholdOkButton) {
        confirmHumidityThresholdWindow();
        return true;
    }
    if (id == ID_MAIN_HumidityThresholdEditText) {
        return false;
    }
    return true;
}

static bool isHomeRainSnowDetected() {
    const SDATA* rain = findHomeSensorByAddress(kHomeRainSensorAddress);
    return rain && rain->connected && rain->stateKnown && rain->state;
}

static bool updateRainDelayAndShouldBlockToday(time_t now) {
    const int todayId = getPage3DayId(now);
    if (isHomeRainSnowDetected() && sRainDelayLastTriggerDayId != todayId) {
        sRainDelayLastTriggerDayId = todayId;
        sRainDelayAllowDayId = todayId + clampRainDelayDays(sRainDelayDays);
    }
    return sRainDelayAllowDayId > todayId;
}

static bool isPage3ProgramActiveOnDay(const SPage3Program& program,
                                      time_t baseMidnight,
                                      int dayOffset) {
    if (!hasPage3ModeSelection(program)) {
        return false;
    }

    time_t dayTime = baseMidnight + static_cast<time_t>(dayOffset) * 24 * 3600;
    struct tm dayValue;
    memset(&dayValue, 0, sizeof(dayValue));
    localtime_r(&dayTime, &dayValue);
    if (program.weekMode) {
        return dayValue.tm_wday >= 0
                && dayValue.tm_wday < PAGE3_WEEKDAY_COUNT
                && program.weekdays[dayValue.tm_wday];
    }

    int intervalDays = program.intervalDaysSet ? program.intervalDays : 1;
    if (intervalDays < 1) {
        intervalDays = 1;
    }
    return (dayOffset % intervalDays) == 0;
}

static void buildPage3ScheduleEvents(const SPage3Program& program,
                                     int startTimeIndex,
                                     int durationSeconds,
                                     std::vector<SPage3ScheduleEvent> &events) {
    events.clear();
    if (!isPage3StartTimeReady(program, startTimeIndex) || durationSeconds <= 0) {
        return;
    }

    const time_t baseMidnight = getPage3TodayMidnight(time(NULL));
    const SPage3StartTime& startTime = program.startTimes[startTimeIndex];
    const int startSecondOfDay = startTime.hour * 3600 + startTime.minute * 60;
    const int kConflictHorizonDays = 10000;
    for (int dayOffset = 0; dayOffset <= kConflictHorizonDays; ++dayOffset) {
        if (!isPage3ProgramActiveOnDay(program, baseMidnight, dayOffset)) {
            continue;
        }
        SPage3ScheduleEvent event;
        event.startSecond = static_cast<long long>(dayOffset) * 24LL * 3600LL
                + startSecondOfDay;
        event.endSecond = event.startSecond + durationSeconds;
        events.push_back(event);
    }
}

static bool page3StartTimesConflict(const SPage3Program& firstProgram,
                                    int firstStartTimeIndex,
                                    const SPage3Program& secondProgram,
                                    int secondStartTimeIndex,
                                    int durationSeconds) {
    std::vector<SPage3ScheduleEvent> firstEvents;
    std::vector<SPage3ScheduleEvent> secondEvents;
    buildPage3ScheduleEvents(firstProgram, firstStartTimeIndex,
                             durationSeconds, firstEvents);
    buildPage3ScheduleEvents(secondProgram, secondStartTimeIndex,
                             durationSeconds, secondEvents);

    int firstIndex = 0;
    int secondIndex = 0;
    while (firstIndex < static_cast<int>(firstEvents.size())
            && secondIndex < static_cast<int>(secondEvents.size())) {
        const SPage3ScheduleEvent& first = firstEvents[firstIndex];
        const SPage3ScheduleEvent& second = secondEvents[secondIndex];
        if (first.startSecond < second.endSecond
                && second.startSecond < first.endSecond) {
            return true;
        }
        if (first.endSecond <= second.endSecond) {
            ++firstIndex;
        } else {
            ++secondIndex;
        }
    }
    return false;
}

static bool findPage3ConflictForCurrentStartTime(int currentStartTimeIndex,
                                                 char *pText,
                                                 size_t textSize) {
    if (!pText || textSize == 0U) {
        return false;
    }
    pText[0] = '\0';
    initPage3Programs();

    const int durationSeconds = getPage3ProgramRuntimeSeconds();
    if (durationSeconds <= 0) {
        return false;
    }

    const SPage3Program& currentProgram = currentPage3Program();
    if (!hasPage3ModeSelection(currentProgram)
            || !isPage3StartTimeReady(currentProgram, currentStartTimeIndex)) {
        return false;
    }

    for (int programIndex = 0; programIndex < kPage3ProgramCount; ++programIndex) {
        const bool sameProgram = programIndex == sPage3CurrentProgram;
        const SPage3Program& candidateProgram = sPage3Programs[programIndex];
        if (!sameProgram && !candidateProgram.enabled) {
            continue;
        }
        if (!hasPage3ModeSelection(candidateProgram)) {
            continue;
        }
        for (int startTimeIndex = 0; startTimeIndex < kPage3StartTimeCount; ++startTimeIndex) {
            if (sameProgram && startTimeIndex == currentStartTimeIndex) {
                continue;
            }
            if (!isPage3StartTimeReady(candidateProgram, startTimeIndex)) {
                continue;
            }
            if (page3StartTimesConflict(currentProgram, currentStartTimeIndex,
                                        candidateProgram, startTimeIndex,
                                        durationSeconds)) {
                snprintf(pText, textSize,
                         "程序%d的第%d个开启时间\n和当前设置的第%d个开启时间有冲突",
                         programIndex + 1, startTimeIndex + 1,
                         currentStartTimeIndex + 1);
                return true;
            }
        }
    }
    return false;
}

static void checkPage3CurrentStartTimeConflictAfterEdit(int startTimeIndex) {
    char text[160] = {0};
    if (findPage3ConflictForCurrentStartTime(startTimeIndex, text, sizeof(text))) {
        showW3TipWindow(text);
        SPage3Program& program = currentPage3Program();
        SPage3StartTime& startTime = program.startTimes[startTimeIndex];
        startTime.hourReady = false;
        startTime.minuteReady = false;
        startTime.hour = 0;
        startTime.minute = 0;
        syncPage3StartTimeEnabled(startTime);
        program.irrCount = getEnabledStartTimeCount(program);
        updatePage3Controls();
    } else {
        hideW3TipWindowOnly();
    }
}

static bool checkPage3CurrentProgramConflictsBeforeEnable() {
    for (int startTimeIndex = 0; startTimeIndex < kPage3StartTimeCount; ++startTimeIndex) {
        if (!isPage3StartTimeReady(currentPage3Program(), startTimeIndex)) {
            continue;
        }
        char text[160] = {0};
        if (findPage3ConflictForCurrentStartTime(startTimeIndex, text, sizeof(text))) {
            showW3TipWindow(text);
            SPage3Program& program = currentPage3Program();
            SPage3StartTime& startTime = program.startTimes[startTimeIndex];
            startTime.hourReady = false;
            startTime.minuteReady = false;
            startTime.hour = 0;
            startTime.minute = 0;
            syncPage3StartTimeEnabled(startTime);
            program.irrCount = getEnabledStartTimeCount(program);
            return false;
        }
    }
    return true;
}

static bool validatePage3ProgramBeforeEnable() {
    SPage3Program& program = currentPage3Program();
    if (getEnabledStartTimeCount(program) <= 0) {
        showW3TipWindow("请选择开启时间");
        return false;
    }

    if (!hasPage3ModeSelection(program)) {
        showW3TipWindow(program.weekMode ? "请设置星期/隔天模式" : "请设置间隔模式");
        return false;
    }

    if (!hasPage3RunTimeValue()) {
        showW3TipWindow("请设置阀组运行时间");
        return false;
    }

    if (!checkPage3CurrentProgramConflictsBeforeEnable()) {
        return false;
    }

    hideW3TipWindowOnly();
    return true;
}

static void resetPage3ScheduleLastFiredDays() {
    for (int programIndex = 0; programIndex < kPage3ProgramCount; ++programIndex) {
        for (int startTimeIndex = 0; startTimeIndex < kPage3StartTimeCount; ++startTimeIndex) {
            sPage3LastFiredDay[programIndex][startTimeIndex] = -1;
        }
    }
}

static void resetPage3ProgramStartTimeLastFiredDay(int programIndex, int startTimeIndex) {
    if (programIndex < 0 || programIndex >= kPage3ProgramCount
            || startTimeIndex < 0 || startTimeIndex >= kPage3StartTimeCount) {
        return;
    }
    sPage3LastFiredDay[programIndex][startTimeIndex] = -1;
}

static void clearPage3ScheduleState() {
    sPage3ScheduleActive = false;
    sPage3ScheduleProgramIndex = -1;
    sPage3ScheduleStartTimeIndex = -1;
    sPage3ScheduleGroupIndex = -1;
    sPage3ScheduleGroupOpen = false;
    sPage3ScheduleGroupClosing = false;
    sPage3ScheduleCloseAtMs = 0;
    sPage3ScheduleSwitchAtMs = 0;
    sPage3ScheduleGroupEndTime = 0;
    sPage3ScheduleEndTime = 0;
    sPage3SchedulePumpPreOpened = false;
    sPage3ScheduleValveStartTime = 0;
    sPage3ScheduleValveStartAtMs = 0;
    sPage3ScheduleGroups.clear();
    sPage3ScheduleDurationsMs.clear();
}

static void collectPage3ScheduleRunTimes(std::vector<int> &groups,
                                         std::vector<long long> &durationsMs) {
    syncRunTimeItemsWithValveGroups();
    groups.clear();
    durationsMs.clear();
    for (int i = 0; i < static_cast<int>(sRunTimeItems.size()); ++i) {
        const SRunTimeItem& item = sRunTimeItems[i];
        const int durationSeconds = item.hour * 3600 + item.minute * 60 + item.second;
        if (durationSeconds <= 0) {
            continue;
        }
        groups.push_back(item.groupNo);
        durationsMs.push_back(static_cast<long long>(durationSeconds) * 1000LL);
    }
}

static bool findPage3PendingScheduleToday(time_t now, SPage3PendingScheduleInfo *pInfo) {
    initPage3Programs();

    std::vector<int> groups;
    std::vector<long long> durationsMs;
    collectPage3ScheduleRunTimes(groups, durationsMs);
    if (groups.empty()) {
        return false;
    }

    struct tm nowValue;
    memset(&nowValue, 0, sizeof(nowValue));
    localtime_r(&now, &nowValue);
    const int currentMinuteOfDay = nowValue.tm_hour * 60 + nowValue.tm_min;
    const int todayId = nowValue.tm_year * 400 + nowValue.tm_yday;
    const time_t baseMidnight = getPage3TodayMidnight(now);

    bool found = false;
    int bestMinuteOfDay = 24 * 60;
    for (int programIndex = 0; programIndex < kPage3ProgramCount; ++programIndex) {
        const SPage3Program& program = sPage3Programs[programIndex];
        if (!program.enabled || !hasPage3ModeSelection(program)
                || !isPage3ProgramActiveOnDay(program, baseMidnight, 0)) {
            continue;
        }

        for (int startTimeIndex = 0; startTimeIndex < kPage3StartTimeCount; ++startTimeIndex) {
            if (!isPage3StartTimeReady(program, startTimeIndex)) {
                continue;
            }
            if (sPage3LastFiredDay[programIndex][startTimeIndex] == todayId) {
                continue;
            }

            const SPage3StartTime& startTime = program.startTimes[startTimeIndex];
            const int startMinuteOfDay = startTime.hour * 60 + startTime.minute;
            if (startMinuteOfDay < currentMinuteOfDay
                    || startMinuteOfDay >= bestMinuteOfDay) {
                continue;
            }

            bestMinuteOfDay = startMinuteOfDay;
            found = true;
        }
    }

    if (!found) {
        return false;
    }

    if (pInfo != NULL) {
        pInfo->groupNo = groups[0];
        pInfo->startTime = baseMidnight +
                static_cast<time_t>(bestMinuteOfDay) * 60;
    }
    return true;
}

static bool openPage3ScheduleGroup(int groupIndex, long long nowMs) {
    if (groupIndex < 0 || groupIndex >= static_cast<int>(sPage3ScheduleGroups.size())
            || groupIndex >= static_cast<int>(sPage3ScheduleDurationsMs.size())) {
        return false;
    }
    if (isWindow5ValveCommandBusy()) {
        return false;
    }

    const int groupNo = sPage3ScheduleGroups[groupIndex];
    const bool pumpAlreadyOpen = sPage3SchedulePumpPreOpened && groupIndex == 0;
    if (pumpAlreadyOpen) {
        if (!requestWindow5GroupValveState(groupNo, true)) {
            return false;
        }
    } else if (!requestWindow5GroupIrrigationState(groupNo, true)) {
        return false;
    }
    sPage3SchedulePumpPreOpened = false;
    sPage3ScheduleGroupIndex = groupIndex;
    sPage3ScheduleGroupOpen = true;
    sPage3ScheduleGroupClosing = false;
    const long long durationMs = sPage3ScheduleDurationsMs[groupIndex];
    const long long closeLeadMs = getPage3ScheduleCloseLeadMs(groupNo, durationMs);
    sPage3ScheduleCloseAtMs = nowMs + durationMs - closeLeadMs;
    sPage3ScheduleSwitchAtMs = nowMs + durationMs;
    sPage3ScheduleGroupEndTime = time(NULL) +
            static_cast<time_t>((durationMs + 999LL) / 1000LL);
    return true;
}

static bool startPage3ScheduledProgram(int programIndex,
                                       int startTimeIndex,
                                       time_t now,
                                       time_t valveStartTime,
                                       bool preOpenPumpOnly) {
    collectPage3ScheduleRunTimes(sPage3ScheduleGroups, sPage3ScheduleDurationsMs);
    if (sPage3ScheduleGroups.empty()) {
        clearPage3ScheduleState();
        return false;
    }

    sPage3ScheduleActive = true;
    sPage3ScheduleProgramIndex = programIndex;
    sPage3ScheduleStartTimeIndex = startTimeIndex;
    sPage3ScheduleGroupIndex = -1;
    sPage3ScheduleGroupOpen = false;
    sPage3ScheduleGroupClosing = false;
    sPage3ScheduleCloseAtMs = 0;
    sPage3ScheduleSwitchAtMs = 0;
    sPage3ScheduleGroupEndTime = 0;
    sPage3SchedulePumpPreOpened = false;
    sPage3ScheduleValveStartTime = valveStartTime;
    sPage3ScheduleValveStartAtMs = static_cast<long long>(valveStartTime) * 1000LL;
    long long totalDurationMs = 0;
    for (int i = 0; i < static_cast<int>(sPage3ScheduleDurationsMs.size()); ++i) {
        totalDurationMs += sPage3ScheduleDurationsMs[i];
    }
    sPage3ScheduleEndTime = valveStartTime +
            static_cast<time_t>((totalDurationMs + 999LL) / 1000LL);
    long long nowMs = getWindow5NowMs();
    if (nowMs <= 0) {
        nowMs = static_cast<long long>(now) * 1000LL;
    }
    if (preOpenPumpOnly && now < valveStartTime) {
        if (isWindow5ValveCommandBusy()) {
            clearPage3ScheduleState();
            return false;
        }
        sPage3SchedulePumpPreOpened =
                requestWindow5GroupPumpState(sPage3ScheduleGroups[0], true);
        return true;
    }
    if (!openPage3ScheduleGroup(0, nowMs)) {
        clearPage3ScheduleState();
        return false;
    }
    return true;
}

static bool stopPage3ScheduledProgram(bool closeCurrentGroup) {
    if (closeCurrentGroup && sPage3ScheduleActive && sPage3ScheduleGroupOpen
            && sPage3ScheduleGroupIndex >= 0
            && sPage3ScheduleGroupIndex < static_cast<int>(sPage3ScheduleGroups.size())) {
        if (isWindow5ValveCommandBusy()) {
            return false;
        }
        if (!requestWindow5GroupIrrigationState(
                sPage3ScheduleGroups[sPage3ScheduleGroupIndex], false)) {
            return false;
        }
    } else if (closeCurrentGroup && sPage3ScheduleActive && sPage3SchedulePumpPreOpened
            && !sPage3ScheduleGroups.empty()) {
        if (isWindow5ValveCommandBusy()) {
            return false;
        }
        if (!requestWindow5GroupPumpState(sPage3ScheduleGroups[0], false)) {
            return false;
        }
    }
    clearPage3ScheduleState();
    return true;
}

static bool advancePage3ScheduledGroup() {
    if (!sPage3ScheduleActive) {
        return false;
    }
    if (isWindow5ValveCommandBusy()) {
        return false;
    }

    if (sPage3ScheduleGroupOpen) {
        if (sPage3ScheduleGroupIndex >= 0
                && sPage3ScheduleGroupIndex < static_cast<int>(sPage3ScheduleGroups.size())) {
            if (!requestWindow5GroupIrrigationState(
                    sPage3ScheduleGroups[sPage3ScheduleGroupIndex], false)) {
                return false;
            }
            sPage3ScheduleGroupClosing = true;
        }
        sPage3ScheduleGroupOpen = false;
        sPage3ScheduleCloseAtMs = getWindow5NowMs();
        sPage3ScheduleSwitchAtMs = getWindow5NowMs();
        sPage3ScheduleGroupEndTime = time(NULL);
        return true;
    }

    const int nextGroupIndex = sPage3ScheduleGroupIndex + 1;
    if (nextGroupIndex >= static_cast<int>(sPage3ScheduleGroups.size())) {
        clearPage3ScheduleState();
        return true;
    }
    long long nowMs = getWindow5NowMs();
    if (nowMs <= 0) {
        nowMs = static_cast<long long>(time(NULL)) * 1000LL;
    }
    return openPage3ScheduleGroup(nextGroupIndex, nowMs);
}

static bool updateActivePage3Schedule(time_t now) {
    if (!sPage3ScheduleActive) {
        return false;
    }

    long long nowMs = getWindow5NowMs();
    if (nowMs <= 0) {
        nowMs = static_cast<long long>(now) * 1000LL;
    }

    if (sPage3ScheduleProgramIndex < 0
            || sPage3ScheduleProgramIndex >= kPage3ProgramCount
            || !sPage3Programs[sPage3ScheduleProgramIndex].enabled) {
        return !stopPage3ScheduledProgram(true);
    }

    if (!sPage3ScheduleGroupOpen && sPage3ScheduleGroupIndex < 0
            && sPage3ScheduleValveStartAtMs > 0) {
        if (nowMs < sPage3ScheduleValveStartAtMs) {
            return true;
        }
        if (isWindow5ValveCommandBusy()) {
            return true;
        }
        if (!openPage3ScheduleGroup(0, nowMs)) {
            return true;
        }
        return true;
    }

    if (sPage3ScheduleGroupOpen) {
        if (!sPage3ScheduleGroupClosing && sPage3ScheduleCloseAtMs > nowMs) {
            return true;
        }

        if (!sPage3ScheduleGroupClosing) {
            if (isWindow5ValveCommandBusy()) {
                return true;
            }
            if (sPage3ScheduleGroupIndex >= 0
                    && sPage3ScheduleGroupIndex < static_cast<int>(sPage3ScheduleGroups.size())) {
                if (!requestWindow5GroupIrrigationState(
                        sPage3ScheduleGroups[sPage3ScheduleGroupIndex], false)) {
                    return true;
                }
            }
            sPage3ScheduleGroupClosing = true;
        }

        if (sPage3ScheduleSwitchAtMs > nowMs) {
            return true;
        }

        if (isWindow5ValveCommandBusy()) {
            return true;
        }

        sPage3ScheduleGroupOpen = false;
        sPage3ScheduleGroupClosing = false;
        sPage3ScheduleCloseAtMs = 0;
        sPage3ScheduleGroupEndTime = 0;
        return true;
    }

    if (isWindow5ValveCommandBusy()) {
        return true;
    }

    const int nextGroupIndex = sPage3ScheduleGroupIndex + 1;
    if (nextGroupIndex >= static_cast<int>(sPage3ScheduleGroups.size())) {
        sPage3TodayCompletedTime = now;
        clearPage3ScheduleState();
        return false;
    }
    if (!openPage3ScheduleGroup(nextGroupIndex, nowMs)) {
        sPage3ScheduleGroupIndex = nextGroupIndex;
        return true;
    }
    return true;
}

static void updatePage3ScheduledPrograms() {
    initPage3Programs();

    const time_t now = time(NULL);
    if (updateRainDelayAndShouldBlockToday(now)) {
        if (sPage3ScheduleActive) {
            (void)stopPage3ScheduledProgram(true);
        }
        return;
    }
    if (updateActivePage3Schedule(now)) {
        return;
    }
    if (sPage3ScheduleActive) {
        return;
    }

    struct tm nowValue;
    memset(&nowValue, 0, sizeof(nowValue));
    localtime_r(&now, &nowValue);
    const int todayId = nowValue.tm_year * 400 + nowValue.tm_yday;
    const time_t baseMidnight = getPage3TodayMidnight(now);

    for (int programIndex = 0; programIndex < kPage3ProgramCount; ++programIndex) {
        const SPage3Program& program = sPage3Programs[programIndex];
        if (!program.enabled || !hasPage3ModeSelection(program)
                || !isPage3ProgramActiveOnDay(program, baseMidnight, 0)) {
            continue;
        }
        for (int startTimeIndex = 0; startTimeIndex < kPage3StartTimeCount; ++startTimeIndex) {
            if (!isPage3StartTimeReady(program, startTimeIndex)) {
                continue;
            }
            const SPage3StartTime& startTime = program.startTimes[startTimeIndex];
            if (sPage3LastFiredDay[programIndex][startTimeIndex] == todayId) {
                continue;
            }

            std::vector<int> groups;
            std::vector<long long> durationsMs;
            collectPage3ScheduleRunTimes(groups, durationsMs);
            if (groups.empty()) {
                continue;
            }

            const time_t valveStartTime = baseMidnight +
                    static_cast<time_t>(startTime.hour * 3600 + startTime.minute * 60);
            const int advanceSeconds = getIrrGroupPumpAdvanceSeconds(groups[0]);
            const time_t triggerTime = valveStartTime - static_cast<time_t>(advanceSeconds);
            if (now < triggerTime || now >= valveStartTime + 60) {
                continue;
            }

            const bool preOpenPumpOnly = now < valveStartTime;
            if (startPage3ScheduledProgram(
                    programIndex, startTimeIndex, now, valveStartTime, preOpenPumpOnly)) {
                sPage3LastFiredDay[programIndex][startTimeIndex] = todayId;
                return;
            }
        }
    }
}

static void refreshRunTimeListView() {
    syncRunTimeItemsWithValveGroups();
    if (mRunTimeListViewPtr) {
        mRunTimeListViewPtr->refreshListView();
    }
}

static void setRunTimeEditTexts(const SRunTimeItem &item) {
    sRunTimeUpdatingEditTexts = true;
    if (mHourEditTextPtr) {
        mHourEditTextPtr->setText(item.hour);
    }
    if (mMinEditTextPtr) {
        mMinEditTextPtr->setText(item.minute);
    }
    if (mSecEditTextPtr) {
        mSecEditTextPtr->setText(item.second);
    }
    sRunTimeUpdatingEditTexts = false;
}

static void closeSetRunTimeWindow() {
    if (mSetRunTimeWindowPtr) {
        mSetRunTimeWindowPtr->hideWnd();
    }
    sRunTimeWindowOpen = false;
    sRunTimeEditingIndex = -1;
    sRunTimeApplyAllGroups = false;
}

static void closeRunTimeScopeWindow() {
    if (mRunTimeScopeWindowPtr) {
        mRunTimeScopeWindowPtr->hideWnd();
    }
    sRunTimeScopePendingIndex = -1;
}

static void openRunTimeScopeWindow(int index) {
    syncRunTimeItemsWithValveGroups();
    if (index < 0 || index >= static_cast<int>(sRunTimeItems.size())) {
        return;
    }
    sRunTimeScopePendingIndex = index;
    if (mRunTimeScopePromptTextPtr) {
        char text[128] = {0};
        snprintf(text, sizeof(text), "编辑%s的运行时间，\n还是编辑所有阀组？",
                 DeviceDataStore::getIrrGroupName(sRunTimeItems[index].groupNo));
        mRunTimeScopePromptTextPtr->setText(text);
    }
    if (mRunTimeScopeWindowPtr) {
        mRunTimeScopeWindowPtr->showWnd();
    }
}

static void openSetRunTimeWindow(int index) {
    syncRunTimeItemsWithValveGroups();
    if (index < 0 || index >= static_cast<int>(sRunTimeItems.size())) {
        return;
    }

    sRunTimeEditingIndex = index;
    sRunTimeEditValue = sRunTimeItems[index];
    if (mButton16Ptr) {
        char title[96] = {0};
        if (sRunTimeApplyAllGroups) {
            snprintf(title, sizeof(title), "全部阀组 设置运行时间");
        } else {
            snprintf(title, sizeof(title), "当前阀组[%d] 设置运行时间",
                     sRunTimeItems[index].groupNo);
        }
        mButton16Ptr->setText(title);
    }
    setRunTimeEditTexts(sRunTimeEditValue);
    if (mSetRunTimeWindowPtr) {
        mSetRunTimeWindowPtr->showWnd();
    }
    sRunTimeWindowOpen = true;
}

static void chooseRunTimeScope(bool allGroups) {
    const int index = sRunTimeScopePendingIndex;
    closeRunTimeScopeWindow();
    if (index < 0 || index >= static_cast<int>(sRunTimeItems.size())) {
        return;
    }
    sRunTimeApplyAllGroups = allGroups;
    openSetRunTimeWindow(index);
    sRunTimeApplyAllGroups = allGroups;
}

static void normalizeRunTimeEditText(ZKEditText* editText, int &value, int minValue, int maxValue) {
    if (!editText || !sRunTimeWindowOpen || sRunTimeUpdatingEditTexts) {
        return;
    }

    const int normalized = parseRunTimeEditText(editText->getText(), value, minValue, maxValue);
    value = normalized;
    sRunTimeUpdatingEditTexts = true;
    editText->setText(normalized);
    sRunTimeUpdatingEditTexts = false;
}

static void changeRunTimeValue(int &value, int delta, int minValue, int maxValue) {
    if (!sRunTimeWindowOpen) {
        return;
    }
    value = clampRunTimeValue(value + delta, minValue, maxValue);
    setRunTimeEditTexts(sRunTimeEditValue);
}

static bool isValidNetworkIp(const char* ip) {
    return ip && ip[0] != '\0'
            && strcmp(ip, "0.0.0.0") != 0
            && strcmp(ip, "127.0.0.1") != 0;
}

static const char* chooseNetworkStatusPic() {
    // Required priority: Ethernet > WiFi > 4G > disconnected.
    if (ETHERNETMANAGER && ETHERNETMANAGER->isConnected()) {
        return kNetworkStatusEthernetPic;
    }
    if (WIFIMANAGER && WIFIMANAGER->isConnected()
            && isMainWifiInternetValidated()) {
        return kNetworkStatusWifiPic;
    }
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
    if (LTE4GMANAGER
            && LTE4GMANAGER->getPowerState() == E_LTE4G_POWER_ON
            && isValidNetworkIp(LTE4GMANAGER->getIp())) {
        return kNetworkStatus4GPic;
    }
#endif
    return kNetworkStatusNonePic;
}

static void refreshNetworkStatusIcon() {
    if (!mwifistatusPtr) {
        return;
    }

    const char* pic = chooseNetworkStatusPic();
    if (strcmp(sCurrentNetworkStatusPic, pic) == 0) {
        return;
    }

    mwifistatusPtr->setButtonStatusPic(ZK_CONTROL_STATUS_NORMAL, pic);
    mwifistatusPtr->setButtonStatusPic(ZK_CONTROL_STATUS_PRESSED, pic);
    mwifistatusPtr->setButtonStatusPic(ZK_CONTROL_STATUS_SELECTED, pic);
    mwifistatusPtr->setButtonStatusPic(
            ZK_CONTROL_STATUS_PRESSED | ZK_CONTROL_STATUS_SELECTED, pic);
    sCurrentNetworkStatusPic = pic;
}

static bool isHomePumpWorking(int address) {
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || data->address != address) {
            continue;
        }
        const bool bound = DeviceDataStore::isDeviceBoundToAnyIrrGroup(data);
        return bound && data->connected && data->stateKnown && data->state
                && strcmp(data->type, "水泵") == 0;
    }
    return false;
}

static void setPumpIconPic(ZKButton* button, const char* pic, const char** currentPic) {
    if (!button || !pic || !currentPic) {
        return;
    }
    if (*currentPic && strcmp(*currentPic, pic) == 0) {
        return;
    }
    button->setButtonStatusPic(ZK_CONTROL_STATUS_NORMAL, pic);
    button->setButtonStatusPic(ZK_CONTROL_STATUS_PRESSED, pic);
    button->setButtonStatusPic(ZK_CONTROL_STATUS_SELECTED, pic);
    button->setButtonStatusPic(
            ZK_CONTROL_STATUS_PRESSED | ZK_CONTROL_STATUS_SELECTED, pic);
    *currentPic = pic;
}

static void updatePumpIconAnimation() {
    const bool pump1Working = isHomePumpWorking(1);
    const bool pump2Working = isHomePumpWorking(2);

    const int frameCount = sizeof(kPumpIcon1AnimFrames) / sizeof(kPumpIcon1AnimFrames[0]);
    if (frameCount <= 0) {
        setPumpIconPic(mPumpIcon1Ptr, kPumpIconStaticPic, &sPumpIcon1CurrentPic);
        setPumpIconPic(mPumpIcon2Ptr, kPumpIconStaticPic, &sPumpIcon2CurrentPic);
        return;
    }
    if (sPumpIcon1AnimFrame < 0 || sPumpIcon1AnimFrame >= frameCount) {
        sPumpIcon1AnimFrame = 0;
    }

    const char* pic = kPumpIcon1AnimFrames[sPumpIcon1AnimFrame];
    setPumpIconPic(mPumpIcon1Ptr, pump1Working ? pic : kPumpIconStaticPic, &sPumpIcon1CurrentPic);
    setPumpIconPic(mPumpIcon2Ptr, pump2Working ? pic : kPumpIconStaticPic, &sPumpIcon2CurrentPic);
    if (pump1Working || pump2Working) {
        sPumpIcon1AnimFrame = (sPumpIcon1AnimFrame + 1) % frameCount;
    }
}

static void bindHomeSensorControls() {
    if (!mWindow7Ptr) {
        return;
    }
    if (!sHomeRainIconPtr) {
        sHomeRainIconPtr = (ZKButton*)mWindow7Ptr->findControlByID(ID_MAIN_Button48);
    }
    if (!sHomeHumidityIconPtr) {
        sHomeHumidityIconPtr = (ZKButton*)mWindow7Ptr->findControlByID(ID_MAIN_Button49);
    }
    if (!sHomeRainTextPtr) {
        sHomeRainTextPtr = (ZKTextView*)mWindow7Ptr->findControlByID(ID_MAIN_TextView19);
    }
    if (!sHomeHumidityTextPtr) {
        sHomeHumidityTextPtr = (ZKTextView*)mWindow7Ptr->findControlByID(ID_MAIN_TextView18);
    }
}

static const SDATA* findHomeSensorByAddress(int address) {
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && data->address == address) {
            return data;
        }
    }
    return NULL;
}

static bool sensorStatusLooksLikePercent(const char* text) {
    return text && std::strchr(text, '%') != NULL;
}

static void refreshHomeSensorStatus() {
    bindHomeSensorControls();

    const SDATA* rain = findHomeSensorByAddress(kHomeRainSensorAddress);
    if (sHomeRainTextPtr) {
        if (!rain || !rain->connected) {
            sHomeRainTextPtr->setText("雨感未连接");
        } else if (rain->stateKnown) {
            sHomeRainTextPtr->setText(rain->state ? "有雨雪" : "无雨雪");
        } else {
            sHomeRainTextPtr->setText("雨感已连接");
        }
    }

    const SDATA* humidity = findHomeSensorByAddress(kHomeHumiditySensorAddress);
    if (sHomeHumidityTextPtr) {
        if (!humidity || !humidity->connected) {
            sHomeHumidityTextPtr->setText("湿度未连接");
        } else if (sensorStatusLooksLikePercent(humidity->status)) {
            sHomeHumidityTextPtr->setText(humidity->status);
        } else {
            sHomeHumidityTextPtr->setText("湿度已连接");
        }
    }
}

static void updateMainClockDateText() {
    if (!sMainClockDateTextPtr && !sMainClockWeekTextPtr) {
        return;
    }

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    if (!t) {
        return;
    }

    if (sMainClockDateTextPtr) {
        char dateText[16];
        snprintf(dateText, sizeof(dateText), "%04d/%d/%d",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
        if (strcmp(sMainClockDateLastText, dateText) != 0) {
            sMainClockDateTextPtr->setText(dateText);
            strncpy(sMainClockDateLastText, dateText, sizeof(sMainClockDateLastText) - 1);
            sMainClockDateLastText[sizeof(sMainClockDateLastText) - 1] = '\0';
        }
    }

    if (sMainClockWeekTextPtr) {
        static const char* kWeekTexts[] = {
            "周\n日", "周\n一", "周\n二", "周\n三", "周\n四", "周\n五", "周\n六"
        };
        const int weekIndex = (t->tm_wday >= 0 && t->tm_wday <= 6) ? t->tm_wday : 0;
        const char* weekText = kWeekTexts[weekIndex];
        if (strcmp(sMainClockWeekLastText, weekText) != 0) {
            sMainClockWeekTextPtr->setText(weekText);
            strncpy(sMainClockWeekLastText, weekText, sizeof(sMainClockWeekLastText) - 1);
            sMainClockWeekLastText[sizeof(sMainClockWeekLastText) - 1] = '\0';
        }
    }
}

//==============================================================================
// 定时器注册表
//==============================================================================
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{ 0, 1000 },
	{ 1, 100 },
	{ 2, 1000 },
	{ 3, 100 },
};

//==============================================================================
// 界面生命周期函数
//==============================================================================

static void onUI_init() {
	refreshNetworkStatusIcon();
    if (WIFIMANAGER) {
        WIFIMANAGER->addWifiListener(&sMainWifiInternetListener);
        checkCurrentMainWifiIfConnected();
    }

	// 文本初始化
	// 初始化共享设备数据
	DeviceDataStore::initDefaultDevices();
	resetPage3ScheduleLastFiredDays();
	sPage3TodayCompletedTime = 0;
	clearPage3ScheduleState();
	if (mTestAdressEditTextPtr) {
		mTestAdressEditTextPtr->setText("20");
	}
	DisplayPowerManager::syncFromContext();

	setWindow5TestAddressTip("");
	hideWindow5TypePopupOnly();
	initMainPageNavigation();
	hideRainDelayWindowOnly();
	hideHumidityThresholdWindowOnly();
	page6HideCycleTip();
    hideMainWifiTipWindowOnly();
	hideW3TipWindowOnly();
	hideW2SetWindowOnly();
	hideGroupBindWindowOnly();
	closeRunTimeScopeWindow();
	closeSetRunTimeWindow();
	refreshDeviceListViews();
	initMainPageNavigation();
	sPumpIcon1CurrentPic = "";
	sPumpIcon2CurrentPic = "";
	updatePumpIconAnimation();
	refreshHomeSensorStatus();
	refreshWindow8IrrigationState();
	refreshRunStatusValueText();
	updateMainClockDateText();
}

static void onUI_intent(const Intent *intentPtr) {
	if (intentPtr != NULL) {
		//TODO
	}
}

static void onUI_show() {
    refreshDeviceListViews();
    refreshHomeSensorStatus();
    refreshChangeIrrListView();
    checkCurrentMainWifiIfConnected();
	if (consumeOverviewOpenMarker()) {
		showMainPage(BACK_GROUND_BTN_1);
	} else if (consumeDebugOpenMarker()) {
		showMainPage(BACK_GROUND_BTN_5);
	}
}

static void onUI_hide() {
}

static void onUI_quit() {
    if (WIFIMANAGER) {
        WIFIMANAGER->removeWifiListener(&sMainWifiInternetListener);
    }
}

//==============================================================================
// 串口数据回调
//==============================================================================
static void onProtocolDataUpdate(const SProtocolData &data) {
}

//==============================================================================
// 定时器触发函数
//==============================================================================
static bool onUI_Timer(int id) {
	if (id == 0) {
        showPendingMainWifiInternetStatusIfNeeded();
		const bool keepTimer = DisplayPowerManager::onOneSecondTimer();
		updateWindow4RoundIrrigation();
		updatePage2DeviceDiscoveryCountdown();
		updateWindow5DeviceStatePolling();
		refreshHomeSensorStatus();
		updatePage3ScheduledPrograms();
		refreshWindow8IrrigationState();
		refreshRunStatusValueText();
		refreshNetworkStatusIcon();
		updateMainClockDateText();
		return keepTimer;
	}
	if (id == 1) {
		updateWindow5TestAddressTipAutoHide();
        updateMainWifiTipAutoHide();
		return true;
	}
	if (id == 2) {
		refreshWindow8IrrigationState();
		refreshRunStatusValueText();
		return true;
	}
	if (id == 3) {
		updatePumpIconAnimation();
		return true;
	}
	return true;
}

//==============================================================================
// 触摸事件
//==============================================================================
static bool onmainActivityTouchEvent(const MotionEvent &ev) {
    if (blockWindow5ValveCommandTouchIfBusy()) {
        return true;
    }
    if (isPage2DeviceDiscoveryTipActive()) {
        return true;
    }
    hideWindow5TestAddressTipIfVisible();
    if (hideCycleTipIfVisible()) {
        return true;
    }
    if (hideWindow4RoundIrrigationTipIfVisible()) {
        return true;
    }
	if (hideW2ActionTipWindowIfVisible()) {
		return true;
	}
    if (hideMainWifiTipWindowIfVisible()) {
        return true;
    }
	if (hideW3TipWindowIfVisible()) {
		return true;
	}
	if (hideW2TipWindowIfVisible()) {
		return true;
	}
	if (DisplayPowerManager::handleTouchEvent()) {
		return true;
	}
	return false;
}

static bool onButtonClick_wifistatus(ZKButton *pButton) {
	return false;
}

static void onCheckedChanged_PumpCheckbox1(ZKCheckBox* pCheckBox,
		bool isChecked) {
	//LOGD(" Checkbox PumpCheckbox1 checked %d", isChecked);
}

static void onCheckedChanged_PumpCheckbox2(ZKCheckBox* pCheckBox,
		bool isChecked) {
	//LOGD(" Checkbox PumpCheckbox2 checked %d", isChecked);
}

static void onCheckedChanged_PumpCheckbox3(ZKCheckBox* pCheckBox,
		bool isChecked) {
	//LOGD(" Checkbox PumpCheckbox3 checked %d", isChecked);
}

static void onCheckedChanged_PumpCheckbox5(ZKCheckBox* pCheckBox,
		bool isChecked) {
	//LOGD(" Checkbox PumpCheckbox5 checked %d", isChecked);
}

static void onCheckedChanged_PumpCheckbox4(ZKCheckBox* pCheckBox,
		bool isChecked) {
	//LOGD(" Checkbox PumpCheckbox4 checked %d", isChecked);
}
static bool onButtonClick_Button1(ZKButton *pButton) {
	return handleButtonClick_Button1(pButton);
}

static bool onButtonClick_Button2(ZKButton *pButton) {
	return handleButtonClick_Button2(pButton);
}

static bool onButtonClick_Button3(ZKButton *pButton) {
	return handleButtonClick_Button3(pButton);
}

static bool onButtonClick_Button4(ZKButton *pButton) {
	return handleButtonClick_Button4(pButton);
}

static bool onButtonClick_Button5(ZKButton *pButton) {
    LOGD(" ButtonClick Button5 target address prev !!!\n");
    return stepWindow5TestAddress(-1);
}

static bool onButtonClick_Button6(ZKButton *pButton) {
    LOGD(" ButtonClick Button6 source address prev !!!\n");
    return stepWindow5SourceAddress(-1);
}

static bool onButtonClick_Button41(ZKButton *pButton) {
    LOGD(" ButtonClick Button41 source address next !!!\n");
    return stepWindow5SourceAddress(1);
}

static bool onButtonClick_Button42(ZKButton *pButton) {
    LOGD(" ButtonClick Button42 target address next !!!\n");
    return stepWindow5TestAddress(1);
}

static bool onButtonClick_Button7(ZKButton *pButton) {
	if (isClearIrrWindowVisible()) {
		closeClearIrrWindow();
		return false;
	}
	return handleButtonClick_Button7(pButton);
}

static bool onButtonClick_Button8(ZKButton *pButton) {
	if (isClearIrrWindowVisible()) {
		chooseCurrentIrrGroupFromDialog();
		return false;
	}
	return handleButtonClick_Button8(pButton);
}

static bool onButtonClick_Button9(ZKButton *pButton) {
	return handleButtonClick_Button9(pButton);
}

static bool onButtonClick_Button46(ZKButton *pButton) {
    LOGD(" ButtonClick Button46 stop current scheduled start time !!!\n");
    (void)stopPage3ScheduledProgram(true);
    return false;
}

static bool onButtonClick_Button47(ZKButton *pButton) {
    LOGD(" ButtonClick Button47 advance scheduled group !!!\n");
    (void)advancePage3ScheduledGroup();
    return false;
}

static int getListItemCount_DeviceListView(const ZKListView *pListView) {
	return getPage2DeviceListItemCount(pListView);
}

static void obtainListItemData_DeviceListView(ZKListView *pListView,
		ZKListView::ZKListItem *pListItem, int index) {
	obtainPage2DeviceListItemData(pListView, pListItem, index);
}

static void onListItemClick_DeviceListView(ZKListView *pListView, int index,
		int id) {
	onPage2DeviceListItemClick(pListView, index, id);
}

static int getListItemCount_DeviceTipListView(const ZKListView *pListView) {
	return getPage2DeviceTipListItemCount(pListView);
}

static void obtainListItemData_DeviceTipListView(ZKListView *pListView,
		ZKListView::ZKListItem *pListItem, int index) {
	obtainPage2DeviceTipListItemData(pListView, pListItem, index);
}

static void onListItemClick_DeviceTipListView(ZKListView *pListView, int index,
		int id) {
	onPage2DeviceTipListItemClick(pListView, index, id);
}
static void onEditTextChanged_W2_NameEditText(const std::string &text) {
	//LOGD(" onEditTextChanged_ W2_NameEditText %s !!!\n", text.c_str());
	refreshChangeIrrListView();
}

static bool onButtonClick_W2_OkButton(ZKButton *pButton) {
	LOGD(" ButtonClick W2_OkButton !!!\n");
	saveW2SetWindow();
	return false;
}

static bool onButtonClick_W2_DelButton(ZKButton *pButton) {
	LOGD(" ButtonClick W2_DelButton !!!\n");
	deleteSelectedIrrGroupFromOverview();
	return false;
}

static bool onButtonClick_W2_CencelButton(ZKButton *pButton) {
	LOGD(" ButtonClick W2_CencelButton !!!\n");
	closeW2SetWindow();
	return false;
}

static bool onButtonClick_Button45(ZKButton *pButton) {
	LOGD(" ButtonClick Button45 delete current device !!!\n");
	deleteW2SetWindowDevice();
	return false;
}

static int getListItemCount_ChangeIrr_ListView(const ZKListView *pListView) {
	//LOGD("getListItemCount_ChangeIrr_ListView !\n");
	return getChangeIrrListItemCount();
}

static void obtainListItemData_ChangeIrr_ListView(ZKListView *pListView,
		ZKListView::ZKListItem *pListItem, int index) {
	//LOGD(" obtainListItemData_ ChangeIrr_ListView  !!!\n");
	obtainChangeIrrListItemData(pListItem, index);
}

static void onListItemClick_ChangeIrr_ListView(ZKListView *pListView, int index,
		int id) {
	//LOGD(" onListItemClick_ ChangeIrr_ListView  !!!\n");
	onChangeIrrListItemClick(index);
}

static bool onButtonClick_ClearIrr_Button(ZKButton *pButton) {
	LOGD(" ButtonClick ClearIrr_Button !!!\n");
	openClearIrrWindow();
	return false;
}

static bool onButtonClick_Button44(ZKButton *pButton) {
	LOGD(" ButtonClick Button44 choose all irrigation groups !!!\n");
	chooseAllIrrGroupsFromDialog();
	return false;
}


static bool onButtonClick_GroupBind_Button(ZKButton *pButton) {
    LOGD(" ButtonClick GroupBind_Button !!!\n");
    openGroupBindWindow();
    return false;
}
static void onEditTextChanged_GroupNumEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ GroupNumEditText %s !!!\n", text.c_str());
}

static void onEditTextChanged_GroupNameEditText(const std::string &text) {
    // The name is committed only by GroupRenameOkButton.
}

static bool onButtonClick_GroupNameButton(ZKButton *pButton) {
    openGroupRenameWindow();
    return false;
}

static bool onButtonClick_GroupRenameOkButton(ZKButton *pButton) {
    saveGroupRenameWindow();
    return false;
}

static bool onButtonClick_GroupRenameCancelButton(ZKButton *pButton) {
    closeGroupRenameWindow();
    return false;
}

static bool onButtonClick_Button43(ZKButton *pButton) {
    openIrrCapacityWindow();
    return false;
}

static bool onButtonClick_IrrCapacity2Button(ZKButton *pButton) {
    selectIrrCapacity(2);
    return false;
}

static bool onButtonClick_IrrCapacity3Button(ZKButton *pButton) {
    selectIrrCapacity(3);
    return false;
}

static bool onButtonClick_IrrCapacity4Button(ZKButton *pButton) {
    selectIrrCapacity(4);
    return false;
}

static bool onButtonClick_IrrCapacityCancelButton(ZKButton *pButton) {
    closeIrrCapacityWindow(false);
    return false;
}

static bool onButtonClick_IrrCapacityOkButton(ZKButton *pButton) {
    closeIrrCapacityWindow(true);
    return false;
}

static bool onButtonClick_GroupCencelButton(ZKButton *pButton) {
    LOGD(" ButtonClick GroupCencelButton !!!\n");
    closeGroupBindWindow();
    return false;
}

static bool onButtonClick_GroupDelButton(ZKButton *pButton) {
    LOGD(" ButtonClick GroupDelButton !!!\n");
    deleteSelectedIrrGroup();
    return false;
}

static bool onButtonClick_GroupAddButton(ZKButton *pButton) {
    LOGD(" ButtonClick GroupAddButton !!!\n");
    bindSelectedDevicesToIrrGroup();
    return false;
}

static void onEditTextChanged_AddPumpEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ AddPumpEditText %s !!!\n", text.c_str());
}

static void onEditTextChanged_AddSenserEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ AddSenserEditText %s !!!\n", text.c_str());
}

static bool onButtonClick_SelectPumpButton(ZKButton *pButton) {
    LOGD(" ButtonClick SelectPumpButton !!!\n");
    return false;
}

static bool onButtonClick_SelectSenserButton(ZKButton *pButton) {
    LOGD(" ButtonClick SelectSenserButton !!!\n");
    return false;
}

static int getListItemCount_SelectPumpListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_SelectPumpListView !\n");
    return getGroupBindDeviceListItemCount(true);
}

static void obtainListItemData_SelectPumpListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ SelectPumpListView  !!!\n");
    obtainGroupBindDeviceListItemData(pListItem, index, true);
}

static void onListItemClick_SelectPumpListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ SelectPumpListView  !!!\n");
    onGroupBindDeviceListItemClick(index, true);
}

static int getListItemCount_SelectSenserListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_SelectSenserListView !\n");
    return getGroupBindDeviceListItemCount(false);
}

static void obtainListItemData_SelectSenserListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ SelectSenserListView  !!!\n");
    obtainGroupBindDeviceListItemData(pListItem, index, false);
}

static void onListItemClick_SelectSenserListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ SelectSenserListView  !!!\n");
    onGroupBindDeviceListItemClick(index, false);
}
static bool onButtonClick_Button10(ZKButton *pButton) {
    return handlePage3ButtonClick_Button10(pButton);
}

static bool onButtonClick_W3ValuePickerField(ZKButton *pButton) {
    return handlePage3ButtonClick_W3ValuePickerField(pButton);
}

static bool onButtonClick_W3PickerOption(ZKButton *pButton) {
    return handlePage3ButtonClick_W3PickerOption(pButton);
}

static bool onButtonClick_W3TimePickerCancelButton(ZKButton *pButton) {
    closePage3ValuePicker();
    return false;
}

static bool onButtonClick_W3TimePickerConfirmButton(ZKButton *pButton) {
    confirmPage3ValuePicker();
    return false;
}

static bool onButtonClick_W3DayPickerCancelButton(ZKButton *pButton) {
    closePage3ValuePicker();
    return false;
}

static bool onButtonClick_W3DayPickerConfirmButton(ZKButton *pButton) {
    confirmPage3ValuePicker();
    return false;
}

static int getListItemCount_W3TimePickerHourListView(const ZKListView *pListView) {
    return getPage3TimePickerHourItemCount(pListView);
}

static void obtainListItemData_W3TimePickerHourListView(
        ZKListView *pListView, ZKListView::ZKListItem *pListItem, int index) {
    obtainPage3TimePickerHourItemData(pListView, pListItem, index);
}

static void onListItemClick_W3TimePickerHourListView(ZKListView *pListView, int index, int id) {
    onPage3TimePickerHourItemClick(pListView, index, id);
}

static int getListItemCount_W3TimePickerMinuteListView(const ZKListView *pListView) {
    return getPage3TimePickerMinuteItemCount(pListView);
}

static void obtainListItemData_W3TimePickerMinuteListView(
        ZKListView *pListView, ZKListView::ZKListItem *pListItem, int index) {
    obtainPage3TimePickerMinuteItemData(pListView, pListItem, index);
}

static void onListItemClick_W3TimePickerMinuteListView(ZKListView *pListView, int index, int id) {
    onPage3TimePickerMinuteItemClick(pListView, index, id);
}

static int getListItemCount_W3DayPickerListView(const ZKListView *pListView) {
    return getPage3DayPickerItemCount(pListView);
}

static void obtainListItemData_W3DayPickerListView(
        ZKListView *pListView, ZKListView::ZKListItem *pListItem, int index) {
    obtainPage3DayPickerItemData(pListView, pListItem, index);
}

static void onListItemClick_W3DayPickerListView(ZKListView *pListView, int index, int id) {
    onPage3DayPickerItemClick(pListView, index, id);
}

static void onEditTextChanged_StartTimeHour1EditText(const std::string &text) {
    handlePage3EditTextChanged_StartTimeHour1EditText(text);
}

static void onEditTextChanged_StartTimeMin1EditText(const std::string &text) {
    handlePage3EditTextChanged_StartTimeMin1EditText(text);
}

static void onEditTextChanged_StartTimeHour2EditText(const std::string &text) {
    handlePage3EditTextChanged_StartTimeHour2EditText(text);
}

static bool onButtonClick_Button11(ZKButton *pButton) {
    return handlePage3ButtonClick_Button11(pButton);
}

static void onEditTextChanged_StartTimeMin2EditText(const std::string &text) {
    handlePage3EditTextChanged_StartTimeMin2EditText(text);
}

static void onEditTextChanged_StartTimeHour3EditText(const std::string &text) {
    handlePage3EditTextChanged_StartTimeHour3EditText(text);
}

static bool onButtonClick_Button12(ZKButton *pButton) {
    return handlePage3ButtonClick_Button12(pButton);
}

static void onEditTextChanged_StartTimeMin3EditText(const std::string &text) {
    handlePage3EditTextChanged_StartTimeMin3EditText(text);
}

static void onEditTextChanged_StartTimeHour4EditText(const std::string &text) {
    handlePage3EditTextChanged_StartTimeHour4EditText(text);
}

static bool onButtonClick_Button13(ZKButton *pButton) {
    return handlePage3ButtonClick_Button13(pButton);
}

static void onEditTextChanged_StartTimeMin4EditText(const std::string &text) {
    handlePage3EditTextChanged_StartTimeMin4EditText(text);
}
static bool onButtonClick_Button14(ZKButton *pButton) {
    return handlePage3ButtonClick_Button14(pButton);
}
static void onEditTextChanged_ShowProgEditText(const std::string &text) {
    handlePage3EditTextChanged_ShowProgEditText(text);
}

static bool onButtonClick_Button15(ZKButton *pButton) {
    LOGD(" ButtonClick Button15 open CycleWindow !!!\n");
    showCycleWindow();
    return false;
}



static bool onButtonClick_PervProgButton(ZKButton *pButton) {
    return handlePage3ButtonClick_PervProgButton(pButton);
}

static bool onButtonClick_NextProgButton(ZKButton *pButton) {
    return handlePage3ButtonClick_NextProgButton(pButton);
}

static bool onButtonClick_OnOffProgButton(ZKButton *pButton) {
    return handlePage3ButtonClick_OnOffProgButton(pButton);
}

static bool onButtonClick_WeekModeButton(ZKButton *pButton) {
    return handlePage3ButtonClick_WeekModeButton(pButton);
}

static bool onButtonClick_IntervalModeButton(ZKButton *pButton) {
    return handlePage3ButtonClick_IntervalModeButton(pButton);
}

static bool onButtonClick_WednesdayButton(ZKButton *pButton) {
    return handlePage3ButtonClick_WednesdayButton(pButton);
}

static bool onButtonClick_ThursdayButton(ZKButton *pButton) {
    return handlePage3ButtonClick_ThursdayButton(pButton);
}

static bool onButtonClick_FridayButton(ZKButton *pButton) {
    return handlePage3ButtonClick_FridayButton(pButton);
}

static bool onButtonClick_SaturdayButton(ZKButton *pButton) {
    return handlePage3ButtonClick_SaturdayButton(pButton);
}

static bool onButtonClick_EverDayButton(ZKButton *pButton) {
    return handlePage3ButtonClick_EverDayButton(pButton);
}

static bool onButtonClick_SundayButton(ZKButton *pButton) {
    return handlePage3ButtonClick_SundayButton(pButton);
}

static bool onButtonClick_MondayButton(ZKButton *pButton) {
    return handlePage3ButtonClick_MondayButton(pButton);
}

static bool onButtonClick_TuesdayButton(ZKButton *pButton) {
    return handlePage3ButtonClick_TuesdayButton(pButton);
}

static void onEditTextChanged_IntervalDayEditText(const std::string &text) {
    handlePage3EditTextChanged_IntervalDayEditText(text);
}

static bool onButtonClick_Button26(ZKButton *pButton) {
    return handlePage3ButtonClick_Button26(pButton);
}

static bool onButtonClick_Button27(ZKButton *pButton) {
    return handlePage3ButtonClick_Button27(pButton);
}

static bool onButtonClick_OnOffProgTextButton(ZKButton *pButton) {
    return handlePage3ButtonClick_OnOffProgButton(pButton);
}
static int getListItemCount_RunTimeListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_RunTimeListView !\n");
    syncRunTimeItemsWithValveGroups();
    const int itemCount = static_cast<int>(sRunTimeItems.size());
    return itemCount > kRunTimeMinDisplayRows ? itemCount : kRunTimeMinDisplayRows;
}

static void obtainListItemData_RunTimeListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ RunTimeListView  !!!\n");
    if (!pListItem || index < 0) {
        return;
    }

    if (index >= static_cast<int>(sRunTimeItems.size())) {
        setListSubItemText(pListItem, ID_MAIN_RunTimeNameSubItem, "空阀组");
        setListSubItemText(pListItem, ID_MAIN_RunTimeValueSubItem, "");
        LayoutPosition namePosition = pListItem->getPosition();
        namePosition.mLeft = 0;
        namePosition.mTop = 0;
        setListSubItemPosition(
                pListItem, ID_MAIN_RunTimeNameSubItem, namePosition);
        setListSubItemAlignment(
                pListItem, ID_MAIN_RunTimeNameSubItem,
                ZKTextView::E_ALIGN_H_CENTER, ZKTextView::E_ALIGN_V_CENTER);
        setListSubItemVisible(pListItem, ID_MAIN_RunTimeNameSubItem, true);
        setListSubItemVisible(pListItem, ID_MAIN_RunTimeValueSubItem, false);
        return;
    }

    const SRunTimeItem& item = sRunTimeItems[index];
    char nameText[64] = {0};
    snprintf(nameText, sizeof(nameText), "%s", DeviceDataStore::getIrrGroupName(item.groupNo));

    char valueText[64] = {0};
    snprintf(valueText, sizeof(valueText), "时长 %02d小时 %02d分 %02d秒", item.hour, item.minute, item.second);

    setListSubItemText(pListItem, ID_MAIN_RunTimeNameSubItem, nameText);
    setListSubItemText(pListItem, ID_MAIN_RunTimeValueSubItem, valueText);
    LayoutPosition namePosition = pListItem->getPosition();
    namePosition.mLeft = 0;
    namePosition.mTop = 0;
    namePosition.mWidth = 240;
    setListSubItemPosition(pListItem, ID_MAIN_RunTimeNameSubItem, namePosition);
    LayoutPosition valuePosition = pListItem->getPosition();
    valuePosition.mLeft = 240;
    valuePosition.mTop = 0;
    valuePosition.mWidth -= 240;
    setListSubItemPosition(pListItem, ID_MAIN_RunTimeValueSubItem, valuePosition);
    setListSubItemVisible(pListItem, ID_MAIN_RunTimeNameSubItem, true);
    setListSubItemVisible(pListItem, ID_MAIN_RunTimeValueSubItem, true);
}

static void onListItemClick_RunTimeListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ RunTimeListView  !!!\n");
    if (index < 0 || index >= static_cast<int>(sRunTimeItems.size())) {
        return;
    }

    if (id == ID_MAIN_RunTimeNameSubItem || id == ID_MAIN_RunTimeValueSubItem || id == 0) {
        openRunTimeScopeWindow(index);
    }
}

static bool onButtonClick_RunTimeScopeCancelButton(ZKButton *pButton) {
    closeRunTimeScopeWindow();
    return false;
}

static bool onButtonClick_RunTimeScopeCurrentButton(ZKButton *pButton) {
    chooseRunTimeScope(false);
    return false;
}

static bool onButtonClick_RunTimeScopeAllButton(ZKButton *pButton) {
    chooseRunTimeScope(true);
    return false;
}

static bool onButtonClick_Button16(ZKButton *pButton) {
    LOGD(" ButtonClick Button16 !!!\n");
    changeRunTimeValue(sRunTimeEditValue.hour, -1, 0, 23);
    return false;
}

static void onEditTextChanged_HourEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ HourEditText %s !!!\n", text.c_str());
    normalizeRunTimeEditText(mHourEditTextPtr, sRunTimeEditValue.hour, 0, 23);
}

static bool onButtonClick_Button17(ZKButton *pButton) {
    LOGD(" ButtonClick Button17 !!!\n");
    changeRunTimeValue(sRunTimeEditValue.hour, 1, 0, 23);
    return false;
}

static bool onButtonClick_Button18(ZKButton *pButton) {
    LOGD(" ButtonClick Button18 !!!\n");
    changeRunTimeValue(sRunTimeEditValue.minute, -1, 0, 59);
    return false;
}

static void onEditTextChanged_MinEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ MinEditText %s !!!\n", text.c_str());
    normalizeRunTimeEditText(mMinEditTextPtr, sRunTimeEditValue.minute, 0, 59);
}

static bool onButtonClick_Button19(ZKButton *pButton) {
    LOGD(" ButtonClick Button19 !!!\n");
    changeRunTimeValue(sRunTimeEditValue.minute, 1, 0, 59);
    return false;
}

static void onEditTextChanged_SecEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ SecEditText %s !!!\n", text.c_str());
    normalizeRunTimeEditText(mSecEditTextPtr, sRunTimeEditValue.second, 0, 59);
}

static bool onButtonClick_SetRunTimeOKButton(ZKButton *pButton) {
    LOGD(" ButtonClick SetRunTimeOKButton !!!\n");
    normalizeRunTimeEditText(mHourEditTextPtr, sRunTimeEditValue.hour, 0, 23);
    normalizeRunTimeEditText(mMinEditTextPtr, sRunTimeEditValue.minute, 0, 59);
    normalizeRunTimeEditText(mSecEditTextPtr, sRunTimeEditValue.second, 0, 59);
    if (sRunTimeApplyAllGroups) {
        for (int i = 0; i < static_cast<int>(sRunTimeItems.size()); ++i) {
            sRunTimeItems[i].hour = sRunTimeEditValue.hour;
            sRunTimeItems[i].minute = sRunTimeEditValue.minute;
            sRunTimeItems[i].second = sRunTimeEditValue.second;
        }
    } else if (sRunTimeEditingIndex >= 0 && sRunTimeEditingIndex < static_cast<int>(sRunTimeItems.size())) {
        sRunTimeItems[sRunTimeEditingIndex] = sRunTimeEditValue;
    }
    closeSetRunTimeWindow();
    refreshRunTimeListView();
    if (!checkPage3CurrentProgramConflictsBeforeEnable()) {
        updatePage3Controls();
    }
    return false;
}

static bool onButtonClick_SetRunTimeCencelButton(ZKButton *pButton) {
    LOGD(" ButtonClick SetRunTimeCencelButton !!!\n");
    closeSetRunTimeWindow();
    return false;
}

static bool onButtonClick_SetRunTimeZeroButton(ZKButton *pButton) {
    LOGD(" ButtonClick SetRunTimeZeroButton !!!\n");
    sRunTimeEditValue.hour = 0;
    sRunTimeEditValue.minute = 0;
    sRunTimeEditValue.second = 0;
    setRunTimeEditTexts(sRunTimeEditValue);
    return false;
}
static bool onButtonClick_Button20(ZKButton *pButton) {
    LOGD(" ButtonClick Button20 !!!\n");
    return false;
}

static bool onButtonClick_Button22(ZKButton *pButton) {
    LOGD(" ButtonClick Button22 !!!\n");
    return false;
}

static bool onButtonClick_Window4RoundIrrigationOkButton(ZKButton *pButton) {
    LOGD(" ButtonClick Window4RoundIrrigationOkButton !!!\n");
    confirmWindow4RoundIrrigation();
    return false;
}

static bool onButtonClick_Window4RoundIrrigationCancelButton(ZKButton *pButton) {
    LOGD(" ButtonClick Window4RoundIrrigationCancelButton !!!\n");
    hideWindow4RoundIrrigationTipOnly();
    return false;
}

static int getListItemCount_DeviceTestTipsListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_DeviceTestTipsListView !\n");
    return 1;
}

static void obtainListItemData_DeviceTestTipsListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    (void)pListView;
    (void)pListItem;
    (void)index;
}

static void onListItemClick_DeviceTestTipsListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ DeviceTestTipsListView  !!!\n");
}

static int getListItemCount_DeviceTestValueListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_DeviceTestValueListView !\n");
    return getWindow4DeviceCount();
}

static void obtainListItemData_DeviceTestValueListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    if (index < 0 || index >= getWindow4DeviceCount()) {
        return;
    }
    setWindow4DeviceValueItem(pListItem, index);
}

static void onListItemClick_DeviceTestValueListView(ZKListView *pListView, int index, int id) {
    if (index < 0 || index >= getWindow4DeviceCount()) {
        return;
    }

    if (id == ID_MAIN_DeviceTestActionValueSubItem || id == 0) {
        const int deviceIndex = getWindow4OutputDeviceIndex(index);
        const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
        if (data && (std::strcmp(data->type, "电磁阀") == 0)) {
            requestWindow5ValveState(deviceIndex, !data->state);
        } else {
            requestWindow5DeviceState(deviceIndex);
        }
    }
}

static int getListItemCount_GroupTestTipsListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_GroupTestTipsListView !\n");
    return 1;
}

static void obtainListItemData_GroupTestTipsListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    (void)pListView;
    (void)pListItem;
    (void)index;
}

static void onListItemClick_GroupTestTipsListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ GroupTestTipsListView  !!!\n");
}

static int getListItemCount_GroupTestValueListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_GroupTestValueListView !\n");
    return getWindow4GroupListRowCount();
}

static void obtainListItemData_GroupTestValueListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    if (index < 0 || index >= getListItemCount_GroupTestValueListView(pListView)) {
        return;
    }
    setWindow4GroupValueItem(pListItem, index);
}

static void onListItemClick_GroupTestValueListView(ZKListView *pListView, int index, int id) {
    if (index < 0 || index >= getListItemCount_GroupTestValueListView(pListView)) {
        return;
    }

    if (isWindow4BlankGroupRow(index)) {
        return;
    }

    if (id == ID_MAIN_GroupTestActionValueSubItem || id == 0) {
        const int groupNo = index + 1;
        requestWindow5GroupValveState(groupNo, !isWindow4GroupActionOn(groupNo));
    }
}

static bool onButtonClick_Window4RoundIrrigationToggleButton(ZKButton *pButton) {
    LOGD(" ButtonClick Window4RoundIrrigationToggleButton !!!\n");
    if (isWindow4RoundIrrigationEnabled()) {
        stopWindow4RoundIrrigation(true);
        refreshWindow4ListViews();
    } else {
        requestWindow4RoundIrrigationEnable();
    }
    updateWindow4RoundIrrigationToggleButton();
    return false;
}

static bool onButtonClick_Button21(ZKButton *pButton) {
    LOGD(" ButtonClick Button21 !!!\n");
    return false;
}

static int getListItemCount_SenserTestTipsListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_SenserTestTipsListView !\n");
    return 1;
}

static void obtainListItemData_SenserTestTipsListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    (void)pListView;
    (void)pListItem;
    (void)index;
}

static void onListItemClick_SenserTestTipsListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ SenserTestTipsListView  !!!\n");
}

static int getListItemCount_SenserTestValueListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_SenserTestValueListView !\n");
    return getWindow4SensorCount();
}

static void obtainListItemData_SenserTestValueListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    if (index < 0 || index >= getWindow4SensorCount()) {
        return;
    }
    setWindow4SensorValueItem(pListItem, index);
}

static void onListItemClick_SenserTestValueListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ SenserTestValueListView  !!!\n");
}
static bool onButtonClick_ValveAddressPrevButton(ZKButton *pButton) {
    LOGD(" ButtonClick ValveAddressPrevButton !!!\n");
    return stepWindow5ValveAddress(-1);
}

static bool onButtonClick_ValveAddressNextButton(ZKButton *pButton) {
    LOGD(" ButtonClick ValveAddressNextButton !!!\n");
    return stepWindow5ValveAddress(1);
}

static bool onButtonClick_UartValueOnButton(ZKButton *pButton) {
    LOGD(" ButtonClick UartValueOnButton !!!\n");
    sendWindow5ValveOnCommand();
    return false;
}

static bool onButtonClick_UartValueOffButton(ZKButton *pButton) {
    LOGD(" ButtonClick UartValueOffButton !!!\n");
    sendWindow5ValveOffCommand();
    return false;
}

static bool onButtonClick_sys_back(ZKButton *pButton) {
    LOGD(" ButtonClick sys_back !!!\n");
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return true;
    }
    if (sCycleWindowOpen) {
        hideCycleWindowOnly();
        return true;
    }
    if (sCurrentPageIndex == BACK_GROUND_BTN_5 || (mWindow5Ptr && mWindow5Ptr->isWndShow())) {
        EASYUICONTEXT->openActivity("page1topsetActivity");
        return true;
    }
    EASYUICONTEXT->goBack();
    return true;
}

static bool onButtonClick_ScreenshotButton(ZKButton *pButton) {
    LOGD(" ButtonClick ScreenshotButton save to %s !!!\n", kScreenshotSavePath);
    const bool ok = ScreenHelper::screenShot(kScreenshotSavePath);
    LOGD(" ScreenshotButton result = %d !!!\n", ok ? 1 : 0);
    return false;
}

static bool onButtonClick_CycleButton(ZKButton *pButton) {
    LOGD(" ButtonClick CycleButton !!!\n");
    if (isCycleWindowProgramEnabled()) {
        disableCycleWindowProgram();
        hideCycleWindowOnly();
        return false;
    }
    showCycleWindow();
    return false;
}

static bool onButtonClick_CycleCancelButton(ZKButton *pButton) {
    LOGD(" ButtonClick CycleCancelButton !!!\n");
    hideCycleWindowOnly();
    return false;
}

static bool handleWindow3Region1WindowClick() {
    showCycleWindow();
    return true;
}

static bool onButtonClick_Window3Region1Window(ZKButton *pButton) {
    LOGD(" ButtonClick Window3Region1Window !!!\n");
    return handleWindow3Region1WindowClick();
}

static bool onButtonClick_Button23(ZKButton *pButton) {
    LOGD(" ButtonClick Button23 !!!\n");
    return handlePage6ButtonClick_Button23(pButton);
}

static void onEditTextChanged_Cycle1MinStartEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ Cycle1MinStartEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_Cycle1MinStartEditText(text);
}

static void onEditTextChanged_Cycle1HourStartEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ Cycle1HourStartEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_Cycle1HourStartEditText(text);
}

static bool onButtonClick_Button25(ZKButton *pButton) {
    LOGD(" ButtonClick Button25 !!!\n");
    return handlePage6ButtonClick_Button25(pButton);
}

static void onEditTextChanged_Cycle1HourEndEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ Cycle1HourEndEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_Cycle1HourEndEditText(text);
}

static void onEditTextChanged_Cycle1MinEndEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ Cycle1MinEndEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_Cycle1MinEndEditText(text);
}

static void onEditTextChanged_Cycle2MinEndEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ Cycle2MinEndEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_Cycle2MinEndEditText(text);
}

static bool onButtonClick_Button29(ZKButton *pButton) {
    LOGD(" ButtonClick Button29 !!!\n");
    return handlePage6ButtonClick_Button29(pButton);
}

static void onEditTextChanged_Cycle2HourStartEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ Cycle2HourStartEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_Cycle2HourStartEditText(text);
}

static void onEditTextChanged_Cycle2MinStartEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ Cycle2MinStartEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_Cycle2MinStartEditText(text);
}

static bool onButtonClick_Button30(ZKButton *pButton) {
    LOGD(" ButtonClick Button30 !!!\n");
    return handlePage6ButtonClick_Button30(pButton);
}

static void onEditTextChanged_Cycle2HourEndEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ Cycle2HourEndEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_Cycle2HourEndEditText(text);
}

static bool onButtonClick_Button24(ZKButton *pButton) {
    LOGD(" ButtonClick Button24 !!!\n");
    return handlePage6ButtonClick_Button24(pButton);
}

static bool onButtonClick_Button31(ZKButton *pButton) {
    LOGD(" ButtonClick Button31 !!!\n");
    return handlePage6ButtonClick_Button31(pButton);
}

static void onEditTextChanged_IrrigationHourEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ IrrigationHourEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_IrrigationHourEditText(text);
}

static bool onButtonClick_Button32(ZKButton *pButton) {
    LOGD(" ButtonClick Button32 !!!\n");
    return handlePage6ButtonClick_Button32(pButton);
}

static bool onButtonClick_Button33(ZKButton *pButton) {
    LOGD(" ButtonClick Button33 !!!\n");
    return handlePage6ButtonClick_Button33(pButton);
}

static bool onButtonClick_Button34(ZKButton *pButton) {
    LOGD(" ButtonClick Button34 !!!\n");
    return handlePage6ButtonClick_Button34(pButton);
}

static void onEditTextChanged_IrrigationMinEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ IrrigationMinEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_IrrigationMinEditText(text);
}

static void onEditTextChanged_IrrigationSecEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ IrrigationSecEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_IrrigationSecEditText(text);
}

static void onEditTextChanged_SoakSecEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ SoakSecEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_SoakSecEditText(text);
}

static void onEditTextChanged_SoakHourEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ SoakHourEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_SoakHourEditText(text);
}

static bool onButtonClick_Button35(ZKButton *pButton) {
    LOGD(" ButtonClick Button35 !!!\n");
    return handlePage6ButtonClick_Button35(pButton);
}

static void onEditTextChanged_SoakMinEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ SoakMinEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_SoakMinEditText(text);
}

static bool onButtonClick_Button36(ZKButton *pButton) {
    LOGD(" ButtonClick Button36 !!!\n");
    return handlePage6ButtonClick_Button36(pButton);
}

static bool onButtonClick_Button37(ZKButton *pButton) {
    LOGD(" ButtonClick Button37 !!!\n");
    return handlePage6ButtonClick_Button37(pButton);
}

static bool onButtonClick_Button38(ZKButton *pButton) {
    LOGD(" ButtonClick Button38 !!!\n");
    return handlePage6ButtonClick_Button38(pButton);
}

static void onEditTextChanged_IntervalEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ IntervalEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_IntervalEditText(text);
}

static void onEditTextChanged_CycleCountEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ CycleCountEditText %s !!!\n", text.c_str());
    handlePage6EditTextChanged_CycleCountEditText(text);
}

static bool onButtonClick_Button39(ZKButton *pButton) {
    LOGD(" ButtonClick Button39 !!!\n");
    return handlePage6ButtonClick_Button39(pButton);
}
static bool onButtonClick_CycleOKButton(ZKButton *pButton) {
    LOGD(" ButtonClick CycleOKButton !!!\n");
    if (handleCycleWindowOkButton()) {
        hideCycleWindowOnly();
    }
    return false;
}
static void onEditTextChanged_TestAdressEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ TestAdressEditText %s !!!\n", text.c_str());
    handleWindow5TestAddressTextChanged(text);
}
static void onEditTextChanged_ValveAddressEditText(const std::string &text) {
    handleWindow5ValveAddressTextChanged(text);
}
static void onEditTextChanged_SrouceAddressEditText(const std::string &text) {
    handleWindow5SourceAddressTextChanged(text);
}

static bool onButtonClick_TestAdressOkButton(ZKButton *pButton) {
    LOGD(" ButtonClick TestAdressOkButton !!!\n");
    sendWindow5CheckAddressCommand();
    return false;
}
static bool onButtonClick_ChangeAdressOkButton(ZKButton *pButton) {
    LOGD(" ButtonClick ChangeAdressOkButton !!!\n");
    sendWindow5SetAddressCommand();
    return false;
}
static void onCheckedChanged_RadioGroup1(ZKRadioGroup* pRadioGroup, int checkedID) {
    LOGD(" RadioGroup RadioGroup1 checked %d", checkedID);
    updateWindow5DecoderTypeTitle();
    if (checkedID == ID_MAIN_SenserRadioButton) {
        showWindow5TypePopup(true);
    } else if (checkedID == ID_MAIN_ValueRadioButton) {
        showWindow5TypePopup(false);
    }
}
static bool onButtonClick_Button40(ZKButton *pButton) {
    LOGD(" ButtonClick Button40 !!!\n");
    return false;
}

static bool onButtonClick_SenserRadioButton(ZKButton *pButton) {
    LOGD(" ButtonClick SenserRadioButton !!!\n");
    if (mRadioGroup1Ptr) {
        mRadioGroup1Ptr->setCheckedID(ID_MAIN_SenserRadioButton);
    }
    showWindow5TypePopup(true);
    return false;
}

static bool onButtonClick_ValueRadioButton(ZKButton *pButton) {
    LOGD(" ButtonClick ValueRadioButton !!!\n");
    if (mRadioGroup1Ptr) {
        mRadioGroup1Ptr->setCheckedID(ID_MAIN_ValueRadioButton);
    }
    showWindow5TypePopup(false);
    return false;
}

static bool onButtonClick_Window5TypeRainButton(ZKButton *pButton) {
    LOGD(" ButtonClick Window5TypeRainButton !!!\n");
    selectWindow5DecoderSubtype(WINDOW5_DECODER_TYPE_SENSER, "雨量传感器");
    return false;
}

static bool onButtonClick_Window5TypeHumidityButton(ZKButton *pButton) {
    LOGD(" ButtonClick Window5TypeHumidityButton !!!\n");
    selectWindow5DecoderSubtype(WINDOW5_DECODER_TYPE_SENSER, "湿度传感器");
    return false;
}

static bool onButtonClick_Window5TypePressureButton(ZKButton *pButton) {
    LOGD(" ButtonClick Window5TypePressureButton !!!\n");
    selectWindow5DecoderSubtype(WINDOW5_DECODER_TYPE_SENSER, "水压");
    return false;
}

static bool onButtonClick_Window5TypeFlowButton(ZKButton *pButton) {
    LOGD(" ButtonClick Window5TypeFlowButton !!!\n");
    selectWindow5DecoderSubtype(WINDOW5_DECODER_TYPE_SENSER, "流量");
    return false;
}

static bool onButtonClick_Window5TypeACValveButton(ZKButton *pButton) {
    LOGD(" ButtonClick Window5TypeACValveButton !!!\n");
    selectWindow5DecoderSubtype(WINDOW5_DECODER_TYPE_VALUE, "AC电磁阀");
    return false;
}

static bool onButtonClick_Window5TypeDCValveButton(ZKButton *pButton) {
    LOGD(" ButtonClick Window5TypeDCValveButton !!!\n");
    selectWindow5DecoderSubtype(WINDOW5_DECODER_TYPE_VALUE, "DC电磁阀");
    return false;
}

static bool onButtonClick_MustChangeAdressButton(ZKButton *pButton) {
    LOGD(" ButtonClick MustChangeAdressButton !!!\n");
    sendWindow5ForceSetAddressCommand();
    return false;
}
static bool onButtonClick_RunStatusIcon(ZKButton *pButton) {
    LOGD(" ButtonClick RunStatusIcon !!!\n");
    return false;
}

static bool onButtonClick_WaterPressureIcon(ZKButton *pButton) {
    LOGD(" ButtonClick WaterPressureIcon !!!\n");
    return false;
}

static bool onButtonClick_FlowIcon(ZKButton *pButton) {
    LOGD(" ButtonClick FlowIcon !!!\n");
    return false;
}
static bool onButtonClick_PumpIcon1(ZKButton *pButton) {
    LOGD(" ButtonClick PumpIcon1 !!!\n");
    return false;
}

static bool onButtonClick_PumpIcon2(ZKButton *pButton) {
    LOGD(" ButtonClick PumpIcon2 !!!\n");
    return false;
}

static bool onButtonClick_PumpIcon3(ZKButton *pButton) {
    LOGD(" ButtonClick PumpIcon3 !!!\n");
    return false;
}

static bool onButtonClick_PumpIcon4(ZKButton *pButton) {
    LOGD(" ButtonClick PumpIcon4 !!!\n");
    return false;
}

static bool onButtonClick_PumpIcon5(ZKButton *pButton) {
    LOGD(" ButtonClick PumpIcon5 !!!\n");
    return false;
}
