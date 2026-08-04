#pragma once
#include "uart/ProtocolSender.h"
#include "DisplayPowerManager.h"
#include "net/NetManager.h"

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
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define ETHERNETMANAGER        NETMANAGER->getEthernetManager()

static bool sUpdatingDateEditTexts = false;
static int sCalendarYear = 0;
static int sCalendarMonth = 0;
static int sCalendarDay = 0;
static bool sUse24HourFormat = true;
static bool sTimePickerOpen = false;
static const int kMinEditableYear = 2026;
static const int kMaxEditableYear = kMinEditableYear + 100;

enum TimeSyncState {
	TIME_SYNC_IDLE = 0,
	TIME_SYNC_RUNNING,
	TIME_SYNC_SUCCESS,
	TIME_SYNC_FAILED,
};

static volatile int sTimeSyncState = TIME_SYNC_IDLE;
static volatile int sLastSuccessfulServer = -1;
static time_t sPendingNetworkTime = 0;
static pthread_mutex_t sTimeSyncMutex = PTHREAD_MUTEX_INITIALIZER;
static bool sAutoTimeSyncEnabled = false;
static int sLastAutoSyncAttemptDay = -1;
static const char *ETHERNET_INTERFACE_NAME = "eth0";
static const char *kLastNtpServerPath = "/mnt/extsd/cj96_ntp_last_server.txt";

struct TimezoneOption {
	const char *label;
	int offsetMinutes;
};

static const TimezoneOption kTimezoneOptions[] = {
	{"UTC-8 \xE6\xB4\x9B\xE6\x9D\x89\xE7\x9F\xB6", -8 * 60},
	{"UTC-5 \xE7\xBA\xBD\xE7\xBA\xA6", -5 * 60},
	{"UTC+0 \xE4\xBC\xA6\xE6\x95\xA6", 0},
	{"UTC+1 \xE5\xB7\xB4\xE9\xBB\x8E", 1 * 60},
	{"UTC+3 \xE8\x8E\xAB\xE6\x96\xAF\xE7\xA7\x91", 3 * 60},
	{"UTC+5:30 \xE5\x8D\xB0\xE5\xBA\xA6", 5 * 60 + 30},
	{"UTC+7 \xE6\x9B\xBC\xE8\xB0\xB7", 7 * 60},
	{"UTC+8 \xE5\x8C\x97\xE4\xBA\xAC", 8 * 60},
	{"UTC+9 \xE4\xB8\x9C\xE4\xBA\xAC", 9 * 60},
	{"UTC+10 \xE6\x82\x89\xE5\xB0\xBC", 10 * 60},
};
static const int kTimezoneOptionCount = sizeof(kTimezoneOptions) / sizeof(kTimezoneOptions[0]);
static const int kDefaultTimezoneIndex = 7;
static const char *kTimezoneIndexPath = "/mnt/extsd/cj96_timezone_index.txt";
static int sTimezoneIndex = kDefaultTimezoneIndex;
static bool sTimezoneDropdownVisible = false;

static int normalizeTimezoneIndex(int index) {
	if (index < 0 || index >= kTimezoneOptionCount) {
		return kDefaultTimezoneIndex;
	}
	return index;
}

static void loadTimezoneSelection() {
	FILE *fp = fopen(kTimezoneIndexPath, "r");
	if (fp == NULL) {
		sTimezoneIndex = kDefaultTimezoneIndex;
		return;
	}
	int index = kDefaultTimezoneIndex;
	if (fscanf(fp, "%d", &index) != 1) {
		index = kDefaultTimezoneIndex;
	}
	fclose(fp);
	sTimezoneIndex = normalizeTimezoneIndex(index);
}

static void saveTimezoneSelection() {
	FILE *fp = fopen(kTimezoneIndexPath, "w");
	if (fp == NULL) {
		return;
	}
	fprintf(fp, "%d\n", sTimezoneIndex);
	fclose(fp);
}

static int getSelectedTimezoneOffsetSeconds() {
	return kTimezoneOptions[normalizeTimezoneIndex(sTimezoneIndex)].offsetMinutes * 60;
}

static void setTimezoneDropdownVisible(bool visible) {
	sTimezoneDropdownVisible = visible;
	if (mTimezoneDropdownWindowPtr) {
		if (visible) {
			mTimezoneDropdownWindowPtr->showWnd();
		} else {
			mTimezoneDropdownWindowPtr->hideWnd();
		}
	}
}

