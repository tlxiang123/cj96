#pragma once
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

#include "net/NetManager.h"
#include <pthread.h>

#define WIFIMANAGER			NETMANAGER->getWifiManager()


static std::vector<WifiInfo> sWifiInfos;
static std::map<std::string, std::string> sWifiChangeAps;
static Mutex sLock;

static const uint32_t WIFI_STATUS_BLUE = 0x168BFF;
static const uint32_t WIFI_STATUS_GRAY = 0x526579;

struct WifiEnableRequest {
	WifiManager *manager;
	bool enable;
};

static volatile bool sWifiEnableRequestRunning = false;

static void* wifiEnableWorker(void *arg) {
	WifiEnableRequest *request = static_cast<WifiEnableRequest *>(arg);
	if (request && request->manager) {
		request->manager->enableWifi(request->enable);
	}
	delete request;
	sWifiEnableRequestRunning = false;
	return NULL;
}

static bool isConnectedWifi(const WifiInfo &wifiInfo) {
	if (!WIFIMANAGER->isConnected()) {
		return false;
	}

	const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
	if (connectionInfo == NULL) {
		return false;
	}

	// BSSID is the precise match. Some firmware reports it a little later
	// than the SSID, so use SSID as a fallback during the initial refresh.
	if (!connectionInfo->getBssid().empty()
			&& !wifiInfo.getBssid().empty()
			&& connectionInfo->getBssid() == wifiInfo.getBssid()) {
		return true;
	}

	return !connectionInfo->getSsid().empty()
			&& connectionInfo->getSsid() == wifiInfo.getSsid();
}

static void deduplicateWifiInfos(const std::vector<WifiInfo> &source,
		std::vector<WifiInfo> &result) {
	result.clear();
	std::map<std::string, size_t> ssidIndexes;
	const WifiInfo *connectedInfo = WIFIMANAGER->isConnected()
			? WIFIMANAGER->getConnectionInfo() : NULL;

	for (size_t i = 0; i < source.size(); ++i) {
		const WifiInfo &candidate = source[i];
		const std::string &ssid = candidate.getSsid();
		if (ssid.empty()) {
			continue;
		}

		std::map<std::string, size_t>::iterator it = ssidIndexes.find(ssid);
		if (it == ssidIndexes.end()) {
			ssidIndexes[ssid] = result.size();
			result.push_back(candidate);
			continue;
		}

		WifiInfo &current = result[it->second];
		const bool candidateConnected = connectedInfo
				&& candidate.getBssid() == connectedInfo->getBssid();
		const bool currentConnected = connectedInfo
				&& current.getBssid() == connectedInfo->getBssid();
		if (candidateConnected
				|| (!currentConnected && candidate.getRssi() > current.getRssi())) {
			current = candidate;
		}
	}
}

void removeItemWifiChangeAps(const char *bssid) {
	Mutex::Autolock _l(sLock);
	std::map<std::string, std::string>::iterator it;
	it = sWifiChangeAps.find(bssid);
	if (it != sWifiChangeAps.end()) {
		sWifiChangeAps.erase(it);
	}
}


class MyWifiListener : public WifiManager::IWifiListener {
public:
	virtual void handleWifiEnable(E_WIFI_ENABLE event, int args) {
		LOGD("MyWifiListener handleWifiEnable event: %d\n", event);
		if (!mButtonOnOffPtr) {
			return;
		}

		switch (event) {
		case E_WIFI_ENABLE_ENABLE:
			mButtonOnOffPtr->setInvalid(false);
			mButtonOnOffPtr->setSelected(true);
			break;
		case E_WIFI_ENABLE_DISABLE:
			mButtonOnOffPtr->setInvalid(false);
			mButtonOnOffPtr->setSelected(false);
			break;
		case E_WIFI_ENABLE_DISABLEING:
		case E_WIFI_ENABLE_ENABLEING:
			mButtonOnOffPtr->setInvalid(true);
			mButtonOnOffPtr->setSelected(false);
			break;
		case E_WIFI_ENABLE_UNKNOW:
			mButtonOnOffPtr->setInvalid(false);
			mButtonOnOffPtr->setSelected(false);
			break;
		}
	}

	virtual void handleWifiConnect(E_WIFI_CONNECT event, int args) {
		LOGD("MyWifiListener handleWifiConnect event: %d\n", event);
		if (mListViewWifiInfoPtr) {
			mListViewWifiInfoPtr->refreshListView();
		}
	}

