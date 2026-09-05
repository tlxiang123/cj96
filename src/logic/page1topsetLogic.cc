#pragma once
#include "uart/ProtocolSender.h"
#include "DisplayPowerManager.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#define DISPLAY_POWER_TIMER_ID 100
#define REMOTE_UPGRADE_TIMER_ID 101
static const char* kDebugOpenMarkerPath = "/tmp/cj96_open_debug_page";
static const char* kOverviewOpenMarkerPath = "/tmp/cj96_open_overview_page";
static const char* kRemoteUpgradeRequestPath = "/tmp/cj96_tuya_demo/ota_request";
static const char* kRemoteUpgradeStatusPath = "/tmp/cj96_tuya_demo/ota_status";
static bool sDebugPasswordWindowVisible = false;
static bool sRemoteUpgradeWindowVisible = false;
static int sRemoteUpgradeCheckingSeconds = 0;
static bool sRemoteUpgradeCheckTimedOut = false;
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


/**
 * 注册定时器
 * 填充数组用于注册定时器
 * 注意：id不能重复
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{DISPLAY_POWER_TIMER_ID,  1000},
	{REMOTE_UPGRADE_TIMER_ID,  1000},
};

/**
 * 当界面构造时触发
 */
static void onUI_init(){
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	DisplayPowerManager::syncFromContext();
	if (mDebugPasswordWindowPtr) {
		mDebugPasswordWindowPtr->hideWnd();
	}
	if (mRemoteUpgradeWindowPtr) {
		mRemoteUpgradeWindowPtr->hideWnd();
	}
	sDebugPasswordWindowVisible = false;
	sRemoteUpgradeWindowVisible = false;
	sRemoteUpgradeCheckingSeconds = 0;
	sRemoteUpgradeCheckTimedOut = false;
	unlink(kRemoteUpgradeStatusPath);
	if (mRemoteUpgradeProgressBarPtr) {
		mRemoteUpgradeProgressBarPtr->setVisible(false);
		mRemoteUpgradeProgressBarPtr->setProgress(0);
	}
	if (mRemoteUpgradeProgressTextPtr) {
		mRemoteUpgradeProgressTextPtr->setVisible(false);
		mRemoteUpgradeProgressTextPtr->setText("");
	}

}

static void setRemoteUpgradeInfoVisible(bool visible) {
	if (mRemoteUpgradeVersionTextPtr) mRemoteUpgradeVersionTextPtr->setVisible(visible);
	if (mRemoteUpgradeContentTextPtr) mRemoteUpgradeContentTextPtr->setVisible(visible);
	if (mRemoteUpgradeHintTextPtr) mRemoteUpgradeHintTextPtr->setVisible(visible);
	if (mRemoteUpgradeConfirmButtonPtr) mRemoteUpgradeConfirmButtonPtr->setVisible(visible);
	if (mRemoteUpgradeCancelButtonPtr) mRemoteUpgradeCancelButtonPtr->setVisible(visible);
}

static void setRemoteUpgradeProgressVisible(bool visible) {
	if (mRemoteUpgradeProgressBarPtr) mRemoteUpgradeProgressBarPtr->setVisible(visible);
	if (mRemoteUpgradeProgressTextPtr) mRemoteUpgradeProgressTextPtr->setVisible(visible);
}