static void refreshTimezoneControls() {
	sTimezoneIndex = normalizeTimezoneIndex(sTimezoneIndex);
	if (mTimezoneTitleTextPtr) {
		mTimezoneTitleTextPtr->setText("\xE8\xAE\xBE\xE7\xBD\xAE\xE6\x97\xB6\xE5\x8C\xBA");
	}
	if (mTimezoneSelectButtonPtr) {
		mTimezoneSelectButtonPtr->setText(kTimezoneOptions[sTimezoneIndex].label);
	}
	for (int i = 0; i < kTimezoneOptionCount; ++i) {
		ZKButton *button = mTimezoneOptionButtonPtrs[i];
		if (button == NULL) continue;
		button->setText(kTimezoneOptions[i].label);
		button->setSelected(i == sTimezoneIndex);
	}
}

static bool isUsableNetworkIpText(const char *ip) {
	return ip && ip[0] != '\0'
			&& strcmp(ip, "0.0.0.0") != 0
			&& strcmp(ip, "127.0.0.1") != 0
			&& strncmp(ip, "169.254.", 8) != 0;
}

static bool shouldPreferEthernet() {
	return ETHERNETMANAGER
			&& ETHERNETMANAGER->isConnected()
			&& isUsableNetworkIpText(ETHERNETMANAGER->getIp());
}