	virtual void handleWifiErrorCode(E_WIFI_ERROR_CODE code) {
		mWindowPasswordErrorPtr->showWnd();
	}

	virtual void handleWifiScanResult(std::vector<WifiInfo>* wifiInfos) {
		if (wifiInfos) {
			Mutex::Autolock _l(sLock);
			deduplicateWifiInfos(*wifiInfos, sWifiInfos);
		}

		mListViewWifiInfoPtr->refreshListView();
	}

	virtual void notifySupplicantStateChange(int networkid,
			const char* ssid, const char* bssid,
			E_SUPPLICATION_STATE newState) {
		switch (newState) {
		case AUTHENTICATING: {
			Mutex::Autolock _l(sLock);
			sWifiChangeAps[bssid] = "认证中...";
			break;
		}
		case ASSOCIATING:
			break;
		case ASSOCIATED:
			break;
		case FOUR_WAY_HANDSHAKE:
			break;
		case GROUP_HANDSHAKE: {
			Mutex::Autolock _l(sLock);
			sWifiChangeAps[bssid] = "正在获取IP";
			break;
		}
		case COMPLETED: {
			Mutex::Autolock _l(sLock);
			sWifiChangeAps.clear();
			break;
		}

		case DISCONNECTED:
		case INTERFACE_DISABLED:
		case INACTIVE:
		case SCANNING:
		case DORMANT:
		case UNINITIALIZED:
		case INVALID:
			removeItemWifiChangeAps(bssid);
			break;
		}

		mListViewWifiInfoPtr->refreshListView();
	}
};

static MyWifiListener sMyWifiListener;


static std::string getEncryptionInfo(const WifiInfo &wi) {
	std::string info;

	const std::string &encryption = wi.getEncryption();

	if (encryption.find("WEP") != std::string::npos) {
		info = "WEP";
	}
	if (encryption.find("WPA") != std::string::npos) {
		info += info.empty() ? "WPA" : " WPA";
	}
	if (encryption.find("WPA2") != std::string::npos) {
		info += info.empty() ? "WPA2" : " WPA2";
	}
	if (encryption.find("WPS") != std::string::npos) {
		info += "(WPS 加密)";
	}
	if (info.empty()) {
		info = "开放";
	}

	return info;
}

