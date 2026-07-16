#pragma once
#include "uart/ProtocolSender.h"
#include "uart/Rs485Bus.h"
#include "net/NetManager.h"
#include "mainLogic.h"
#include "DeviceDataStore.h"
#include "DisplayPowerManager.h"
#include "utils/BrightnessHelper.h"
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#define WIFIMANAGER            NETMANAGER->getWifiManager()

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
static bool requestWindow5DeviceDiscovery();
static bool isWindow5DeviceDiscoveryRunning();
static void updateWindow5DeviceStatePolling();

#include <control/ZKRadioGroup.h>

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

struct SRunTimeItem {
    int groupNo;
    int hour;
    int minute;
    int second;
};

static std::vector<SRunTimeItem> sRunTimeItems;
static int sRunTimeEditingIndex = -1;
static bool sRunTimeWindowOpen = false;
static bool sRunTimeUpdatingEditTexts = false;
static SRunTimeItem sRunTimeEditValue = {0, 0, 0, 0};
static bool sW3TipWindowVisible = false;
static const int kRunTimeMinDisplayRows = 2;
static int sWindow8TrackedRunningGroup = -1;
static time_t sWindow8StopTime = 0;

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
                                           const char* mode,
                                           int remainingSeconds,
                                           int stopHour,
                                           int stopMinute) {
    setWindow8GroupNumber(mWindow8CompletedGroupTextPtr, completedGroup);
    setWindow8GroupNumber(mWindow8RunningGroupTextPtr, runningGroup);
    setWindow8GroupNumber(mWindow8WaitingGroupTextPtr, waitingGroup);

    const int safeSeconds = remainingSeconds > 0 ? remainingSeconds : 0;
    const int hours = safeSeconds / 3600;
    const int minutes = (safeSeconds % 3600) / 60;
    const int seconds = safeSeconds % 60;
    char line1[64] = {0};
    char line2[64] = {0};
    char line3[64] = {0};
    snprintf(line1, sizeof(line1), "阀组%d%s", runningGroup, mode ? mode : "");
    snprintf(line2, sizeof(line2), "剩余%d小时%d分%d秒", hours, minutes, seconds);
    snprintf(line3, sizeof(line3), "停止时间：%02d:%02d", stopHour, stopMinute);
    if (mWindow8StatusLine1TextPtr) mWindow8StatusLine1TextPtr->setText(line1);
    if (mWindow8StatusLine2TextPtr) mWindow8StatusLine2TextPtr->setText(line2);
    if (mWindow8StatusLine3TextPtr) mWindow8StatusLine3TextPtr->setText(line3);
}

static void clearWindow8IrrigationDisplay() {
    setWindow8GroupNumber(mWindow8CompletedGroupTextPtr, 1);
    setWindow8GroupNumber(mWindow8RunningGroupTextPtr, 2);
    setWindow8GroupNumber(mWindow8WaitingGroupTextPtr, 3);
    if (mWindow8StatusLine1TextPtr) mWindow8StatusLine1TextPtr->setText("");
    if (mWindow8StatusLine2TextPtr) mWindow8StatusLine2TextPtr->setText("");
    if (mWindow8StatusLine3TextPtr) mWindow8StatusLine3TextPtr->setText("");
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

    char groupText[16] = {0};
    snprintf(groupText, sizeof(groupText), "%d", groupNo);

    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data) {
            continue;
        }
        if ((strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0) && (strcmp(data->arre, groupText) == 0)) {
            return true;
        }
    }
    return false;
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
            (atoi(data->arre) == groupNo)) {
            return true;
        }
    }
    return false;
}

