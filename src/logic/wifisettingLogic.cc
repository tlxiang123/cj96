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
#include "net/WifiCtrl.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define WIFIMANAGER			NETMANAGER->getWifiManager()


static std::vector<WifiInfo> sWifiInfos;
static std::map<std::string, std::string> sWifiChangeAps;
static Mutex sLock;

static const uint32_t WIFI_STATUS_BLUE = 0x168BFF;
static const uint32_t WIFI_STATUS_GRAY = 0x526579;
static const uint32_t WIFI_STATUS_GREEN = 0x248A3D;
static const uint32_t WIFI_STATUS_RED = 0xD92D20;

struct WifiEnableRequest {
	WifiManager *manager;
	bool enable;
};

static volatile bool sWifiEnableRequestRunning = false;
static volatile bool sWifiToggleInProgress = false;
static volatile bool sWifiInternetCheckRunning = false;
static int sWifiInternetPendingTip = 0;
static bool sWifiInternetFailurePending = false;
static std::string sWifiInternetFailureSsid;
static std::string sWifiInternetPendingTipSsid;
static std::string sWifiManualConnectingSsid;
static std::string sWifiInternetCheckingSsid;
static std::string sWifiInternetValidatedSsid;
static long long sWifiTipAutoHideDeadlineMs = 0;
static long long sWifiManualConnectingStartMs = 0;
static time_t sWifiToggleStartTime = 0;
static const int WIFI_TOGGLE_TIMEOUT_SECONDS = 90;
static const int WIFI_CONNECT_STAGE_TIMEOUT_MS = 8000;
static const int WIFI_QUALITY_STAGE_TIMEOUT_MS = 8000;
static const char *WIFI_INTERFACE_NAME = "wlan0";
static const char *WIFI_BAD_SSID_PATH = "/mnt/extsd/cj96_wifi_bad_ssid.txt";
static const char *WIFI_MANUAL_RETRY_PATH = "/mnt/extsd/cj96_wifi_manual_retry_ssid.txt";
static const char *WIFI_INTERNET_FAILURE_TEXT =
		"\xE6\x97\xA0\xE6\xB3\x95\xE8\xBF\x9E\xE6\x8E\xA5\xE5\xA4\x96\xE7\xBD\x91";
static const char *WIFI_PASSWORD_ERROR_TEXT =
		"\xE5\xAF\x86\xE7\xA0\x81\xE9\x94\x99\xE8\xAF\xAF";
static const char *WIFI_CONNECT_FAILURE_TEXT =
		"\xE8\xBF\x9E\xE6\x8E\xA5\xE5\xA4\xB1\xE8\xB4\xA5";
static const char *WIFI_QUALITY_CHECK_TEXT =
		"\xE6\xAD\xA3\xE5\x9C\xA8\xE6\xA3\x80\xE6\x9F\xA5\xE7\xBD\x91\xE7\xBB\x9C\xE8\xB4\xA8\xE9\x87\x8F";

static int collectWifiDnsServers(char servers[][16], int maxCount);
static void showPendingWifiInternetStatusIfNeeded();

static bool isWifiEnableStatusBusy(E_WIFI_ENABLE status) {
	return (status == E_WIFI_ENABLE_ENABLEING)
			|| (status == E_WIFI_ENABLE_DISABLEING);
}

static bool isWifiEnableStatusTerminal(E_WIFI_ENABLE status) {
	return (status == E_WIFI_ENABLE_ENABLE)
			|| (status == E_WIFI_ENABLE_DISABLE)
			|| (status == E_WIFI_ENABLE_UNKNOW);
}

static bool isWifiToggleBusyNow() {
	E_WIFI_ENABLE status = WIFIMANAGER->getEnableStatus();
	return sWifiEnableRequestRunning
			|| sWifiToggleInProgress
			|| isWifiEnableStatusBusy(status);
}

static void clearWifiToggleBusy() {
	sWifiToggleInProgress = false;
	sWifiToggleStartTime = 0;
}