/**
 * 注册定时器
 * 在此数组中添加即可
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{0,  500}, // Rebind connection state shortly after the page becomes visible.
};

static void onUI_init() {
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");

	mButtonOnOffPtr->setSelected(WIFIMANAGER->isWifiEnable());

	mTextIPAddrPtr->setText(WIFIMANAGER->getIp());
	mTextMacAddrPtr->setText(WIFIMANAGER->getMacAddr());

	std::vector<WifiInfo> wifiInfos;
	WIFIMANAGER->getWifiScanInfosLock(wifiInfos);
	deduplicateWifiInfos(wifiInfos, sWifiInfos);
	WIFIMANAGER->addWifiListener(&sMyWifiListener);
	// The list adapter is installed before onUI_init(), so refresh once after
	// loading the cached scan data to render the current connection state
	// immediately instead of waiting for a touch event.
	mListViewWifiInfoPtr->refreshListView();
}

static void onUI_quit() {
	WIFIMANAGER->removeWifiListener(&sMyWifiListener);

	sWifiInfos.clear();
	sWifiChangeAps.clear();
}

static void onProtocolDataUpdate(const SProtocolData &data) {
    // 串口数据回调接口
}

static bool onUI_Timer(int id) {
    //Tips:添加定时器响应的代码到这里,但是需要在本文件的 REGISTER_ACTIVITY_TIMER_TAB 数组中 注册
    //id 是定时器设置时候的标签,这里不要写耗时的操作，否则影响UI刷新,ruturn:[true] 继续运行定时器;[false] 停止运行当前定时器
	if (id == 0 && mListViewWifiInfoPtr) {
		mListViewWifiInfoPtr->refreshListView();
	}

    return true;
}

static bool onwifisettingActivityTouchEvent(const MotionEvent &ev) {
    // 返回false触摸事件将继续传递到控件上，返回true表示该触摸事件在此被拦截了，不再传递到控件上
    return false;
}

static int getListItemCount_ListViewWifiInfo(const ZKListView *pListView) {
    //LOGD(" getListItemCount_ ListViewWifiInfo  !!!\n");
	Mutex::Autolock _l(sLock);
	return sWifiInfos.size();
}

static void obtainListItemData_ListViewWifiInfo(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ ListViewWifiInfo  !!!\n");
	ZKListView::ZKListSubItem *pLevelItem = pListItem->findSubItemByID(ID_WIFISETTING_LISTSUBITEM_LEVEL);
	ZKListView::ZKListSubItem *pNameItem = pListItem->findSubItemByID(ID_WIFISETTING_LISTSUBITEM_NAME);
	ZKListView::ZKListSubItem *pSubItem = pListItem->findSubItemByID(ID_WIFISETTING_LISTSUBITEM_SUB);

	Mutex::Autolock _l(sLock);
	const WifiInfo &wi = sWifiInfos.at(index);
	if (wi.getEncryption().length() < 5) {
		pLevelItem->setText(5 + wi.getLevel());
	} else {
		pLevelItem->setText(wi.getLevel());
	}
	pNameItem->setText(wi.getSsid());

	if (isConnectedWifi(wi)) {
		pNameItem->setSelected(true);
		pSubItem->setText("已连接");
		pSubItem->setTextColor(WIFI_STATUS_BLUE);
	} else {
		pNameItem->setSelected(false);
		pSubItem->setTextColor(WIFI_STATUS_GRAY);

		if (sWifiChangeAps.find(wi.getBssid()) != sWifiChangeAps.end()) {
			pSubItem->setText(sWifiChangeAps[wi.getBssid()]);
		} else {
			pSubItem->setText(getEncryptionInfo(wi));
		}
	}
}

static void onListItemClick_ListViewWifiInfo(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ ListViewWifiInfo  !!!\n");
	Mutex::Autolock _l(sLock);
	const WifiInfo &wi = sWifiInfos.at(index);
	if (isConnectedWifi(wi)) {
		mTextConnectSsidPtr->setText(wi.getSsid());
		mTextConnectSecTypePtr->setText(getEncryptionInfo(wi));

		mWindowDisconnectPtr->showWnd();
	} else {
		mTextSsidPtr->setText(wi.getSsid());
		mTextSecTypePtr->setText(getEncryptionInfo(wi));
		mEdittextPwdPtr->setText("");

		mWindowSetPtr->showWnd();
	}
}

static bool onButtonClick_ButtonOnOff(ZKButton *pButton) {
    //LOGD(" ButtonClick ButtonOnOff !!!\n");
	if (sWifiEnableRequestRunning) {
		LOGD("wifi enable request is already running\n");
		return false;
	}

	WifiEnableRequest *request = new WifiEnableRequest;
	request->manager = WIFIMANAGER;
	request->enable = !WIFIMANAGER->isWifiEnable();
	sWifiEnableRequestRunning = true;

	pthread_t worker;
	const int ret = pthread_create(&worker, NULL, wifiEnableWorker, request);
	if (ret != 0) {
		delete request;
		sWifiEnableRequestRunning = false;
		LOGD("wifi enable worker create failed: %d\n", ret);
	}
	else {
		pthread_detach(worker);
	}
    return false;
}

static bool onButtonClick_sys_back(ZKButton *pButton) {
    //LOGD(" ButtonClick sys_back !!!\n");
    return false;
}

static bool onButtonClick_ButtonMenu(ZKButton *pButton) {
    //LOGD(" ButtonClick ButtonMenu !!!\n");
	mTextIPAddrPtr->setText(WIFIMANAGER->getIp());
	mTextMacAddrPtr->setText(WIFIMANAGER->getMacAddr());

	mWindowMenuMorePtr->showWnd();

	return false;
}

static bool onButtonClick_ButtonConnect(ZKButton *pButton) {
    //LOGD(" ButtonClick ButtonConnect !!!\n");
	WIFIMANAGER->connect(mTextSsidPtr->getText(), mEdittextPwdPtr->getText());
	mWindowSetPtr->hideWnd();
    return false;
}

static void onEditTextChanged_EdittextPwd(const std::string &text) {
    //LOGD(" onEditTextChanged_ EdittextPwd %s !!!\n", text.c_str());
}

static bool onButtonClick_ButtonShowPwd(ZKButton *pButton) {
    //LOGD(" ButtonClick ButtonShowPwd !!!\n");
	pButton->setSelected(!pButton->isSelected());
	mEdittextPwdPtr->setPassword(!pButton->isSelected());
    return false;
}

static bool onButtonClick_ButtonDisconnect(ZKButton *pButton) {
    //LOGD(" ButtonClick ButtonDisconnect !!!\n");
	WIFIMANAGER->disconnect();
	mWindowDisconnectPtr->hideWnd();
    return false;
}