static void refreshWindow8IrrigationState() {
    syncRunTimeItemsWithValveGroups();
    int runningIndex = -1;
    for (int i = 0; i < static_cast<int>(sRunTimeItems.size()); ++i) {
        if (isWindow8GroupRunning(sRunTimeItems[i].groupNo)) {
            runningIndex = i;
            break;
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
    if (sWindow8TrackedRunningGroup != runningItem.groupNo) {
        const int durationSeconds = runningItem.hour * 3600 +
                                    runningItem.minute * 60 +
                                    runningItem.second;
        sWindow8TrackedRunningGroup = runningItem.groupNo;
        sWindow8StopTime = now + (durationSeconds > 0 ? durationSeconds : 0);
    }

    const int completedGroup = runningIndex > 0 ?
                               sRunTimeItems[runningIndex - 1].groupNo : 0;
    const int waitingGroup = runningIndex + 1 < static_cast<int>(sRunTimeItems.size()) ?
                             sRunTimeItems[runningIndex + 1].groupNo : 0;
    const int remainingSeconds = sWindow8StopTime > now ?
                                 static_cast<int>(sWindow8StopTime - now) : 0;
    struct tm stopTimeValue;
    memset(&stopTimeValue, 0, sizeof(stopTimeValue));
    localtime_r(&sWindow8StopTime, &stopTimeValue);
    updateWindow8IrrigationDisplay(completedGroup,
                                   runningItem.groupNo,
                                   waitingGroup,
                                   "普通灌溉",
                                   remainingSeconds,
                                   stopTimeValue.tm_hour,
                                   stopTimeValue.tm_min);
    if (mWindow8Ptr) {
        mWindow8Ptr->showWnd();
    }
}

static void setW3TipText(const char* text) {
    if (mW3TipTextViewPtr) {
        mW3TipTextViewPtr->setText(text);
    }
}

static void hideW3TipWindowOnly() {
    if (mW3TipWindowPtr) {
        mW3TipWindowPtr->hideWnd();
    }
    sW3TipWindowVisible = false;
}

static bool hideW3TipWindowIfVisible() {
    if (!sW3TipWindowVisible) {
        return false;
    }

    hideW3TipWindowOnly();
    return true;
}

static void showW3TipWindow(const char* text) {
    setW3TipText(text);
    if (mW3TipWindowPtr) {
        mW3TipWindowPtr->showWnd();
        sW3TipWindowVisible = true;
    }
}

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
    return program.intervalDaysSet || program.afterDaysSet;
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

    hideW3TipWindowOnly();
    return true;
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
}

static void openSetRunTimeWindow(int index) {
    syncRunTimeItemsWithValveGroups();
    if (index < 0 || index >= static_cast<int>(sRunTimeItems.size())) {
        return;
    }

    sRunTimeEditingIndex = index;
    sRunTimeEditValue = sRunTimeItems[index];
    setRunTimeEditTexts(sRunTimeEditValue);
    if (mSetRunTimeWindowPtr) {
        mSetRunTimeWindowPtr->showWnd();
    }
    sRunTimeWindowOpen = true;
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

//==============================================================================
// 定时器注册表
//==============================================================================
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{ 0, 1000 },
};

//==============================================================================
// 界面生命周期函数
//==============================================================================

static void onUI_init() {
	// WiFi 状态初始化

	//usleep(1000 * 5000);
	if (mwifistatusPtr) {
		bool wifiConnected = WIFIMANAGER->isConnected();
		mwifistatusPtr->setSelected(wifiConnected);
	}

	// 文本初始化
	// 初始化共享设备数据
	DeviceDataStore::initDefaultDevices();
	if (mTestAdressEditTextPtr) {
		mTestAdressEditTextPtr->setText("20");
	}
	DisplayPowerManager::syncFromContext();

	setWindow5TestAddressTip("");
	initMainPageNavigation();
	page6HideCycleTip();
	hideW3TipWindowOnly();
	hideW2SetWindowOnly();
	hideGroupBindWindowOnly();
	closeSetRunTimeWindow();
	refreshDeviceListViews();
	initMainPageNavigation();
	refreshWindow8IrrigationState();
}

static void onUI_intent(const Intent *intentPtr) {
	if (intentPtr != NULL) {
		//TODO
	}
}

static void onUI_show() {
    refreshDeviceListViews();
    refreshChangeIrrListView();
}

static void onUI_hide() {
}

static void onUI_quit() {
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
		const bool keepTimer = DisplayPowerManager::onOneSecondTimer();
		updateWindow5DeviceStatePolling();
		refreshWindow8IrrigationState();
		return keepTimer;
	}
	return true;
}

