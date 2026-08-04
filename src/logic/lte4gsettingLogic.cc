#pragma once
#include "uart/ProtocolSender.h"
#include "DisplayPowerManager.h"

#define DISPLAY_POWER_TIMER_ID 100
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
#include "net/LTE4GManager.h"
#endif
#include "net/NetManager.h"
#include "utils/Log.h"
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif
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
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
#define LTE4GMANAGER			NETMANAGER->getLTE4GManager()
static ELTE4GPowerState sLTE4GPowerState = E_LTE4G_UNKNOWN;

enum LTE4GConnectivityState {
	LTE4G_CONNECTIVITY_IDLE = 0,
	LTE4G_CONNECTIVITY_CHECKING,
	LTE4G_CONNECTIVITY_ONLINE,
	LTE4G_CONNECTIVITY_FAILED,
};

static volatile int sLTE4GConnectivityState = LTE4G_CONNECTIVITY_IDLE;
static pthread_mutex_t sLTE4GConnectivityMutex = PTHREAD_MUTEX_INITIALIZER;

static bool isValidLTE4GIp(const char *ip) {
	return (ip != NULL) &&
		   (ip[0] != '\0') &&
		   (strcmp(ip, "0.0.0.0") != 0);
}

static bool isLTE4GPowerBusy(ELTE4GPowerState state) {
	return (state == E_LTE4G_POWER_ONING) || (state == E_LTE4G_POWER_OFFING);
}

static void setLTE4GConnectivityState(int state) {
	pthread_mutex_lock(&sLTE4GConnectivityMutex);
	sLTE4GConnectivityState = state;
	pthread_mutex_unlock(&sLTE4GConnectivityMutex);
}

static int getLTE4GConnectivityState() {
	pthread_mutex_lock(&sLTE4GConnectivityMutex);
	const int state = sLTE4GConnectivityState;
	pthread_mutex_unlock(&sLTE4GConnectivityMutex);
	return state;
}

static bool requestLTE4GNtpProbe(const char *server) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	struct addrinfo *addresses = NULL;
	if (getaddrinfo(server, "123", &hints, &addresses) != 0) {
		return false;
	}

	bool success = false;
	for (struct addrinfo *address = addresses; address != NULL; address = address->ai_next) {
		const int fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
		if (fd < 0) {
			continue;
		}

		struct timeval timeout;
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		unsigned char packet[48];
		memset(packet, 0, sizeof(packet));
		packet[0] = 0x1b;
		const ssize_t sent = sendto(fd, packet, sizeof(packet), 0,
				address->ai_addr, address->ai_addrlen);
		const ssize_t received = sent == (ssize_t)sizeof(packet)
				? recvfrom(fd, packet, sizeof(packet), 0, NULL, NULL) : -1;
		close(fd);

		if (received >= (ssize_t)sizeof(packet)) {
			success = true;
			break;
		}
	}

	freeaddrinfo(addresses);
	return success;
}

static void* lte4gConnectivityWorker(void *arg) {
	(void)arg;
	static const char *servers[] = {
		"ntp.aliyun.com",
		"ntp.tencent.com",
		"ntp.ntsc.ac.cn",
		"cn.pool.ntp.org",
	};

	bool success = false;
	const int serverCount = sizeof(servers) / sizeof(servers[0]);
	for (int i = 0; i < serverCount; ++i) {
		if (requestLTE4GNtpProbe(servers[i])) {
			success = true;
			break;
		}
	}

	setLTE4GConnectivityState(success ? LTE4G_CONNECTIVITY_ONLINE
									 : LTE4G_CONNECTIVITY_FAILED);
	return NULL;
}

static void startLTE4GConnectivityCheck() {
	if (getLTE4GConnectivityState() == LTE4G_CONNECTIVITY_CHECKING) {
		return;
	}

	setLTE4GConnectivityState(LTE4G_CONNECTIVITY_CHECKING);
	pthread_t thread;
	if (pthread_create(&thread, NULL, lte4gConnectivityWorker, NULL) != 0) {
		setLTE4GConnectivityState(LTE4G_CONNECTIVITY_FAILED);
		return;
	}
	pthread_detach(thread);
}

static void refreshLTE4GInfoTexts() {
	mTextIPAddrPtr->setText(LTE4GMANAGER->getIp());
	mTextMacAddrPtr->setText(LTE4GMANAGER->getMacAddr());
	mTextViewVersionPtr->setText(LTE4GMANAGER->getVersion());
	mTextViewManufacturerPtr->setText(LTE4GMANAGER->getManufacturer());
	mTextViewIMEIPtr->setText(LTE4GMANAGER->getIMEI());
	mTextViewIMSIPtr->setText(LTE4GMANAGER->getIMSI());
	mTextViewICCIDPtr->setText(LTE4GMANAGER->getICCID());
}