static void updateRemoteUpgradeProgress() {
	FILE* input = fopen(kRemoteUpgradeStatusPath, "r");
	if (!input) return;
	std::string state;
	std::string message;
	int progress = 0;
	char lineBuffer[256];
	while (fgets(lineBuffer, sizeof(lineBuffer), input)) {
		std::string line(lineBuffer);
		while (!line.empty() && (line[line.size() - 1] == '\n' || line[line.size() - 1] == '\r')) {
			line.erase(line.size() - 1);
		}
		if (line.compare(0, 6, "state=") == 0) state = line.substr(6);
		else if (line.compare(0, 9, "progress=") == 0) progress = std::atoi(line.substr(9).c_str());
		else if (line.compare(0, 8, "message=") == 0) message = line.substr(8);
	}
	fclose(input);
	if (progress < 0) progress = 0;
	if (progress > 100) progress = 100;
	if (state.empty()) return;

	if (state == "checking") {
		if (sRemoteUpgradeCheckTimedOut) return;
		if (sRemoteUpgradeCheckingSeconds < 30) ++sRemoteUpgradeCheckingSeconds;
		if (sRemoteUpgradeCheckingSeconds >= 30) {
			sRemoteUpgradeCheckTimedOut = true;
			setRemoteUpgradeInfoVisible(false);
			if (mRemoteUpgradeCancelButtonPtr) mRemoteUpgradeCancelButtonPtr->setVisible(true);
			setRemoteUpgradeProgressVisible(true);
			if (mRemoteUpgradeProgressBarPtr) mRemoteUpgradeProgressBarPtr->setProgress(0);
			if (mRemoteUpgradeProgressTextPtr) mRemoteUpgradeProgressTextPtr->setText(
				"云端未返回升级包，请先发布升级任务");
			return;
		}
		setRemoteUpgradeInfoVisible(false);
		setRemoteUpgradeProgressVisible(true);
		if (mRemoteUpgradeProgressBarPtr) mRemoteUpgradeProgressBarPtr->setProgress(progress);
		char text[96];
		const char* title = "正在检查更新";
		snprintf(text, sizeof(text), "%s %d%%", title, progress);
		if (mRemoteUpgradeProgressTextPtr) mRemoteUpgradeProgressTextPtr->setText(text);
	} else if (state == "downloading" || state == "ready") {
		sRemoteUpgradeCheckingSeconds = 0;
		sRemoteUpgradeCheckTimedOut = false;
		setRemoteUpgradeInfoVisible(false);
		setRemoteUpgradeProgressVisible(true);
		if (mRemoteUpgradeProgressBarPtr) mRemoteUpgradeProgressBarPtr->setProgress(progress);
		char text[96];
		const char* title = state == "ready" ? "升级包准备完成" : "正在下载升级包";
		snprintf(text, sizeof(text), "%s %d%%", title, progress);
		if (mRemoteUpgradeProgressTextPtr) mRemoteUpgradeProgressTextPtr->setText(text);
	} else if (state == "error") {
		sRemoteUpgradeCheckingSeconds = 0;
		sRemoteUpgradeCheckTimedOut = false;
		setRemoteUpgradeInfoVisible(false);
		if (mRemoteUpgradeCancelButtonPtr) mRemoteUpgradeCancelButtonPtr->setVisible(true);
		setRemoteUpgradeProgressVisible(true);
		if (mRemoteUpgradeProgressBarPtr) mRemoteUpgradeProgressBarPtr->setProgress(progress);
		if (mRemoteUpgradeProgressTextPtr) mRemoteUpgradeProgressTextPtr->setText(
			message.empty() ? "升级失败，请检查网络或云端版本" : message.c_str());
	}
}

/**
 * 当切换到该界面时触发
 */
static void onUI_intent(const Intent *intentPtr) {
    if (intentPtr != NULL) {
        //TODO
    }
}

/*
 * 当界面显示时触发
 */
static void onUI_show() {

}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {

}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {

}

/**
 * 串口数据回调接口
 */
static void onProtocolDataUpdate(const SProtocolData &data) {

}

/**
 * 定时器触发函数
 * 不建议在此函数中写耗时操作，否则将影响UI刷新
 * 参数： id
 *         当前所触发定时器的id，与注册时的id相同
 * 返回值: true
 *             继续运行当前定时器
 *         false
 *             停止运行当前定时器
 */
static bool onUI_Timer(int id){
	switch (id) {
		case DISPLAY_POWER_TIMER_ID:
			return DisplayPowerManager::onOneSecondTimer();
		case REMOTE_UPGRADE_TIMER_ID:
			if (sRemoteUpgradeWindowVisible) updateRemoteUpgradeProgress();
			break;

		default:
			break;
	}
    return true;
}