static bool getInterfaceIpv4Address(const char *interfaceName,
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

static bool bindSocketToInterfaceIpv4(int fd, int family,
		const char *interfaceName) {
	if (!interfaceName || interfaceName[0] == '\0') {
		return true;
	}
	if (family != AF_INET) {
		return false;
	}

	struct in_addr localAddress;
	if (!getInterfaceIpv4Address(interfaceName, &localAddress)) {
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

static bool isValidNtpServerText(const char *server) {
	if (!server || server[0] == '\0') {
		return false;
	}
	size_t length = strlen(server);
	if (length < 3 || length >= 64) {
		return false;
	}
	for (size_t i = 0; i < length; ++i) {
		const char ch = server[i];
		const bool valid = (ch >= '0' && ch <= '9')
				|| (ch >= 'A' && ch <= 'Z')
				|| (ch >= 'a' && ch <= 'z')
				|| ch == '.' || ch == '-';
		if (!valid) {
			return false;
		}
	}
	return true;
}

static bool loadLastNtpServer(char *server, size_t serverSize) {
	if (!server || serverSize == 0) {
		return false;
	}
	server[0] = '\0';
	FILE *fp = fopen(kLastNtpServerPath, "r");
	if (fp == NULL) {
		return false;
	}
	const bool readOk = fgets(server, serverSize, fp) != NULL;
	fclose(fp);
	if (!readOk) {
		server[0] = '\0';
		return false;
	}
	server[strcspn(server, "\r\n")] = '\0';
	if (!isValidNtpServerText(server)) {
		server[0] = '\0';
		return false;
	}
	return true;
}

static void saveLastNtpServer(const char *server) {
	if (!isValidNtpServerText(server)) {
		return;
	}
	FILE *fp = fopen(kLastNtpServerPath, "w");
	if (fp == NULL) {
		return;
	}
	fprintf(fp, "%s\n", server);
	fclose(fp);
}

static int findNtpServerIndex(const char *server, const char *servers[],
		int serverCount) {
	if (!server) {
		return -1;
	}
	for (int i = 0; i < serverCount; ++i) {
		if (strcmp(server, servers[i]) == 0) {
			return i;
		}
	}
	return -1;
}

static bool requestNtpTime(const char *server, const char *preferredInterface,
		time_t *result) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = preferredInterface ? AF_INET : AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	struct addrinfo *addresses = NULL;
	if (getaddrinfo(server, "123", &hints, &addresses) != 0) {
		return false;
	}

	bool success = false;
	for (struct addrinfo *address = addresses; address != NULL; address = address->ai_next) {
		const int fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
		if (fd < 0) continue;
		if (!bindSocketToInterfaceIpv4(fd, address->ai_family,
				preferredInterface)) {
			close(fd);
			continue;
		}

		struct timeval timeout;
		timeout.tv_sec = 4;
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

		if (received < (ssize_t)sizeof(packet)) continue;
		const uint32_t ntpSeconds = ((uint32_t)packet[40] << 24)
				| ((uint32_t)packet[41] << 16)
				| ((uint32_t)packet[42] << 8)
				| (uint32_t)packet[43];
		if (ntpSeconds <= 2208988800U) continue;
		*result = (time_t)(ntpSeconds - 2208988800U);
		success = true;
		break;
	}

	freeaddrinfo(addresses);
	return success;
}

static void* timeSyncWorker(void *arg) {
	static const char *servers[] = {
		"ntp.aliyun.com",
		"ntp.tencent.com",
		"ntp.ntsc.ac.cn",
		"cn.pool.ntp.org",
		"203.107.6.88",
		"106.55.184.199",
		"1.82.219.234",
		"113.141.164.38",
		"113.141.164.39",
	};
	time_t networkTime = 0;
	bool success = false;
	const char *preferredInterface = shouldPreferEthernet()
			? ETHERNET_INTERFACE_NAME : NULL;
	const int serverCount = sizeof(servers) / sizeof(servers[0]);
	char lastSavedServer[64] = {0};
	if (loadLastNtpServer(lastSavedServer, sizeof(lastSavedServer))) {
		if (requestNtpTime(lastSavedServer, preferredInterface, &networkTime)) {
			success = true;
			sLastSuccessfulServer = findNtpServerIndex(lastSavedServer,
					servers, serverCount);
		}
	}
	for (int attempt = 0; attempt < serverCount; ++attempt) {
		if (success) {
			break;
		}
		int index = attempt;
		if (sLastSuccessfulServer >= 0) {
			index = attempt == 0 ? sLastSuccessfulServer
					: (sLastSuccessfulServer + attempt) % serverCount;
		}
		if (requestNtpTime(servers[index], preferredInterface, &networkTime)) {
			success = true;
			sLastSuccessfulServer = index;
			saveLastNtpServer(servers[index]);
			break;
		}
	}
	if (success && sLastSuccessfulServer < 0
			&& isValidNtpServerText(lastSavedServer)) {
		saveLastNtpServer(lastSavedServer);
	}

	pthread_mutex_lock(&sTimeSyncMutex);
	sPendingNetworkTime = success ? networkTime : 0;
	sTimeSyncState = success ? TIME_SYNC_SUCCESS : TIME_SYNC_FAILED;
	pthread_mutex_unlock(&sTimeSyncMutex);
	return NULL;
}

static void updateUI_time() {
	char timeStr[20];
	struct tm *t = TimeHelper::getDateTime();

	sprintf(timeStr, "%d年%02d月%02d日", 1900 + t->tm_year, t->tm_mon + 1, t->tm_mday);
	mTextDatePtr->setText(timeStr); // 注意修改控件名称

	static const char *day[] = { "日", "一", "二", "三", "四", "五", "六" };
	sprintf(timeStr, "星期%s", day[t->tm_wday]);
	mTextWeekPtr->setText(timeStr); // 注意修改控件名称
}

static void updateVisibleSummary() {
	char timeStr[32];
	struct tm *t = TimeHelper::getDateTime();
	sprintf(timeStr, "%d\xE5\xB9\xB4%02d\xE6\x9C\x88%02d\xE6\x97\xA5",
			1900 + t->tm_year, t->tm_mon + 1, t->tm_mday);
	mTextDatePtr->setText(timeStr);
	if (mDigitalClock1Ptr) {
		mDigitalClock1Ptr->setHourFormat(sUse24HourFormat);
	}
	if (mAmPmTextPtr) {
		mAmPmTextPtr->setVisible(!sUse24HourFormat && !sTimePickerOpen);
		if (!sUse24HourFormat) {
			mAmPmTextPtr->setText(t->tm_hour >= 12 ? "PM" : "AM");
		}
	}
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
	memset(&t, 0, sizeof(t));
	t.tm_year = year - 1900;//年
	t.tm_mon = mon - 1;			//月
	t.tm_mday = day;				//日
	t.tm_hour = hour;		//时
	t.tm_min = min;		//分
	t.tm_sec = sec;		//秒

	t.tm_isdst = -1;
	TimeHelper::setDateTime(&t);
}

static int daysInCalendarMonth(int year, int month) {
	static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int result = days[month - 1];
	if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
		result = 29;
	}
	return result;
}

static void refreshDatePicker();

static int clampDateTimeValue(int value, int minimum, int maximum) {
	if (value < minimum) return minimum;
	if (value > maximum) return maximum;
	return value;
}

static bool editTextHasValue(ZKEditText *editText) {
	return editText && !editText->getText().empty();
}

static int editTextTextLength(ZKEditText *editText) {
	return editText ? (int) editText->getText().length() : 0;
}

static int editTextIntValue(ZKEditText *editText, int defaultValue) {
	if (!editText || editText->getText().empty()) {
		return defaultValue;
	}
	return atoi(editText->getText().c_str());
}

static void setEditTextIntValue(ZKEditText *editText, int value, bool twoDigits) {
	if (!editText) return;

	char text[16];
	if (twoDigits) {
		sprintf(text, "%02d", value);
	} else {
		sprintf(text, "%d", value);
	}
	if (editText->getText() != text) {
		editText->setText(text);
	}
}

static void normalizeDateEditTexts(bool fillEmpty, int *outYear, int *outMonth, int *outDay) {
	struct tm *now = TimeHelper::getDateTime();
	const int defaultYear = sCalendarYear > 0 ? sCalendarYear : 1900 + now->tm_year;
	const int defaultMonth = sCalendarMonth > 0 ? sCalendarMonth : now->tm_mon + 1;
	const int defaultDay = sCalendarDay > 0 ? sCalendarDay : now->tm_mday;
	const int yearTextLength = editTextTextLength(mYearEditTextPtr);
	const int yearTextValue = editTextIntValue(mYearEditTextPtr, defaultYear);
	const bool commitYear = fillEmpty
			|| yearTextLength >= 3
			|| (yearTextLength == 1 && yearTextValue != 2)
			|| (yearTextLength == 2 && (yearTextValue < 20 || yearTextValue > 21));
	const bool commitMonth = fillEmpty
			|| editTextTextLength(mMonthEditTextPtr) >= 2
			|| (editTextHasValue(mMonthEditTextPtr)
					&& editTextIntValue(mMonthEditTextPtr, defaultMonth) <= 0);
	const bool commitDay = fillEmpty
			|| editTextTextLength(mDayEditTextPtr) >= 2
			|| (editTextHasValue(mDayEditTextPtr)
					&& editTextIntValue(mDayEditTextPtr, defaultDay) <= 0);

	int year = defaultYear;
	if (commitYear) {
		if (!fillEmpty && yearTextLength == 3) {
			if (yearTextValue <= kMinEditableYear / 10) {
				year = kMinEditableYear;
			} else if (yearTextValue >= kMaxEditableYear / 10) {
				year = kMaxEditableYear;
			} else {
				year = yearTextValue * 10;
			}
		} else {
			year = yearTextValue;
		}
	}
	int month = commitMonth
			? editTextIntValue(mMonthEditTextPtr, defaultMonth) : defaultMonth;
	year = clampDateTimeValue(year, kMinEditableYear, kMaxEditableYear);
	month = clampDateTimeValue(month, 1, 12);

	const int monthDays = daysInCalendarMonth(year, month);
	int day = commitDay
			? editTextIntValue(mDayEditTextPtr, defaultDay) : defaultDay;
	day = clampDateTimeValue(day, 1, monthDays);

	sUpdatingDateEditTexts = true;
	if (commitYear && (fillEmpty || editTextHasValue(mYearEditTextPtr))) {
		setEditTextIntValue(mYearEditTextPtr, year, false);
	}
	if (commitMonth && (fillEmpty || editTextHasValue(mMonthEditTextPtr))) {
		setEditTextIntValue(mMonthEditTextPtr, month, true);
	}
	if (commitDay && (fillEmpty || editTextHasValue(mDayEditTextPtr))) {
		setEditTextIntValue(mDayEditTextPtr, day, true);
	}
	sUpdatingDateEditTexts = false;

	sCalendarYear = year;
	sCalendarMonth = month;
	sCalendarDay = day;
	if (mDatePickerMonthPtr) {
		refreshDatePicker();
	}

	if (outYear) *outYear = year;
	if (outMonth) *outMonth = month;
	if (outDay) *outDay = day;
}

static void normalizeTimeEditTexts(bool fillEmpty, int *outHour, int *outMin, int *outSec) {
	struct tm *now = TimeHelper::getDateTime();
	const bool commitHour = fillEmpty || editTextTextLength(mHourEditTextPtr) >= 2;
	const bool commitMin = fillEmpty || editTextTextLength(mMinEditTextPtr) >= 2;
	const bool commitSec = fillEmpty || editTextTextLength(mSecEditTextPtr) >= 2;
	int hour = commitHour ? editTextIntValue(mHourEditTextPtr, now->tm_hour) : now->tm_hour;
	int min = commitMin ? editTextIntValue(mMinEditTextPtr, now->tm_min) : now->tm_min;
	int sec = commitSec ? editTextIntValue(mSecEditTextPtr, now->tm_sec) : now->tm_sec;
	hour = clampDateTimeValue(hour, 0, 23);
	min = clampDateTimeValue(min, 0, 59);
	sec = clampDateTimeValue(sec, 0, 59);

	sUpdatingDateEditTexts = true;
	if (commitHour && (fillEmpty || editTextHasValue(mHourEditTextPtr))) {
		setEditTextIntValue(mHourEditTextPtr, hour, true);
	}
	if (commitMin && (fillEmpty || editTextHasValue(mMinEditTextPtr))) {
		setEditTextIntValue(mMinEditTextPtr, min, true);
	}
	if (commitSec && (fillEmpty || editTextHasValue(mSecEditTextPtr))) {
		setEditTextIntValue(mSecEditTextPtr, sec, true);
	}
	sUpdatingDateEditTexts = false;

	if (outHour) *outHour = hour;
	if (outMin) *outMin = min;
	if (outSec) *outSec = sec;
}

static int firstWeekdayOfCalendarMonth(int year, int month) {
	struct tm first;
	memset(&first, 0, sizeof(first));
	first.tm_year = year - 1900;
	first.tm_mon = month - 1;
	first.tm_mday = 1;
	first.tm_isdst = -1;
	mktime(&first);
	return first.tm_wday;
}

static void refreshDatePicker() {
	char title[32];
	sprintf(title, "%d\xE5\xB9\xB4%d\xE6\x9C\x88", sCalendarYear, sCalendarMonth);
	if (mDatePickerMonthPtr) mDatePickerMonthPtr->setText(title);

	const int first = firstWeekdayOfCalendarMonth(sCalendarYear, sCalendarMonth);
	const int monthDays = daysInCalendarMonth(sCalendarYear, sCalendarMonth);
	for (int index = 0; index < 42; ++index) {
		ZKButton *button = mDateDayButtonPtrs[index];
		if (button == NULL) continue;
		const int day = index - first + 1;
		if (day < 1 || day > monthDays) {
			button->setVisible(false);
			button->setSelected(false);
			continue;
		}
		char dayText[8];
		sprintf(dayText, "%d", day);
		button->setVisible(true);
		button->setText(dayText);
		button->setSelected(day == sCalendarDay);
	}
}

static void openDatePicker() {
	struct tm *now = TimeHelper::getDateTime();
	sCalendarYear = 1900 + now->tm_year;
	sCalendarMonth = 1 + now->tm_mon;
	sCalendarDay = now->tm_mday;
	updateDateEditText();
	normalizeDateEditTexts(true, NULL, NULL, NULL);
	refreshDatePicker();
	mDatePickerWindowPtr->showWnd();
}

static void openTimePicker() {
	updateDateEditText();
	normalizeTimeEditTexts(true, NULL, NULL, NULL);
	sTimePickerOpen = true;
	if (mDigitalClock1Ptr) mDigitalClock1Ptr->setVisible(false);
	if (mAmPmTextPtr) mAmPmTextPtr->setVisible(false);
	mTimePickerWindowPtr->showWnd();
}

static bool onButtonClick_DateRowButton(ZKButton *pButton) {
	if (mDatePickerWindowPtr) {
		openDatePicker();
	}
	return false;
}

static bool onButtonClick_TimeRowButton(ZKButton *pButton) {
	if (mTimePickerWindowPtr) {
		openTimePicker();
	}
	return false;
}

static bool onButtonClick_DatePrevButton(ZKButton *pButton) {
	if (sCalendarYear == kMinEditableYear && sCalendarMonth == 1) {
		return false;
	}
	if (--sCalendarMonth < 1) {
		sCalendarMonth = 12;
		--sCalendarYear;
	}
	const int monthDays = daysInCalendarMonth(sCalendarYear, sCalendarMonth);
	if (sCalendarDay > monthDays) sCalendarDay = monthDays;
	refreshDatePicker();
	return false;
}

static bool onButtonClick_DateNextButton(ZKButton *pButton) {
	if (sCalendarYear == kMaxEditableYear && sCalendarMonth == 12) {
		return false;
	}
	if (++sCalendarMonth > 12) {
		sCalendarMonth = 1;
		++sCalendarYear;
	}
	const int monthDays = daysInCalendarMonth(sCalendarYear, sCalendarMonth);
	if (sCalendarDay > monthDays) sCalendarDay = monthDays;
	refreshDatePicker();
	return false;
}

static bool onButtonClick_DateCancelButton(ZKButton *pButton) {
	mDatePickerWindowPtr->hideWnd();
	return false;
}

static bool onButtonClick_DateDoneButton(ZKButton *pButton) {
	struct tm *now = TimeHelper::getDateTime();
	int year = 0;
	int month = 0;
	int day = 0;
	normalizeDateEditTexts(true, &year, &month, &day);
	setSystemTime(year, month, day,
			now->tm_hour, now->tm_min, now->tm_sec);
	mDatePickerWindowPtr->hideWnd();
	updateUI_time();
	updateVisibleSummary();
	updateDateEditText();
	return false;
}

static void onButtonClick_DateDayButton(ZKButton *pButton, int index) {
	const int day = index - firstWeekdayOfCalendarMonth(sCalendarYear, sCalendarMonth) + 1;
	if (day >= 1 && day <= daysInCalendarMonth(sCalendarYear, sCalendarMonth)) {
		sCalendarDay = day;
		refreshDatePicker();
	}
}

static bool onButtonClick_TimeCancelButton(ZKButton *pButton) {
	mTimePickerWindowPtr->hideWnd();
	sTimePickerOpen = false;
	if (mDigitalClock1Ptr) mDigitalClock1Ptr->setVisible(true);
	updateDateEditText();
	updateVisibleSummary();
	return false;
}

static bool onButtonClick_TimeDoneButton(ZKButton *pButton) {
	struct tm *now = TimeHelper::getDateTime();
	int hour = 0;
	int min = 0;
	int sec = 0;
	normalizeTimeEditTexts(true, &hour, &min, &sec);
	setSystemTime(1900 + now->tm_year, 1 + now->tm_mon, now->tm_mday, hour, min, sec);
	mTimePickerWindowPtr->hideWnd();
	sTimePickerOpen = false;
	if (mDigitalClock1Ptr) mDigitalClock1Ptr->setVisible(true);
	updateUI_time();
	updateVisibleSummary();
	updateDateEditText();
	return false;
}

static bool onButtonClick_SyncTimeButton(ZKButton *pButton) {
	if (sTimeSyncState != TIME_SYNC_IDLE) {
		return false;
	}
	setTimezoneDropdownVisible(false);
	if (sAutoTimeSyncEnabled) {
		sAutoTimeSyncEnabled = false;
		pButton->setSelected(false);
		return false;
	}
	sTimeSyncState = TIME_SYNC_RUNNING;
	pButton->setSelected(false);
	pButton->setInvalid(true);
	pthread_t worker;
	if (pthread_create(&worker, NULL, timeSyncWorker, NULL) != 0) {
		sTimeSyncState = TIME_SYNC_IDLE;
		pButton->setInvalid(false);
		if (mSyncFailureWindowPtr) mSyncFailureWindowPtr->showWnd();
		return false;
	}
	pthread_detach(worker);
	return false;
}

static bool onButtonClick_SyncFailureOkButton(ZKButton *pButton) {
	if (mSyncFailureWindowPtr) mSyncFailureWindowPtr->hideWnd();
	return false;
}

static bool onButtonClick_TwentyFourHourButton(ZKButton *pButton) {
	sUse24HourFormat = !sUse24HourFormat;
	if (mTwentyFourHourButtonPtr) {
		mTwentyFourHourButtonPtr->setSelected(sUse24HourFormat);
	}
	if (mTwentyFourHourSwitchButtonPtr) {
		mTwentyFourHourSwitchButtonPtr->setSelected(sUse24HourFormat);
	}
	updateVisibleSummary();
	return false;
}

static bool onButtonClick_TimezoneSelectButton(ZKButton *pButton) {
	setTimezoneDropdownVisible(!sTimezoneDropdownVisible);
	return false;
}

static bool onButtonClick_TimezoneOptionButton(ZKButton *pButton) {
	if (pButton == NULL) {
		return false;
	}
	const int index = pButton->getID() - ID_SHOWSYSDATE_TimezoneOptionButtonFirst;
	if (index < 0 || index >= kTimezoneOptionCount) {
		return false;
	}
	sTimezoneIndex = index;
	saveTimezoneSelection();
	refreshTimezoneControls();
	setTimezoneDropdownVisible(false);
	return false;
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
	updateVisibleSummary();
	updateDateEditText();
	DisplayPowerManager::syncFromContext();
	if (mDatePickerWindowPtr) mDatePickerWindowPtr->setVisible(false);
	if (mTimePickerWindowPtr) mTimePickerWindowPtr->setVisible(false);
	if (mSyncFailureWindowPtr) mSyncFailureWindowPtr->setVisible(false);
	loadTimezoneSelection();
	setTimezoneDropdownVisible(false);
	refreshTimezoneControls();
	if (mSyncTimeButtonPtr) {
		mSyncTimeButtonPtr->setInvalid(false);
		mSyncTimeButtonPtr->setSelected(sAutoTimeSyncEnabled);
	}
	if (mTwentyFourHourButtonPtr) mTwentyFourHourButtonPtr->setSelected(sUse24HourFormat);
	if (mTwentyFourHourSwitchButtonPtr) mTwentyFourHourSwitchButtonPtr->setSelected(sUse24HourFormat);
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
		if (sTimeSyncState == TIME_SYNC_SUCCESS || sTimeSyncState == TIME_SYNC_FAILED) {
			pthread_mutex_lock(&sTimeSyncMutex);
			bool success = sTimeSyncState == TIME_SYNC_SUCCESS;
			const time_t networkTime = sPendingNetworkTime;
			sPendingNetworkTime = 0;
			sTimeSyncState = TIME_SYNC_IDLE;
			pthread_mutex_unlock(&sTimeSyncMutex);
			if (success) {
				struct tm localTime;
				const time_t selectedLocalTime = networkTime + getSelectedTimezoneOffsetSeconds();
				if (networkTime == 0 || gmtime_r(&selectedLocalTime, &localTime) == NULL) {
					success = false;
				} else {
					TimeHelper::setDateTime(&localTime);
					if (mActivityPtr) {
						mActivityPtr->resetUserTimer(0, 1000);
						mActivityPtr->resetUserTimer(DISPLAY_POWER_TIMER_ID, 1000);
					}
				}
			}
			sAutoTimeSyncEnabled = success;
			if (success) {
				struct tm *synced = TimeHelper::getDateTime();
				sLastAutoSyncAttemptDay = (1900 + synced->tm_year) * 1000 + synced->tm_yday;
			}
			if (mSyncTimeButtonPtr) {
				mSyncTimeButtonPtr->setInvalid(false);
				mSyncTimeButtonPtr->setSelected(success);
			}
			if (!success && mSyncFailureWindowPtr) {
				mSyncFailureWindowPtr->showWnd();
			}
		}
		if (sAutoTimeSyncEnabled && sTimeSyncState == TIME_SYNC_IDLE) {
			struct tm *now = TimeHelper::getDateTime();
			const int dayStamp = (1900 + now->tm_year) * 1000 + now->tm_yday;
			if (now->tm_hour == 0 && dayStamp != sLastAutoSyncAttemptDay) {
				sLastAutoSyncAttemptDay = dayStamp;
				sTimeSyncState = TIME_SYNC_RUNNING;
				if (mSyncTimeButtonPtr) mSyncTimeButtonPtr->setInvalid(true);
				pthread_t worker;
				if (pthread_create(&worker, NULL, timeSyncWorker, NULL) == 0) {
					pthread_detach(worker);
				} else {
					sTimeSyncState = TIME_SYNC_FAILED;
				}
			}
		}
		updateUI_time();
		updateVisibleSummary();
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
	if (sUpdatingDateEditTexts) {
		return false;
	}

	normalizeDateEditTexts(false, NULL, NULL, NULL);
	normalizeTimeEditTexts(false, NULL, NULL, NULL);
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