static void refreshLTE4GPowerUi(ELTE4GPowerState state) {
	sLTE4GPowerState = state;
	const bool busy = isLTE4GPowerBusy(state);
	if (state != E_LTE4G_POWER_ON) {
		if (!busy) {
			setLTE4GConnectivityState(LTE4G_CONNECTIVITY_IDLE);
		}
		mButtonOnOffPtr->setInvalid(busy);
		mButtonOnOffPtr->setSelected(false);
		if ((state == E_LTE4G_POWER_OFF) || (state == E_LTE4G_UNKNOWN)) {
			refreshLTE4GInfoTexts();
		}
		return;
	}

	refreshLTE4GInfoTexts();
	if (!isValidLTE4GIp(LTE4GMANAGER->getIp())) {
		setLTE4GConnectivityState(LTE4G_CONNECTIVITY_FAILED);
		mButtonOnOffPtr->setInvalid(false);
		mButtonOnOffPtr->setSelected(false);
		mTextIPAddrPtr->setText("No network");
		return;
	}

	const int connectivityState = getLTE4GConnectivityState();
	if (connectivityState == LTE4G_CONNECTIVITY_IDLE) {
		startLTE4GConnectivityCheck();
	}

	const int nextConnectivityState = getLTE4GConnectivityState();
	mButtonOnOffPtr->setInvalid(nextConnectivityState == LTE4G_CONNECTIVITY_CHECKING);
	mButtonOnOffPtr->setSelected(nextConnectivityState == LTE4G_CONNECTIVITY_ONLINE);
	if (nextConnectivityState == LTE4G_CONNECTIVITY_CHECKING) {
		mTextIPAddrPtr->setText("Checking...");
	} else if (nextConnectivityState == LTE4G_CONNECTIVITY_FAILED) {
		mTextIPAddrPtr->setText("No network");
	}
}

class MyLTE4GPowerStateListener : public LTE4GManager::ILTE4GPowerStateListener{
public:
	virtual void handleLTE4GPowerState(ELTE4GPowerState state) {
		refreshLTE4GPowerUi(state);
	}
};

static MyLTE4GPowerStateListener sMyLTE4GPowerStateListener;
#endif


/**
 * 注册定时器
 * 填充数组用于注册定时器
 * 注意：id不能重复
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{DISPLAY_POWER_TIMER_ID,  1000},
};

/**
 * 当界面构造时触发
 */
static void onUI_init(){
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	DisplayPowerManager::syncFromContext();
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
	refreshLTE4GPowerUi(LTE4GMANAGER->getPowerState());
	LTE4GMANAGER->addLTE4GPowerStateListener(&sMyLTE4GPowerStateListener);
#endif
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
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
			refreshLTE4GPowerUi(LTE4GMANAGER->getPowerState());
#endif
			return DisplayPowerManager::onOneSecondTimer();

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
static bool onlte4gsettingActivityTouchEvent(const MotionEvent &ev) {
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


static bool onButtonClick_sys_back(ZKButton *pButton) {
    LOGD(" ButtonClick sys_back !!!\n");
    return false;
}

static bool onButtonClick_ButtonMenu(ZKButton *pButton) {
    LOGD(" ButtonClick ButtonMenu !!!\n");
	if (!mWindowMenuModePtr->isWndShow()) {
		mWindowMenuModePtr->showWnd();
	}
	else {
		mWindowMenuModePtr->hideWnd();
	}

    return false;
}

static bool onButtonClick_ButtonOnOff(ZKButton *pButton) {
    LOGD(" ButtonClick ButtonOnOff !!!\n");
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
	ELTE4GPowerState state = LTE4GMANAGER->getPowerState();
	if (state == E_LTE4G_UNKNOWN) {
		state = sLTE4GPowerState;
	}
	if (isLTE4GPowerBusy(state)) {
		return false;
	}

	const bool visibleOn = (pButton != NULL) && pButton->isSelected();
	const bool powerOn = (state == E_LTE4G_POWER_ON) ||
						 ((state == E_LTE4G_UNKNOWN) && visibleOn);
	const bool targetOn = !powerOn;
	mButtonOnOffPtr->setInvalid(true);
	mButtonOnOffPtr->setSelected(targetOn);
	LTE4GMANAGER->setPower(targetOn);
#endif
    return false;
}