/**
 * 有新的触摸事件时触发
 * 参数：ev
 *         新的触摸事件
 * 返回值：true
 *            表示该触摸事件在此被拦截，系统不再将此触摸事件传递到控件上
 *         false
 *            触摸事件将继续传递到控件上
 */
static bool onpage1topsetActivityTouchEvent(const MotionEvent &ev) {
    if (DisplayPowerManager::handleTouchEvent()) {
        return true;
    }

    switch (ev.mActionStatus) {
		case MotionEvent::E_ACTION_DOWN://触摸按下
			//LOGD("时刻 = %ld 坐标  x = %d, y = %d", ev.mEventTime, ev.mX, ev.mY);
			break;
		case MotionEvent::E_ACTION_MOVE://触摸滑动
			break;
		case MotionEvent::E_ACTION_UP:  //触摸抬起
			break;
		default:
			break;
	}
	return false;
}

static void hideDebugPasswordWindow() {
	if (mDebugPasswordWindowPtr) {
		mDebugPasswordWindowPtr->hideWnd();
	}
	if (mDebugPasswordEditTextPtr) {
		mDebugPasswordEditTextPtr->setText("");
	}
	if (mDebugPasswordTipTextPtr) {
		mDebugPasswordTipTextPtr->setText("");
	}
	sDebugPasswordWindowVisible = false;
}

static void showDebugPasswordWindow() {
	if (mDebugPasswordEditTextPtr) {
		mDebugPasswordEditTextPtr->setText("");
	}
	if (mDebugPasswordTipTextPtr) {
		mDebugPasswordTipTextPtr->setText("");
	}
	if (mDebugPasswordWindowPtr) {
		mDebugPasswordWindowPtr->showWnd();
	}
	sDebugPasswordWindowVisible = true;
}

static void hideRemoteUpgradeWindow() {
	if (mRemoteUpgradeWindowPtr) {
		mRemoteUpgradeWindowPtr->hideWnd();
	}
	sRemoteUpgradeWindowVisible = false;
	sRemoteUpgradeCheckingSeconds = 0;
	sRemoteUpgradeCheckTimedOut = false;
	setRemoteUpgradeInfoVisible(true);
	setRemoteUpgradeProgressVisible(false);
}

static void showRemoteUpgradeWindow() {
	setRemoteUpgradeInfoVisible(true);
	setRemoteUpgradeProgressVisible(false);
	sRemoteUpgradeCheckingSeconds = 0;
	sRemoteUpgradeCheckTimedOut = false;
	if (mRemoteUpgradeProgressBarPtr) mRemoteUpgradeProgressBarPtr->setProgress(0);
	if (mRemoteUpgradeWindowPtr) {
		mRemoteUpgradeWindowPtr->showWnd();
	}
	sRemoteUpgradeWindowVisible = true;
}

static void requestOpenDebugPageFromMain() {
	FILE* fp = fopen(kDebugOpenMarkerPath, "w");
	if (fp) {
		fputs("1", fp);
		fclose(fp);
	}
	EASYUICONTEXT->goBack();
}

static void requestOpenOverviewPageFromMain() {
	remove(kDebugOpenMarkerPath);
	FILE* fp = fopen(kOverviewOpenMarkerPath, "w");
	if (fp) {
		fputs("1", fp);
		fclose(fp);
	}
	EASYUICONTEXT->goBack();
}

static bool onButtonClick_DebugBtn(ZKButton *pButton) {
	showDebugPasswordWindow();
	return true;
}

static bool onButtonClick_DebugPasswordCancelButton(ZKButton *pButton) {
	hideDebugPasswordWindow();
	return true;
}