//==============================================================================
// 触摸事件
//==============================================================================
static bool onmainActivityTouchEvent(const MotionEvent &ev) {
    hideWindow5TestAddressTipIfVisible();
    if (hideCycleTipIfVisible()) {
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
	return handleButtonClick_Button5(pButton);
}

static bool onButtonClick_Button7(ZKButton *pButton) {
	return handleButtonClick_Button7(pButton);
}

static bool onButtonClick_Button8(ZKButton *pButton) {
	return handleButtonClick_Button8(pButton);
}

static bool onButtonClick_Button9(ZKButton *pButton) {
	return handleButtonClick_Button9(pButton);
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
static void onEditTextChanged_W2_AddressEditText(const std::string &text) {
	//LOGD(" onEditTextChanged_ W2_AddressEditText %s !!!\n", text.c_str());
	refreshChangeIrrListView();
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
	deleteW2SetWindowDevice();
	return false;
}

static bool onButtonClick_W2_CencelButton(ZKButton *pButton) {
	LOGD(" ButtonClick W2_CencelButton !!!\n");
	closeW2SetWindow();
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
	clearSelectedIrrGroup();
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
    //LOGD(" onEditTextChanged_ GroupNameEditText %s !!!\n", text.c_str());
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
    return handlePage3ButtonClick_Button15(pButton);
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

static void onEditTextChanged_AfterDayEditText(const std::string &text) {
    handlePage3EditTextChanged_AfterDayEditText(text);
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
        return;
    }

    const SRunTimeItem& item = sRunTimeItems[index];
    char nameText[64] = {0};
    snprintf(nameText, sizeof(nameText), "%s", DeviceDataStore::getIrrGroupName(item.groupNo));

    char valueText[64] = {0};
    snprintf(valueText, sizeof(valueText), "时长 %02d小时 %02d分 %02d秒", item.hour, item.minute, item.second);

    setListSubItemText(pListItem, ID_MAIN_RunTimeNameSubItem, nameText);
    setListSubItemText(pListItem, ID_MAIN_RunTimeValueSubItem, valueText);
}

static void onListItemClick_RunTimeListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ RunTimeListView  !!!\n");
    if (index < 0 || index >= static_cast<int>(sRunTimeItems.size())) {
        return;
    }

    if (id == ID_MAIN_RunTimeNameSubItem || id == ID_MAIN_RunTimeValueSubItem || id == 0) {
        openSetRunTimeWindow(index);
    }
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
    if (sRunTimeEditingIndex >= 0 && sRunTimeEditingIndex < static_cast<int>(sRunTimeItems.size())) {
        sRunTimeItems[sRunTimeEditingIndex] = sRunTimeEditValue;
    }
    closeSetRunTimeWindow();
    refreshRunTimeListView();
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

static int getListItemCount_DeviceTestTipsListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_DeviceTestTipsListView !\n");
    return 1;
}

static void obtainListItemData_DeviceTestTipsListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    setListSubItemText(pListItem, ID_MAIN_DeviceTestNumTipSubItem, "设备号");
    setListSubItemText(pListItem, ID_MAIN_DeviceTestNameTipSubItem, "名称");
    setListSubItemText(pListItem, ID_MAIN_DeviceTestActionTipSubItem, "动作");
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
        const SDATA* data = DeviceDataStore::getDevice(index);
        if (data && (std::strcmp(data->type, "电磁阀") == 0)) {
            requestWindow5ValveState(index, !data->state);
        } else {
            requestWindow5DeviceState(index);
        }
    }
}

static int getListItemCount_GroupTestTipsListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_GroupTestTipsListView !\n");
    return 1;
}

static void obtainListItemData_GroupTestTipsListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    setListSubItemText(pListItem, ID_MAIN_GroupTestNumTipSubItem, "组号");
    setListSubItemText(pListItem, ID_MAIN_GroupTestNameTipSubItem, "名称");
    setListSubItemText(pListItem, ID_MAIN_GroupTestActionTipSubItem, "动作");
}

static void onListItemClick_GroupTestTipsListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ GroupTestTipsListView  !!!\n");
}

static int getListItemCount_GroupTestValueListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_GroupTestValueListView !\n");
    return getWindow4GroupCount();
}

static void obtainListItemData_GroupTestValueListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    if (index < 0 || index >= getWindow4GroupCount()) {
        return;
    }
    setWindow4GroupValueItem(pListItem, index);
}

static void onListItemClick_GroupTestValueListView(ZKListView *pListView, int index, int id) {
    if (index < 0 || index >= getWindow4GroupCount()) {
        return;
    }

    if (id == ID_MAIN_GroupTestActionValueSubItem || id == 0) {
        const int groupNo = index + 1;
        requestWindow5GroupValveState(groupNo, !isWindow4GroupActionOn(groupNo));
    }
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
    setListSubItemText(pListItem, ID_MAIN_SubItem13, "设备号");
    setListSubItemText(pListItem, ID_MAIN_SubItem14, "名称");
    setListSubItemText(pListItem, ID_MAIN_SubItem15, "数值");
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
static bool onButtonClick_Button41(ZKButton *pButton) {
    LOGD(" ButtonClick Button41 TEST_VALVE_ON !!!\n");
    sendWindow5UnaddressedValveOnCommand();
    return false;
}

static bool onButtonClick_Button42(ZKButton *pButton) {
    LOGD(" ButtonClick Button42 TEST_VALVE_OFF !!!\n");
    sendWindow5UnaddressedValveOffCommand();
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

static bool onButtonClick_Button28(ZKButton *pButton) {
    LOGD(" ButtonClick Button28 !!!\n");
    return handlePage6ButtonClick_Button28(pButton);
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

static bool onButtonClick_Button39(ZKButton *pButton) {
    LOGD(" ButtonClick Button39 !!!\n");
    return handlePage6ButtonClick_Button39(pButton);
}
static bool onButtonClick_CycleOKButton(ZKButton *pButton) {
    LOGD(" ButtonClick CycleOKButton !!!\n");
    if (handleCycleWindowOkButton()) {
        showMainPage(BACK_GROUND_BTN_3);
    }
    return false;
}
static void onEditTextChanged_TestAdressEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ TestAdressEditText %s !!!\n", text.c_str());
    handleWindow5TestAddressTextChanged(text);
}
static void onEditTextChanged_SrouceAddressEditText(const std::string &text) {
    setWindow5TestAddressTip("");
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
}
static bool onButtonClick_Button40(ZKButton *pButton) {
    LOGD(" ButtonClick Button40 !!!\n");
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