static void* wifiEnableWorker(void *arg) {
	WifiEnableRequest *request = static_cast<WifiEnableRequest *>(arg);
	if (request && request->manager) {
		request->manager->enableWifi(request->enable);
	}
	delete request;
	sWifiEnableRequestRunning = false;
	return NULL;
}

static long long getWifiNowMs() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return static_cast<long long>(tv.tv_sec) * 1000LL
			+ static_cast<long long>(tv.tv_usec) / 1000LL;
}

static void hideWifiTipWindowOnly() {
	if (mWindowPasswordErrorPtr) {
		mWindowPasswordErrorPtr->hideWnd();
	}
	sWifiTipAutoHideDeadlineMs = 0;
}

static void queueWifiConnectFailure(const std::string &ssid,
		const char *statusText) {
	Mutex::Autolock _l(sLock);
	sWifiInternetFailureSsid = ssid;
	sWifiInternetFailurePending = true;
	sWifiInternetPendingTipSsid = ssid;
	sWifiInternetPendingTip = 4;
	sWifiManualConnectingSsid.clear();
	sWifiManualConnectingStartMs = 0;
	sWifiInternetCheckingSsid.clear();
	if (sWifiInternetValidatedSsid == ssid) {
		sWifiInternetValidatedSsid.clear();
	}
	(void)statusText;
}

static void showWifiTipWindow(const char *text, uint32_t color,
		int autoHideMs) {
	if (mTextview27Ptr) {
		mTextview27Ptr->setText(text);
		mTextview27Ptr->setTextColor(color);
	}
	sWifiTipAutoHideDeadlineMs = autoHideMs > 0
			? getWifiNowMs() + autoHideMs : 0;
	if (mWindowPasswordErrorPtr) {
		mWindowPasswordErrorPtr->showWnd();
	}
}

static void showWifiSsidStatusTip(const std::string &ssid, const char *status,
		uint32_t color, int autoHideMs) {
	char message[160];
	snprintf(message, sizeof(message), "%s\n%s",
			ssid.empty() ? "WiFi" : ssid.c_str(),
			status ? status : "");
	showWifiTipWindow(message, color, autoHideMs);
}

static void updateWifiTipAutoHide() {
	if (sWifiTipAutoHideDeadlineMs <= 0) {
		return;
	}
	if (getWifiNowMs() >= sWifiTipAutoHideDeadlineMs) {
		hideWifiTipWindowOnly();
	}
}