static bool onButtonClick_DebugPasswordOkButton(ZKButton *pButton) {
	const std::string password = mDebugPasswordEditTextPtr ? mDebugPasswordEditTextPtr->getText() : "";
	if (password == "88888") {
		hideDebugPasswordWindow();
		requestOpenDebugPageFromMain();
	} else if (mDebugPasswordTipTextPtr) {
		mDebugPasswordTipTextPtr->setText("密码错误");
	}
	return true;
}

static bool onButtonClick_RemoteUpgradeButton(ZKButton *pButton) {
	showRemoteUpgradeWindow();
	return true;
}

static bool onButtonClick_RemoteUpgradeCancelButton(ZKButton *pButton) {
	hideRemoteUpgradeWindow();
	return true;
}

static bool onButtonClick_RemoteUpgradeConfirmButton(ZKButton *pButton) {
	unlink(kRemoteUpgradeStatusPath);
	sRemoteUpgradeCheckingSeconds = 0;
	sRemoteUpgradeCheckTimedOut = false;
	FILE* fp = fopen(kRemoteUpgradeRequestPath, "w");
	if (!fp) {
		LOGD("Remote upgrade request create failed\n");
		setRemoteUpgradeInfoVisible(false);
		setRemoteUpgradeProgressVisible(true);
		if (mRemoteUpgradeProgressTextPtr) mRemoteUpgradeProgressTextPtr->setText("无法开始升级，请检查设备连接");
		return true;
	}
	fputs("check\n", fp);
	fflush(fp);
	fsync(fileno(fp));
	fclose(fp);
	LOGD("Remote upgrade requested\n");
	setRemoteUpgradeInfoVisible(false);
	setRemoteUpgradeProgressVisible(true);
	if (mRemoteUpgradeProgressBarPtr) mRemoteUpgradeProgressBarPtr->setProgress(0);
	if (mRemoteUpgradeProgressTextPtr) mRemoteUpgradeProgressTextPtr->setText("正在检查更新 0%");
	return true;
}

static bool onButtonClick_OpenWifiButton(ZKButton *pButton) {
    //LOGD(" ButtonClick OpenWifiButton !!!\n");
	EASYUICONTEXT->openActivity("wifisettingActivity");
    return false;
}

static bool onButtonClick_OpenNetButton(ZKButton *pButton) {
    //LOGD(" ButtonClick OpenNetButton !!!\n");
	EASYUICONTEXT->openActivity("ethernetsettingActivity");
    return false;
}

static bool onButtonClick_Open4GButton(ZKButton *pButton) {
    //LOGD(" ButtonClick Open4GButton !!!\n");
	EASYUICONTEXT->openActivity("lte4gsettingActivity");
    return false;
}

static bool onButtonClick_sys_back(ZKButton *pButton) {
    //LOGD(" ButtonClick sys_back !!!\n");
	if (sDebugPasswordWindowVisible) {
		hideDebugPasswordWindow();
		return true;
	}
	if (sRemoteUpgradeWindowVisible) {
		hideRemoteUpgradeWindow();
		return true;
	}
	requestOpenOverviewPageFromMain();
    return true;
}

static bool onButtonClick_SetSysTimeBtn(ZKButton *pButton) {
    //LOGD(" ButtonClick SetSysTimeBtn !!!\n");
	EASYUICONTEXT->openActivity("showsysdateActivity");
    return false;
}

static bool onButtonClick_LanBtn(ZKButton *pButton) {
    //LOGD(" ButtonClick LanBtn !!!\n");
    return false;
}

static bool onButtonClick_DisplayBtn(ZKButton *pButton) {
    //LOGD(" ButtonClick DisplayBtn !!!\n");
	EASYUICONTEXT->openActivity("setdisplayActivity");
    return false;
}

static bool onButtonClick_Button2(ZKButton *pButton) {
    LOGD(" ButtonClick Button2 !!!\n");
    return false;
}

static bool onButtonClick_Button3(ZKButton *pButton) {
    LOGD(" ButtonClick Button3 !!!\n");
    return false;
}
