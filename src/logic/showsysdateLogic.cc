#pragma once
#include "uart/ProtocolSender.h"
#include "DisplayPowerManager.h"

#define DISPLAY_POWER_TIMER_ID 100
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

#include "uart/ProtocolSender.h"
#include "utils/TimeHelper.h"

static bool sUpdatingDateEditTexts = false;

static void updateUI_time() {
	char timeStr[20];
	struct tm *t = TimeHelper::getDateTime();

	sprintf(timeStr, "%02d:%02d:%02d", t->tm_hour,t->tm_min,t->tm_sec);
	mTextTimePtr->setText(timeStr); // 注意修改控件名称

	sprintf(timeStr, "%d年%02d月%02d日", 1900 + t->tm_year, t->tm_mon + 1, t->tm_mday);
	mTextDatePtr->setText(timeStr); // 注意修改控件名称

	static const char *day[] = { "日", "一", "二", "三", "四", "五", "六" };
	sprintf(timeStr, "星期%s", day[t->tm_wday]);
	mTextWeekPtr->setText(timeStr); // 注意修改控件名称
}

static void updateDateEditText() {

	char timeStr[20];
	struct tm *t = TimeHelper::getDateTime();
	sUpdatingDateEditTexts = true;
	mYearEditTextPtr->setText(t->tm_year + 1900); //年

	sprintf(timeStr, "%02d", t->tm_mon + 1);//月
	mMonthEditTextPtr->setText(timeStr);

	sprintf(timeStr, "%02d", t->tm_mday);//日
	mDayEditTextPtr->setText(timeStr);

	sprintf(timeStr, "%02d", t->tm_hour);//时
	mHourEditTextPtr->setText(timeStr);

	sprintf(timeStr, "%02d", t->tm_min);//分
	mMinEditTextPtr->setText(timeStr);

	sprintf(timeStr, "%02d", t->tm_sec);//秒
	mSecEditTextPtr->setText(timeStr);
	sUpdatingDateEditTexts = false;
}

static void setSystemTime(int year, int mon, int day, int hour, int min, int sec) {
	struct tm t;
	t.tm_year = year - 1900;//年
	t.tm_mon = mon - 1;			//月
	t.tm_mday = day;				//日
	t.tm_hour = hour;		//时
	t.tm_min = min;		//分
	t.tm_sec = sec;		//秒

	TimeHelper::setDateTime(&t);
}

static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{0,  1000}, //定时器id=0, 时间间隔6秒
	{DISPLAY_POWER_TIMER_ID,  1000},
};

/**
 * 当界面构造时触发
 */
static void onUI_init(){
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	updateUI_time();
	updateDateEditText();
	DisplayPowerManager::syncFromContext();
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
	case 0:
		updateUI_time();
		break;
	case DISPLAY_POWER_TIMER_ID:
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
static bool onshowsysdateActivityTouchEvent(const MotionEvent &ev) {
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
static bool SetEditTextSysData(void) {
	//atoi 将字符串转整型
	if (sUpdatingDateEditTexts) {
		return false;
	}

	int year = atoi(mYearEditTextPtr->getText().c_str());		//年
	int mon = atoi(mMonthEditTextPtr->getText().c_str());			//月
	int day = atoi(mDayEditTextPtr->getText().c_str());				//日
	int hour = atoi(mHourEditTextPtr->getText().c_str());		//时
	int min = atoi(mMinEditTextPtr->getText().c_str());		//分
	int sec = atoi(mSecEditTextPtr->getText().c_str());		//秒
	setSystemTime(year, mon, day, hour, min, sec);
	updateDateEditText();
    return false;
}

static void onEditTextChanged_YearEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ YearEditText %s !!!\n", text.c_str());
	SetEditTextSysData();

}

static void onEditTextChanged_MonthEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ MonthEditText %s !!!\n", text.c_str());
	SetEditTextSysData();
}

static void onEditTextChanged_DayEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ DayEditText %s !!!\n", text.c_str());
	SetEditTextSysData();
}

static void onEditTextChanged_HourEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ HourEditText %s !!!\n", text.c_str());
	SetEditTextSysData();
}

static void onEditTextChanged_MinEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ MinEditText %s !!!\n", text.c_str());
	SetEditTextSysData();
}

static bool onButtonClick_sys_back(ZKButton *pButton) {
    LOGD(" ButtonClick sys_back !!!\n");
    return false;
}
static void onEditTextChanged_SecEditText(const std::string &text) {
    //LOGD(" onEditTextChanged_ SecEditText %s !!!\n", text.c_str());
	SetEditTextSysData();
}