static std::string normalizeSsidText(const char *ssid) {
	std::string text = ssid ? ssid : "";
	if (text.length() >= 2 && text[0] == '"'
			&& text[text.length() - 1] == '"') {
		text = text.substr(1, text.length() - 2);
	}
	return text;
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

static bool bindSocketToInterfaceIpv4(int fd, const char *interfaceName) {
	if (!interfaceName || interfaceName[0] == '\0') {
		return true;
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

static int appendDnsName(unsigned char *packet, int offset, int maxLength,
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

static bool requestWifiDnsProbeOnInterface(const char *interfaceName,
		const char *serverIp, const char *domain, struct in_addr *resolvedIp) {
	const int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		return false;
	}

	bool success = false;
	if (!bindSocketToInterfaceIpv4(fd, interfaceName)) {
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
		int length = appendDnsName(packet, 12, sizeof(packet), domain);
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
							}
							else {
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

static bool resolveWifiDomainOnInterface(const char *interfaceName,
		const char *domain, struct in_addr *resolvedIp) {
	char dnsProbeIps[4][16];
	const int count = collectWifiDnsServers(dnsProbeIps,
			sizeof(dnsProbeIps) / sizeof(dnsProbeIps[0]));
	for (int i = 0; i < count; ++i) {
		if (requestWifiDnsProbeOnInterface(interfaceName,
				dnsProbeIps[i], domain, resolvedIp)) {
			return true;
		}
	}
	return false;
}

static bool canWifiResolvePublicDomain() {
	static const char *domains[] = {
		"www.microsoft.com",
		"www.apple.com",
		"www.baidu.com",
		"www.qq.com",
	};
	const int count = sizeof(domains) / sizeof(domains[0]);
	for (int i = 0; i < count; ++i) {
		if (resolveWifiDomainOnInterface(WIFI_INTERFACE_NAME, domains[i], NULL)) {
			return true;
		}
	}
	return false;
}

static bool requestWifiHttpProbeOnInterface(const char *interfaceName,
		const char *host, const char *path, const char *expectedText) {
	struct in_addr remoteIp;
	memset(&remoteIp, 0, sizeof(remoteIp));
	if (!resolveWifiDomainOnInterface(interfaceName, host, &remoteIp)
			|| remoteIp.s_addr == 0) {
		return false;
	}

	const int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		return false;
	}

	bool success = false;
	if (!bindSocketToInterfaceIpv4(fd, interfaceName)) {
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

static bool canWifiReachInternet() {
	if (!canWifiResolvePublicDomain()) {
		return false;
	}
	return requestWifiHttpProbeOnInterface(WIFI_INTERFACE_NAME,
			"www.msftconnecttest.com",
			"/connecttest.txt",
			"Microsoft Connect Test")
			|| requestWifiHttpProbeOnInterface(WIFI_INTERFACE_NAME,
					"captive.apple.com",
					"/hotspot-detect.html",
					"Success");
}

static void trimWifiLineEnd(char *text) {
	if (!text) {
		return;
	}
	text[strcspn(text, "\r\n")] = '\0';
}

static bool readWifiOneLineFile(const char *path, std::string &value) {
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
	trimWifiLineEnd(line);
	value = line;
	return !value.empty();
}

static void writeWifiOneLineFile(const char *path, const std::string &value) {
	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		return;
	}
	fprintf(fp, "%s\n", value.c_str());
	fclose(fp);
}

static void addWifiDnsServer(char servers[][16], int &count, int maxCount,
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

static int collectWifiDnsServers(char servers[][16], int maxCount) {
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
					&& strcmp(iface, WIFI_INTERFACE_NAME) == 0
					&& destination == 0 && gateway != 0) {
				struct in_addr address;
				address.s_addr = static_cast<in_addr_t>(gateway);
				char text[16];
				if (inet_ntop(AF_INET, &address, text, sizeof(text))) {
					addWifiDnsServer(servers, count, maxCount, text);
				}
				break;
			}
		}
		fclose(fp);
	}
	addWifiDnsServer(servers, count, maxCount, "223.5.5.5");
	addWifiDnsServer(servers, count, maxCount, "114.114.114.114");
	addWifiDnsServer(servers, count, maxCount, "1.1.1.1");
	return count;
}

static void removeWifiFile(const char *path) {
	unlink(path);
}

static bool isBadWifiSsid(const std::string &ssid) {
	std::string badSsid;
	return !ssid.empty()
			&& readWifiOneLineFile(WIFI_BAD_SSID_PATH, badSsid)
			&& badSsid == ssid;
}

static bool isManualRetryWifiSsid(const std::string &ssid) {
	std::string retrySsid;
	return !ssid.empty()
			&& readWifiOneLineFile(WIFI_MANUAL_RETRY_PATH, retrySsid)
			&& retrySsid == ssid;
}

static void markBadWifiSsid(const std::string &ssid) {
	if (!ssid.empty()) {
		writeWifiOneLineFile(WIFI_BAD_SSID_PATH, ssid);
	}
	removeWifiFile(WIFI_MANUAL_RETRY_PATH);
}

static void markManualRetryWifiSsid(const std::string &ssid) {
	if (!ssid.empty()) {
		writeWifiOneLineFile(WIFI_MANUAL_RETRY_PATH, ssid);
	}
}

static void clearBadWifiSsid(const std::string &ssid) {
	std::string badSsid;
	if (readWifiOneLineFile(WIFI_BAD_SSID_PATH, badSsid)
			&& badSsid == ssid) {
		removeWifiFile(WIFI_BAD_SSID_PATH);
	}
	std::string retrySsid;
	if (readWifiOneLineFile(WIFI_MANUAL_RETRY_PATH, retrySsid)
			&& retrySsid == ssid) {
		removeWifiFile(WIFI_MANUAL_RETRY_PATH);
	}
}

static void forgetWifiNetworkIfKnown(int networkId) {
	if (networkId < 0) {
		return;
	}
	WifiCtrl ctrl(WIFI_INTERFACE_NAME);
	if (!ctrl.connectToSupplicant()) {
		return;
	}
	ctrl.removeNetwork(networkId);
	ctrl.saveConfig();
	ctrl.closeSupplicantConnection();
}

static void queueWifiInternetFailure(const std::string &ssid) {
	Mutex::Autolock _l(sLock);
	sWifiInternetFailureSsid = ssid;
	sWifiInternetFailurePending = true;
	sWifiInternetPendingTipSsid = ssid;
	sWifiInternetPendingTip = 3;
	sWifiInternetCheckingSsid.clear();
	if (sWifiManualConnectingSsid == ssid) {
		sWifiManualConnectingSsid.clear();
		sWifiManualConnectingStartMs = 0;
	}
	if (sWifiInternetValidatedSsid == ssid) {
		sWifiInternetValidatedSsid.clear();
	}
}

static void markWifiInternetValidationSuccess(const std::string &ssid) {
	Mutex::Autolock _l(sLock);
	sWifiInternetValidatedSsid = ssid;
	sWifiInternetPendingTipSsid = ssid;
	sWifiInternetPendingTip = 2;
	if (sWifiManualConnectingSsid == ssid) {
		sWifiManualConnectingSsid.clear();
		sWifiManualConnectingStartMs = 0;
	}
	if (sWifiInternetCheckingSsid == ssid) {
		sWifiInternetCheckingSsid.clear();
	}
}

static void queueWifiInternetChecking(const std::string &ssid) {
	Mutex::Autolock _l(sLock);
	if (sWifiManualConnectingSsid == ssid) {
		sWifiManualConnectingSsid.clear();
		sWifiManualConnectingStartMs = 0;
	}
	sWifiInternetCheckingSsid = ssid;
	sWifiInternetPendingTipSsid = ssid;
	sWifiInternetPendingTip = 1;
}

static void clearWifiInternetChecking(const std::string &ssid) {
	Mutex::Autolock _l(sLock);
	if (ssid.empty() || sWifiInternetCheckingSsid == ssid) {
		sWifiInternetCheckingSsid.clear();
	}
}

struct WifiInternetCheckRequest {
	std::string ssid;
	int networkId;
};

static void* wifiInternetCheckWorker(void *arg) {
	WifiInternetCheckRequest *request =
			static_cast<WifiInternetCheckRequest *>(arg);
	const std::string ssid = request ? request->ssid : "";
	const int networkId = request ? request->networkId : -1;
	delete request;

	bool stillSameWifi = false;
	for (int retry = 0; retry < 10 && !stillSameWifi; ++retry) {
		usleep(200 * 1000);
		if (WIFIMANAGER->isConnected()) {
			const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
			if (connectionInfo) {
				const std::string connectedSsid =
						normalizeSsidText(connectionInfo->getSsid().c_str());
				stillSameWifi = !ssid.empty() && connectedSsid == ssid;
			}
		}
	}

	if (stillSameWifi) {
		const long long startMs = getWifiNowMs();
		bool reachable = false;
		while ((getWifiNowMs() - startMs) <= WIFI_QUALITY_STAGE_TIMEOUT_MS) {
			if (canWifiReachInternet()) {
				reachable = true;
				break;
			}
			usleep(700 * 1000);
			if (!WIFIMANAGER->isConnected()) {
				break;
			}
			const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
			const std::string connectedSsid = connectionInfo
					? normalizeSsidText(connectionInfo->getSsid().c_str()) : "";
			if (connectedSsid != ssid) {
				break;
			}
		}
		if (reachable) {
			markWifiInternetValidationSuccess(ssid);
			clearBadWifiSsid(ssid);
		}
		else {
			WIFIMANAGER->disconnect();
			forgetWifiNetworkIfKnown(networkId);
			markBadWifiSsid(ssid);
			queueWifiInternetFailure(ssid);
		}
	}
	else {
		clearWifiInternetChecking(ssid);
	}

	sWifiInternetCheckRunning = false;
	return NULL;
}

static void startWifiInternetCheck(const char *ssid, int networkId) {
	if (sWifiInternetCheckRunning) {
		return;
	}

	std::string checkSsid = normalizeSsidText(ssid);
	if (WIFIMANAGER->isConnected()) {
		const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
		if (connectionInfo && !connectionInfo->getSsid().empty()) {
			checkSsid = normalizeSsidText(connectionInfo->getSsid().c_str());
		}
	}
	if (checkSsid.empty()) {
		return;
	}
	if (isBadWifiSsid(checkSsid) && !isManualRetryWifiSsid(checkSsid)) {
		WIFIMANAGER->disconnect();
		queueWifiInternetFailure(checkSsid);
		return;
	}

	WifiInternetCheckRequest *request = new WifiInternetCheckRequest;
	request->ssid = checkSsid;
	request->networkId = networkId;
	{
		Mutex::Autolock _l(sLock);
		sWifiInternetCheckingSsid = checkSsid;
		if (sWifiInternetValidatedSsid != checkSsid) {
			sWifiInternetValidatedSsid.clear();
		}
	}
	queueWifiInternetChecking(checkSsid);
	sWifiInternetCheckRunning = true;
	pthread_t worker;
	if (pthread_create(&worker, NULL, wifiInternetCheckWorker, request) != 0) {
		delete request;
		Mutex::Autolock _l(sLock);
		if (sWifiInternetCheckingSsid == checkSsid) {
			sWifiInternetCheckingSsid.clear();
		}
		sWifiInternetCheckRunning = false;
	}
	else {
		pthread_detach(worker);
	}
}

static void checkCurrentWifiInternetIfNeeded() {
	if (!WIFIMANAGER->isConnected()) {
		return;
	}

	const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
	if (!connectionInfo || connectionInfo->getSsid().empty()) {
		return;
	}

	const std::string ssid = normalizeSsidText(connectionInfo->getSsid().c_str());
	if (ssid.empty()) {
		return;
	}

	{
		Mutex::Autolock _l(sLock);
		if (sWifiInternetCheckRunning
				|| sWifiInternetCheckingSsid == ssid
				|| sWifiInternetValidatedSsid == ssid) {
			return;
		}
	}

	startWifiInternetCheck(ssid.c_str(), -1);
}

static void updateWifiManualConnectProgress() {
	std::string pendingSsid;
	long long startMs = 0;
	{
		Mutex::Autolock _l(sLock);
		pendingSsid = sWifiManualConnectingSsid;
		startMs = sWifiManualConnectingStartMs;
	}
	if (pendingSsid.empty()) {
		return;
	}

	if (WIFIMANAGER->isConnected()) {
		const WifiInfo *connectionInfo = WIFIMANAGER->getConnectionInfo();
		const std::string connectedSsid = connectionInfo
				? normalizeSsidText(connectionInfo->getSsid().c_str()) : "";
		if (connectedSsid == pendingSsid) {
			startWifiInternetCheck(pendingSsid.c_str(), -1);
			showPendingWifiInternetStatusIfNeeded();
			return;
		}
	}

	if (startMs > 0 && (getWifiNowMs() - startMs) > WIFI_CONNECT_STAGE_TIMEOUT_MS) {
		queueWifiConnectFailure(pendingSsid, WIFI_CONNECT_FAILURE_TEXT);
		showPendingWifiInternetStatusIfNeeded();
	}
}

static void showPendingWifiInternetStatusIfNeeded() {
	std::string ssid;
	int tipState = 0;
	{
		Mutex::Autolock _l(sLock);
		if (sWifiInternetPendingTip == 0 && !sWifiInternetFailurePending) {
			return;
		}
		tipState = sWifiInternetPendingTip;
		ssid = sWifiInternetPendingTipSsid;
		if (tipState == 0 && sWifiInternetFailurePending) {
			tipState = 3;
			ssid = sWifiInternetFailureSsid;
		}
		sWifiInternetPendingTip = 0;
		sWifiInternetPendingTipSsid.clear();
		sWifiInternetFailurePending = false;
		sWifiInternetFailureSsid.clear();
	}

	if (tipState == 1) {
		showWifiSsidStatusTip(ssid, WIFI_QUALITY_CHECK_TEXT, WIFI_STATUS_BLUE, 0);
	}
	else if (tipState == 2) {
		showWifiSsidStatusTip(ssid, "已连接", WIFI_STATUS_GREEN, 800);
	}
	else if (tipState == 3) {
		showWifiSsidStatusTip(ssid, WIFI_INTERNET_FAILURE_TEXT, WIFI_STATUS_RED, 0);
	}
	else if (tipState == 4) {
		showWifiSsidStatusTip(ssid, WIFI_CONNECT_FAILURE_TEXT, WIFI_STATUS_RED, 0);
	}

	if (mListViewWifiInfoPtr) {
		mListViewWifiInfoPtr->refreshListView();
	}
}

static bool isConnectedWifi(const WifiInfo &wifiInfo) {
	if (!WIFIMANAGER->isConnected()) {
		return false;
	}
	struct in_addr wifiAddress;
	if (!getInterfaceIpv4Address(WIFI_INTERFACE_NAME, &wifiAddress)) {
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

static void refreshWifiInfosFromManager() {
	std::vector<WifiInfo> wifiInfos;
	WIFIMANAGER->getWifiScanInfosLock(wifiInfos);

	Mutex::Autolock _l(sLock);
	deduplicateWifiInfos(wifiInfos, sWifiInfos);
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
		if (isWifiEnableStatusTerminal(event)) {
			clearWifiToggleBusy();
		}
		else if (isWifiEnableStatusBusy(event)) {
			sWifiToggleInProgress = true;
			if (sWifiToggleStartTime == 0) {
				sWifiToggleStartTime = time(NULL);
			}
		}

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
		updateWifiManualConnectProgress();
		checkCurrentWifiInternetIfNeeded();
		if (mListViewWifiInfoPtr) {
			mListViewWifiInfoPtr->refreshListView();
		}
	}

	virtual void handleWifiErrorCode(E_WIFI_ERROR_CODE code) {
		std::string ssid;
		{
			Mutex::Autolock _l(sLock);
			ssid = sWifiManualConnectingSsid;
			sWifiManualConnectingSsid.clear();
			sWifiManualConnectingStartMs = 0;
			sWifiInternetCheckingSsid.clear();
		}
		if (ssid.empty() && mTextSsidPtr) {
			ssid = normalizeSsidText(mTextSsidPtr->getText().c_str());
		}
		showWifiSsidStatusTip(ssid, WIFI_PASSWORD_ERROR_TEXT, WIFI_STATUS_RED, 0);
		if (mListViewWifiInfoPtr) {
			mListViewWifiInfoPtr->refreshListView();
		}
	}

	virtual void handleWifiScanResult(std::vector<WifiInfo>* wifiInfos) {
		if (wifiInfos) {
			Mutex::Autolock _l(sLock);
			deduplicateWifiInfos(*wifiInfos, sWifiInfos);
		}
		else {
			refreshWifiInfosFromManager();
		}

		if (mListViewWifiInfoPtr) {
			mListViewWifiInfoPtr->refreshListView();
		}
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
			{
				Mutex::Autolock _l(sLock);
				sWifiChangeAps[bssid] = "正在获取IP";
			}
			queueWifiInternetChecking(normalizeSsidText(ssid));
			break;
		}
		case COMPLETED: {
			{
				Mutex::Autolock _l(sLock);
				sWifiChangeAps.clear();
			}
			startWifiInternetCheck(ssid, networkid);
			break;
		}

		case DISCONNECTED:
		case INTERFACE_DISABLED:
		case INACTIVE:
		case SCANNING:
		case DORMANT:
		case UNINITIALIZED:
		case INVALID:
		{
			std::string failedConnectingSsid;
			Mutex::Autolock _l(sLock);
			const std::string disconnectedSsid = normalizeSsidText(ssid);
			if (sWifiInternetCheckingSsid == disconnectedSsid) {
				sWifiInternetCheckingSsid.clear();
			}
			if (sWifiInternetValidatedSsid == disconnectedSsid) {
				sWifiInternetValidatedSsid.clear();
			}
			if (sWifiManualConnectingSsid == disconnectedSsid) {
				failedConnectingSsid = sWifiManualConnectingSsid;
				sWifiManualConnectingSsid.clear();
				sWifiManualConnectingStartMs = 0;
			}
			if (!failedConnectingSsid.empty()) {
				sWifiInternetFailureSsid = failedConnectingSsid;
				sWifiInternetFailurePending = true;
				sWifiInternetPendingTipSsid = failedConnectingSsid;
				sWifiInternetPendingTip = 3;
			}
		}
			removeItemWifiChangeAps(bssid);
			break;
		}

		if (mListViewWifiInfoPtr) {
			mListViewWifiInfoPtr->refreshListView();
		}
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
	{0,  3000}, // Keep the Wi-Fi list fresh without competing with password input.
	{1,  100},
};

static bool isWifiPasswordWindowVisible() {
	return mWindowSetPtr && mWindowSetPtr->isWndShow();
}

static void onUI_init() {
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");

	E_WIFI_ENABLE status = WIFIMANAGER->getEnableStatus();
	mButtonOnOffPtr->setInvalid(isWifiEnableStatusBusy(status));
	mButtonOnOffPtr->setSelected(status == E_WIFI_ENABLE_ENABLE);

	mTextIPAddrPtr->setText(WIFIMANAGER->getIp());
	mTextMacAddrPtr->setText(WIFIMANAGER->getMacAddr());

	if (WIFIMANAGER->isWifiEnable()) {
		refreshWifiInfosFromManager();
	}
	WIFIMANAGER->addWifiListener(&sMyWifiListener);
	checkCurrentWifiInternetIfNeeded();
	// The list adapter is installed before onUI_init(), so refresh once after
	// loading the cached scan data to render the current connection state
	// immediately instead of waiting for a touch event.
	if (mListViewWifiInfoPtr) {
		mListViewWifiInfoPtr->refreshListView();
	}
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
	if (id == 0) {
		showPendingWifiInternetStatusIfNeeded();
		checkCurrentWifiInternetIfNeeded();
		if (isWifiPasswordWindowVisible()) {
			return true;
		}
		if (WIFIMANAGER->isWifiEnable()) {
			refreshWifiInfosFromManager();
		}
		if (mListViewWifiInfoPtr) {
			mListViewWifiInfoPtr->refreshListView();
		}
		if (mTextIPAddrPtr) {
			mTextIPAddrPtr->setText(WIFIMANAGER->getIp());
		}
		if (mTextMacAddrPtr) {
			mTextMacAddrPtr->setText(WIFIMANAGER->getMacAddr());
		}
	}
	if (id == 1) {
		showPendingWifiInternetStatusIfNeeded();
		updateWifiManualConnectProgress();
		updateWifiTipAutoHide();
	}
	if (sWifiToggleInProgress && sWifiToggleStartTime > 0) {
		time_t now = time(NULL);
		if ((now - sWifiToggleStartTime) > WIFI_TOGGLE_TIMEOUT_SECONDS) {
			LOGD("wifi enable request timeout, release ui busy state\n");
			clearWifiToggleBusy();
			if (mButtonOnOffPtr) {
				mButtonOnOffPtr->setInvalid(false);
				mButtonOnOffPtr->setSelected(WIFIMANAGER->isWifiEnable());
			}
		}
	}

    return true;
}

static bool onwifisettingActivityTouchEvent(const MotionEvent &ev) {
    // 返回false触摸事件将继续传递到控件上，返回true表示该触摸事件在此被拦截了，不再传递到控件上
	if (mWindowPasswordErrorPtr && mWindowPasswordErrorPtr->isWndShow()) {
		hideWifiTipWindowOnly();
		return true;
	}
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
	const std::string ssid = normalizeSsidText(wi.getSsid().c_str());

	if (!ssid.empty() && ssid == sWifiManualConnectingSsid) {
		pNameItem->setSelected(true);
		pSubItem->setText("正在连接");
		pSubItem->setTextColor(WIFI_STATUS_GRAY);
	}
	else if (isConnectedWifi(wi)) {
		pNameItem->setSelected(true);
		if (!ssid.empty() && sWifiInternetValidatedSsid == ssid) {
			pSubItem->setText("已连接");
			pSubItem->setTextColor(WIFI_STATUS_BLUE);
		}
		else if (!ssid.empty() && ssid == sWifiInternetCheckingSsid) {
			pSubItem->setText("正在检测网络");
			pSubItem->setTextColor(WIFI_STATUS_GRAY);
		}
		else {
			pSubItem->setText("正在连接");
			pSubItem->setTextColor(WIFI_STATUS_GRAY);
		}
	} else {
		pNameItem->setSelected(false);
		pSubItem->setTextColor(WIFI_STATUS_GRAY);

		if (!ssid.empty() && ssid == sWifiInternetCheckingSsid) {
			pSubItem->setText("正在连接网络");
		} else if (isBadWifiSsid(ssid)) {
			pSubItem->setText(WIFI_INTERNET_FAILURE_TEXT);
			pSubItem->setTextColor(WIFI_STATUS_RED);
		} else if (sWifiChangeAps.find(wi.getBssid()) != sWifiChangeAps.end()) {
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
	E_WIFI_ENABLE status = WIFIMANAGER->getEnableStatus();
	if (sWifiEnableRequestRunning || sWifiToggleInProgress
			|| isWifiEnableStatusBusy(status)) {
		LOGD("wifi enable request is already running, status: %d, thread: %d, busy: %d\n",
				status, sWifiEnableRequestRunning, sWifiToggleInProgress);
		if (pButton) {
			pButton->setInvalid(true);
		}
		return false;
	}
	if (!WIFIMANAGER->isSupported()) {
		LOGD("wifi is not supported, ignore enable request\n");
		if (pButton) {
			pButton->setInvalid(false);
			pButton->setSelected(false);
		}
		return false;
	}

	WifiEnableRequest *request = new WifiEnableRequest;
	request->manager = WIFIMANAGER;
	request->enable = (status == E_WIFI_ENABLE_ENABLE)
			? false : !WIFIMANAGER->isWifiEnable();
	sWifiEnableRequestRunning = true;
	sWifiToggleInProgress = true;
	sWifiToggleStartTime = time(NULL);
	if (pButton) {
		pButton->setInvalid(true);
	}

	pthread_t worker;
	const int ret = pthread_create(&worker, NULL, wifiEnableWorker, request);
	if (ret != 0) {
		delete request;
		sWifiEnableRequestRunning = false;
		clearWifiToggleBusy();
		LOGD("wifi enable worker create failed: %d\n", ret);
	}
	else {
		pthread_detach(worker);
	}
    return false;
}

static bool onButtonClick_sys_back(ZKButton *pButton) {
    LOGD(" ButtonClick sys_back, wifi_busy=%d, status=%d !!!\n",
			isWifiToggleBusyNow(), WIFIMANAGER->getEnableStatus());
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
	const std::string ssid = normalizeSsidText(mTextSsidPtr->getText().c_str());
	markManualRetryWifiSsid(ssid);
	{
		Mutex::Autolock _l(sLock);
		sWifiManualConnectingSsid = ssid;
		sWifiManualConnectingStartMs = getWifiNowMs();
		sWifiInternetCheckingSsid.clear();
		if (sWifiInternetValidatedSsid == ssid) {
			sWifiInternetValidatedSsid.clear();
		}
	}
	showWifiSsidStatusTip(ssid, "正在连接", WIFI_STATUS_BLUE, 0);
	WIFIMANAGER->connect(mTextSsidPtr->getText(), mEdittextPwdPtr->getText());
	mWindowSetPtr->hideWnd();
	if (mListViewWifiInfoPtr) {
		mListViewWifiInfoPtr->refreshListView();
	}
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
