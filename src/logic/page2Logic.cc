#include "DeviceDataStore.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/time.h>
#include <vector>

static const char* W2_DEVICE_TYPE_VALVE = "电磁阀";
static const char* W2_DEVICE_TYPE_SENSOR = "传感器";
static int sW2EditingIndex = -1;
static bool sW2AddingDevice = false;
static int sW2CurrentAddress = 0;
static bool sW2SetWindowOpen = false;
static bool sW2TipWindowVisible = false;
static long long sW2TipShownAtMs = 0;
static bool sW2ActionTipWindowVisible = false;
static long long sW2ActionTipShownAtMs = 0;
static bool sGroupRenameWindowVisible = false;
static bool sIrrCapacityWindowVisible = false;
static bool sClearIrrWindowVisible = false;
static int sW2ChoiceDialogMode = 0;
static bool sGroupBindAllGroups = false;
static int sIrrCapacityPending = 2;
static int sIrrCapacityLast = 2;
static const int DEFAULT_IRR_GROUP_VALVE_LIMIT = 3;
static int sIrrGroupValveLimit = DEFAULT_IRR_GROUP_VALVE_LIMIT;
static bool sW2AutoAssignPreviewPending = false;
static std::vector<SDATA> sW2AutoAssignDeviceSnapshot;
static std::vector<int> sW2AutoAssignGroupSnapshot;
static bool sW2AutoAssignOriginalAddingDevice = false;
static int sW2AutoAssignOriginalEditingIndex = -1;
static int sW2AutoAssignOriginalSelectedGroupNo = -1;
static int sW2AutoAssignOriginalValveLimit = DEFAULT_IRR_GROUP_VALVE_LIMIT;
static bool sW2TipCheckedThisVisit = false;
static int sW2SelectedTypeIndex = 0;
static int sSelectedIrrGroupNo = -1;
static const int DEFAULT_PUMP_ADVANCE_SECONDS = 5;
static const int MAX_PUMP_ADVANCE_SECONDS = 999;
static int sIrrGroupPumpAdvanceSeconds[128] = {0};
static bool sIrrGroupPumpAdvanceSecondsSet[128] = {false};
static ZKEditText* sPumpAdvanceSecEditTextPtr = NULL;
static std::vector<int> createInitialIrrGroupNumbers() {
    std::vector<int> groups;
    groups.push_back(1);
    groups.push_back(2);
    groups.push_back(3);
    groups.push_back(4);
    return groups;
}
static std::vector<int> sIrrGroupNumbers = createInitialIrrGroupNumbers();
static bool sIrrEmptyItemLayoutCaptured = false;
static LayoutPosition sIrrNumSubItemPosition;
static LayoutPosition sIrrArrSubItemPosition;
static std::vector<int> sSelectedPumpDeviceIndexes;
static std::vector<int> sSelectedSensorDeviceIndexes;

static void setListSubItemText(ZKListView::ZKListItem *pListItem, int id, const char* text);
static void setListSubItemAlignment(ZKListView::ZKListItem *pListItem, int id, ZKTextView::EAlignH h, ZKTextView::EAlignV v);
static void setListSubItemVisible(ZKListView::ZKListItem *pListItem, int id, bool visible);
static void setListSubItemPosition(ZKListView::ZKListItem *pListItem, int id, const LayoutPosition &position);
static bool isValidIrrGroupNo(int groupNo);
static bool isIrrGroupEmptyRow(int index);
static void setIrrGroupSubItemTexts(ZKListView::ZKListItem *pListItem, int index, int numSubItemId, int nameSubItemId);
static void hideAllPageWindows();
static void hideGroupBindWindowOnly();
static void hideW2SetWindowOnly();
static void hideW2TipWindowOnly();
static bool hideW2TipWindowIfVisible();
static bool handlePage2BeforeMainPageSwitch(int targetPageIndex);
static void closeW2SetWindow();
static void openW2SetWindow(int index);
static const char* getIrrGroupName(int groupNo);
static void buildIrrGroupDisplayText(int groupNo, char* text, size_t size);
static void closeGroupBindWindow();
static void updateGroupBindSelectionEditTexts();
static void updateW2AddressDisplay(int address);
static const char* getW2SelectedDeviceType();
static bool isPumpDevice(const SDATA* data);
static bool isGroupBindPumpDevice(const SDATA* data);
static bool isSensorBindDevice(const SDATA* data);
static bool isSensorTextDevice(const SDATA* data);
static void refreshRunTimeListView();
static bool isW2PreviewValveForGroup(int groupNo);
static int getW2PreviewAddress();
static int getW2DefaultAddress();

static void setW2TipText(const char* text);
static bool showW2TipText(const char* text);
static bool collectUngroupedValveAddresses(char* text, size_t size);
static void openIrrCapacityWindow();
static void selectIrrCapacity(int capacity);
static void closeIrrCapacityWindow(bool save);
static void autoAssignUngroupedValves(int capacity);
static int getIrrGroupValveCount(int groupNo);
static bool canAssignCurrentW2ValveToIrrGroup(int groupNo, bool showTip);
static bool sPage2Active = false;
static bool sPage2CachedDiscoveryRunning = false;
static bool sPage2DiscoveryTipActive = false;
static long long sPage2DiscoveryStartedAtMs = 0;
static const int PAGE2_DISCOVERY_COUNTDOWN_SECONDS = 30;
static bool sW2AddDeviceWindowVisible = false;
static bool sW2AddDeviceSensor = false;
static int sW2AddDeviceAddress = CUSTOM_DEVICE_START_ID;
static const int W2_ADD_DEVICE_ADDRESS_MIN = CUSTOM_DEVICE_START_ID;
static const int W2_ADD_DEVICE_ADDRESS_MAX = 255;

static long long getW2CurrentTimeMs() {
    struct timeval value;
    gettimeofday(&value, NULL);
    return static_cast<long long>(value.tv_sec) * 1000LL
            + static_cast<long long>(value.tv_usec) / 1000LL;
}

static bool isPage2DeviceDiscoveryTipActive() {
    return sPage2DiscoveryTipActive;
}

static void startPage2DeviceDiscoveryTip() {
    sPage2DiscoveryTipActive = true;
    sPage2DiscoveryStartedAtMs = getW2CurrentTimeMs();
    char text[96] = {0};
    snprintf(text, sizeof(text), "同步中，请等待\n剩余时间：%d 秒",
             PAGE2_DISCOVERY_COUNTDOWN_SECONDS);
    (void)showW2TipText(text);
    LOGD("[Page2DeviceSync] countdown started\n");
}

static void updatePage2DeviceDiscoveryCountdown() {
    if (!sPage2DiscoveryTipActive) {
        return;
    }
    if (!isWindow5DeviceDiscoveryRunning()) {
        sPage2DiscoveryTipActive = false;
        sPage2DiscoveryStartedAtMs = 0;
        LOGD("[Page2DeviceSync] countdown finished\n");
        return;
    }

    const long long elapsedMs = getW2CurrentTimeMs() - sPage2DiscoveryStartedAtMs;
    int remaining = PAGE2_DISCOVERY_COUNTDOWN_SECONDS
            - static_cast<int>(elapsedMs / 1000LL);
    if (remaining < 0) {
        remaining = 0;
    }
    char text[96] = {0};
    snprintf(text, sizeof(text), "同步中，请等待\n剩余时间：%d 秒", remaining);
    setW2TipText(text);
}

static bool hasBoundValveInIrrGroup(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return false;
    }

    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data
                && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
                && DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            return true;
        }
    }
    return false;
}

static int resolveSelectedIrrGroupNo() {
    if (isValidIrrGroupNo(sSelectedIrrGroupNo)
            && (hasBoundValveInIrrGroup(sSelectedIrrGroupNo)
                    || isW2PreviewValveForGroup(sSelectedIrrGroupNo))) {
        return sSelectedIrrGroupNo;
    }

    const int total = DeviceDataStore::getDeviceCount();
    if (sW2CurrentAddress > 0) {
        for (int i = 0; i < total; ++i) {
            const SDATA* data = DeviceDataStore::getDevice(i);
            if (!data || data->address != sW2CurrentAddress
                    || std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) != 0) {
                continue;
            }
            const int groupNo = atoi(data->arre);
            if (isValidIrrGroupNo(groupNo)) {
                sSelectedIrrGroupNo = groupNo;
                return groupNo;
            }
        }
    }

    int onlyGroupNo = -1;
    for (int groupNo = 1; groupNo <= 128; ++groupNo) {
        if (!hasBoundValveInIrrGroup(groupNo)) {
            continue;
        }
        if (onlyGroupNo > 0) {
            return -1;
        }
        onlyGroupNo = groupNo;
    }
    if (onlyGroupNo > 0) {
        sSelectedIrrGroupNo = onlyGroupNo;
    }
    return onlyGroupNo;
}

static bool requireSelectedIrrGroup() {
    // Actions from the W2 overview must operate on the row the user selected;
    // do not silently infer a group from the device list.
    if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return true;
    }
    if (mW2ActionTipTextViewPtr) {
        mW2ActionTipTextViewPtr->setText(
                "\xE8\xAF\xB7\xE5\x85\x88\xE9\x80\x89\xE6\x8B\xA9\xE9\x98\x80\xE7\xBB\x84");
    }
    if (mW2ActionTipWindowPtr) {
        mW2ActionTipWindowPtr->showWnd();
        sW2ActionTipWindowVisible = true;
        sW2ActionTipShownAtMs = getW2CurrentTimeMs();
    }
    return false;
}

static void hideW2ActionTipWindowOnly() {
    if (mW2ActionTipWindowPtr) {
        mW2ActionTipWindowPtr->hideWnd();
    }
    sW2ActionTipWindowVisible = false;
    sW2ActionTipShownAtMs = 0;
}

static bool hideW2ActionTipWindowIfVisible() {
    if (!sW2ActionTipWindowVisible) {
        return false;
    }
    if (getW2CurrentTimeMs() - sW2ActionTipShownAtMs < 250LL) {
        return true;
    }
    hideW2ActionTipWindowOnly();
    return true;
}

static void refreshDeviceListViews() {
    sPage2CachedDiscoveryRunning = isWindow5DeviceDiscoveryRunning();
    if (mDeviceTipListViewPtr) {
        mDeviceTipListViewPtr->refreshListView();
    }
    if (mDeviceListViewPtr) {
        mDeviceListViewPtr->refreshListView();
    }
    refreshRunTimeListView();
}

static void refreshChangeIrrListView() {
    if (mChangeIrr_ListViewPtr) {
        mChangeIrr_ListViewPtr->refreshListView();
    }
    refreshWindow4ListViews();
}

static void hideAllPageWindows() {
    ZKWindow* windows[] = {
        mWindow1Ptr,
        mWindow2Ptr,
        mWindow3Ptr,
        mWindow4Ptr,
        mWindow5Ptr,
        mCycleWindowPtr,
        mw2set_windowPtr,
        mGroupBindValueWindowPtr,
    };

    for (int i = 0; i < static_cast<int>(sizeof(windows) / sizeof(windows[0])); ++i) {
        if (windows[i]) {
            windows[i]->hideWnd();
        }
    }
}

static ZKWindow* getW2AddDeviceWindow() {
    return mWindow2Ptr ?
            (ZKWindow*)mWindow2Ptr->findControlByID(ID_MAIN_W2AddDeviceWindow) :
            NULL;
}

static ZKEditText* getW2AddDeviceAddressEditText() {
    ZKWindow* window = getW2AddDeviceWindow();
    return window ?
            (ZKEditText*)window->findControlByID(ID_MAIN_W2AddDeviceAddressEditText) :
            NULL;
}

static ZKTextView* getW2AddDeviceStatusText() {
    ZKWindow* window = getW2AddDeviceWindow();
    return window ?
            (ZKTextView*)window->findControlByID(ID_MAIN_W2AddDeviceStatusText) :
            NULL;
}

static int clampW2AddDeviceAddress(int address) {
    if (address < W2_ADD_DEVICE_ADDRESS_MIN) {
        return W2_ADD_DEVICE_ADDRESS_MIN;
    }
    if (address > W2_ADD_DEVICE_ADDRESS_MAX) {
        return W2_ADD_DEVICE_ADDRESS_MAX;
    }
    return address;
}

static void setW2AddDeviceStatusText(const char* text) {
    ZKTextView* statusText = getW2AddDeviceStatusText();
    if (statusText) {
        statusText->setText(text ? text : "");
    }
}

static void setW2AddDeviceAddressText(int address) {
    sW2AddDeviceAddress = clampW2AddDeviceAddress(address);
    ZKEditText* editText = getW2AddDeviceAddressEditText();
    if (editText) {
        editText->setText(sW2AddDeviceAddress);
    }
}

static int readW2AddDeviceAddressText() {
    ZKEditText* editText = getW2AddDeviceAddressEditText();
    if (!editText) {
        return sW2AddDeviceAddress;
    }
    const std::string text = editText->getText();
    if (text.empty()) {
        return sW2AddDeviceAddress;
    }
    return clampW2AddDeviceAddress(atoi(text.c_str()));
}

static void updateW2AddDeviceTypeButtons() {
    ZKWindow* window = getW2AddDeviceWindow();
    ZKButton* sensorButton = window ?
            (ZKButton*)window->findControlByID(ID_MAIN_W2AddDeviceSensorButton) : NULL;
    ZKButton* valveButton = window ?
            (ZKButton*)window->findControlByID(ID_MAIN_W2AddDeviceValveButton) : NULL;
    if (sensorButton) {
        sensorButton->setSelected(sW2AddDeviceSensor);
    }
    if (valveButton) {
        valveButton->setSelected(!sW2AddDeviceSensor);
    }
}

static int findW2DeviceIndexByAddress(int address) {
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && data->address == address) {
            return i;
        }
    }
    return -1;
}

static void hideW2AddDeviceWindowOnly() {
    ZKWindow* window = getW2AddDeviceWindow();
    if (window) {
        window->hideWnd();
    }
    sW2AddDeviceWindowVisible = false;
}

static void openW2AddDeviceWindow() {
    sW2AddDeviceSensor = false;
    setW2AddDeviceAddressText(getW2DefaultAddress());
    setW2AddDeviceStatusText("");
    updateW2AddDeviceTypeButtons();
    ZKWindow* window = getW2AddDeviceWindow();
    if (window) {
        window->showWnd();
        sW2AddDeviceWindowVisible = true;
    }
}

static void stepW2AddDeviceAddress(int delta) {
    const int address = readW2AddDeviceAddressText();
    setW2AddDeviceAddressText(address + delta);
    setW2AddDeviceStatusText("");
}

static void openW2SetWindowFromAddDeviceDialog() {
    const int address = sW2AddDeviceAddress;
    const bool sensor = sW2AddDeviceSensor;
    hideW2AddDeviceWindowOnly();
    openW2SetWindow(DeviceDataStore::getDeviceCount());
    sW2SelectedTypeIndex = sensor ? 1 : 0;
    updateW2AddressDisplay(address);
    if (mW2_NameEditTextPtr) {
        mW2_NameEditTextPtr->setText(getW2SelectedDeviceType());
    }
    refreshChangeIrrListView();
}

static void confirmW2AddDeviceWindow() {
    const int address = readW2AddDeviceAddressText();
    setW2AddDeviceAddressText(address);
    setW2AddDeviceStatusText("正在测试地址...");

    char message[128] = {0};
    if (!requestWindow5CheckConfigForW2Add(
            sW2AddDeviceAddress, sW2AddDeviceSensor, message, sizeof(message))) {
        showW2TipText(message);
        return;
    }

    if (findW2DeviceIndexByAddress(sW2AddDeviceAddress) >= 0) {
        showW2TipText("地址已在列表中");
        return;
    }

    openW2SetWindowFromAddDeviceDialog();
}

static bool handleW2AddDeviceWindowClick(ZKBase *pBase) {
    if (!sW2AddDeviceWindowVisible || !pBase) {
        return false;
    }

    const int id = pBase->getID();
    switch (id) {
    case ID_MAIN_W2AddDeviceSensorButton:
        sW2AddDeviceSensor = true;
        updateW2AddDeviceTypeButtons();
        setW2AddDeviceStatusText("");
        return true;
    case ID_MAIN_W2AddDeviceValveButton:
        sW2AddDeviceSensor = false;
        updateW2AddDeviceTypeButtons();
        setW2AddDeviceStatusText("");
        return true;
    case ID_MAIN_W2AddDeviceAddressPrevButton:
        stepW2AddDeviceAddress(-1);
        return true;
    case ID_MAIN_W2AddDeviceAddressNextButton:
        stepW2AddDeviceAddress(1);
        return true;
    case ID_MAIN_W2AddDeviceCancelButton:
        hideW2AddDeviceWindowOnly();
        return true;
    case ID_MAIN_W2AddDeviceOkButton:
        confirmW2AddDeviceWindow();
        return true;
    case ID_MAIN_W2AddDeviceAddressEditText:
        return false;
    default:
        return true;
    }
}

static void setW2TipText(const char* text) {
#if defined(ID_MAIN_W2TextView)
    if (mW2TextViewPtr) {
        mW2TextViewPtr->setText(text);
    }
#elif defined(ID_MAIN_W2TipTextView)
    if (mW2TipTextViewPtr) {
        mW2TipTextViewPtr->setText(text);
    }
#elif defined(ID_MAIN_W2TipText)
    if (mW2TipTextPtr) {
        mW2TipTextPtr->setText(text);
    }
#elif defined(ID_MAIN_W2Tip_TextView)
    if (mW2Tip_TextViewPtr) {
        mW2Tip_TextViewPtr->setText(text);
    }
#else
    (void)text;
#endif
}

static bool showW2TipText(const char* text) {
    setW2TipText(text);
#if defined(ID_MAIN_W2TipWindow)
    if (mW2TipWindowPtr) {
        mW2TipWindowPtr->showWnd();
        sW2TipWindowVisible = true;
        sW2TipShownAtMs = getW2CurrentTimeMs();
        return true;
    }
#endif
    return false;
}

static void hideW2TipWindowOnly() {
#if defined(ID_MAIN_W2TipWindow)
    if (mW2TipWindowPtr) {
        mW2TipWindowPtr->hideWnd();
    }
#endif
    sW2TipWindowVisible = false;
    sW2TipShownAtMs = 0;
}

static bool hideW2TipWindowIfVisible() {
    if (!sW2TipWindowVisible) {
        return false;
    }
    if (getW2CurrentTimeMs() - sW2TipShownAtMs < 250LL) {
        return true;
    }
    hideW2TipWindowOnly();
    return true;
}

static void showDeviceListEmptyRow() {
    if (mDeviceListViewPtr) {
        const int rowCount = DeviceDataStore::getDeviceListRowCount();
        if (rowCount > 0) {
            mDeviceListViewPtr->setSelection(rowCount - 1);
        }
    }
}

static void showChangeIrrListEmptyRow() {
    if (mChangeIrr_ListViewPtr) {
        mChangeIrr_ListViewPtr->setSelection(static_cast<int>(sIrrGroupNumbers.size()));
    }
}

static void updateClearIrrButtonText() {
    if (mIrrNumValue_TextViewPtr) {
        char backgroundPic[64] = {0};
        if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            snprintf(backgroundPic, sizeof(backgroundPic),
                    "w2_set_irr_value_%03d.png", sSelectedIrrGroupNo);
        } else {
            snprintf(backgroundPic, sizeof(backgroundPic),
                    "w2_set_irr_value104_none.png");
        }
        mIrrNumValue_TextViewPtr->setText("");
        mIrrNumValue_TextViewPtr->setBackgroundPic(backgroundPic);
    }
}

static bool isValidIrrGroupNo(int groupNo) {
    return groupNo > 0 && groupNo <= 128;
}

static bool isDeviceBoundToIrrGroup(const SDATA* data, int groupNo) {
    if (!data || !isValidIrrGroupNo(groupNo)) {
        return false;
    }
    if (std::strcmp(data->arre, "*") == 0) {
        return hasBoundValveInIrrGroup(groupNo);
    }
    return DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo);
}

static const char* getIrrGroupName(int groupNo) {
    return isValidIrrGroupNo(groupNo) ? DeviceDataStore::getIrrGroupName(groupNo) : "";
#if 0
    static char nameText[32] = {0};
    if (isValidIrrGroupNo(groupNo)) {
        snprintf(nameText, sizeof(nameText), "阀组[%d]", groupNo);
    } else {
        nameText[0] = '\0';
    }
    return nameText;
#endif
}

static void appendTextPart(char* text, size_t size, const char* part, const char* separator) {
    if (!text || size == 0 || !part || part[0] == '\0') {
        return;
    }

    const bool hasText = text[0] != '\0';
    snprintf(text + strlen(text), size - strlen(text), "%s%s", hasText ? separator : "", part);
}

static bool collectUngroupedValveAddresses(char* text, size_t size) {
    if (!text || size == 0) {
        return false;
    }

    text[0] = '\0';
    std::vector<int> addresses;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) != 0) {
            continue;
        }

        if (isValidIrrGroupNo(atoi(data->arre))) {
            continue;
        }

        addresses.push_back(data->address);
    }

    if (addresses.empty()) {
        return false;
    }

    std::sort(addresses.begin(), addresses.end());
    addresses.erase(std::unique(addresses.begin(), addresses.end()), addresses.end());

    int lineLength = 0;
    int lineLimit = 24;
    for (size_t i = 0; i < addresses.size();) {
        const int first = addresses[i];
        int last = first;
        while (i + 1 < addresses.size() && addresses[i + 1] == last + 1) {
            last = addresses[++i];
        }

        char rangeText[32] = {0};
        if (first == last) {
            snprintf(rangeText, sizeof(rangeText), "%d", first);
        } else {
            snprintf(rangeText, sizeof(rangeText), "%d-%d", first, last);
        }

        const int separatorLength = text[0] == '\0' ? 0 : 2;
        const int rangeLength = static_cast<int>(strlen(rangeText));
        const bool wrapLine = lineLength > 0
                && lineLength + separatorLength + rangeLength > lineLimit;
        appendTextPart(text, size, rangeText, wrapLine ? ",\n" : ", ");
        lineLength = wrapLine ? rangeLength : lineLength + separatorLength + rangeLength;
        if (wrapLine) {
            lineLimit = 34;
        }
        ++i;
    }

    return true;
}

static bool showW2UngroupedValveTipIfNeeded() {
    char addresses[1024] = {0};
    if (!collectUngroupedValveAddresses(addresses, sizeof(addresses))) {
        return false;
    }

    char tipText[1152] = {0};
    snprintf(tipText, sizeof(tipText), "电磁阀[%s]未添加到阀组", addresses);
    return showW2TipText(tipText);
}

static bool handlePage2BeforeMainPageSwitch(int targetPageIndex) {
    if (targetPageIndex == BACK_GROUND_BTN_2) {
        return true;
    }

    if (hideW2TipWindowIfVisible()) {
        return false;
    }

    if (sW2TipCheckedThisVisit) {
        return true;
    }

    sW2TipCheckedThisVisit = true;
    return !showW2UngroupedValveTipIfNeeded();
}

static bool isW2PreviewValveForGroup(int groupNo) {
    return sW2SetWindowOpen
            && isValidIrrGroupNo(groupNo)
            && groupNo == sSelectedIrrGroupNo
            && sW2SelectedTypeIndex == 0;
}

static int getW2PreviewAddress() {
    if (sW2CurrentAddress > 0) {
        return sW2CurrentAddress;
    }
    return getW2DefaultAddress();
}

static void updateW2AddressDisplay(int address) {
    if (!mTextView1Ptr || address <= 0 || address > 255) {
        return;
    }

    char addressPic[64] = {0};
    snprintf(addressPic, sizeof(addressPic), "w2_set_address_combined_%03d.png", address);
    sW2CurrentAddress = address;
    mTextView1Ptr->setBackgroundPic(addressPic);
}

static void buildIrrGroupDisplayText(int groupNo, char* text, size_t size) {
    if (!text || size == 0) {
        return;
    }

    text[0] = '\0';
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }

    std::vector<int> valveAddresses;
    char waterPumpAddresses[64] = {0};
    char sensorNames[160] = {0};
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (sW2SetWindowOpen && i == sW2EditingIndex && DeviceDataStore::isCustomDevice(i)) {
            continue;
        }
        if (!isDeviceBoundToIrrGroup(data, groupNo)) {
            continue;
        }

        if (std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0) {
            valveAddresses.push_back(data->address);
            continue;
        }

        if (std::strcmp(data->type, "水泵") == 0) {
            char addressText[16] = {0};
            snprintf(addressText, sizeof(addressText), "%d", data->address);
            appendTextPart(waterPumpAddresses, sizeof(waterPumpAddresses), addressText, "，");
            continue;
        }

        if (!isSensorTextDevice(data)) {
            continue;
        }

        char shortName[32] = {0};
        snprintf(shortName, sizeof(shortName), "%s", data->name);
        char* suffix = strstr(shortName, "传感器");
        if (suffix && suffix[9] == '\0') {
            *suffix = '\0';
        }
        appendTextPart(sensorNames, sizeof(sensorNames), shortName, " ");
    }

    if (isW2PreviewValveForGroup(groupNo)) {
        valveAddresses.push_back(getW2PreviewAddress());
    }

    char summary[256] = {0};
    if (!valveAddresses.empty()) {
        std::sort(valveAddresses.begin(), valveAddresses.end());
        valveAddresses.erase(
                std::unique(valveAddresses.begin(), valveAddresses.end()),
                valveAddresses.end());
        char addresses[96] = {0};
        for (size_t i = 0; i < valveAddresses.size(); ++i) {
            char addressText[16] = {0};
            snprintf(addressText, sizeof(addressText), "%d", valveAddresses[i]);
            appendTextPart(addresses, sizeof(addresses), addressText, ", ");
        }
        char valveText[160] = {0};
        snprintf(valveText, sizeof(valveText), "%s[%s]", W2_DEVICE_TYPE_VALVE, addresses);
        appendTextPart(summary, sizeof(summary), valveText, "");
    }

    char pumpSensorText[224] = {0};
    if (waterPumpAddresses[0] != '\0') {
        snprintf(pumpSensorText, sizeof(pumpSensorText), "水泵%s", waterPumpAddresses);
    }
    if (sensorNames[0] != '\0') {
        appendTextPart(pumpSensorText, sizeof(pumpSensorText), sensorNames, " ");
    }
    if (pumpSensorText[0] != '\0') {
        char pumpSensorBracket[240] = {0};
        snprintf(pumpSensorBracket, sizeof(pumpSensorBracket), "[%s]", pumpSensorText);
        appendTextPart(summary, sizeof(summary), pumpSensorBracket, "");
    }

    if (summary[0] != '\0') {
        snprintf(text, size, "%s %s", getIrrGroupName(groupNo), summary);
    } else {
        snprintf(text, size, "%s     [空]", getIrrGroupName(groupNo));
    }
}

static void resetIrrGroupSelection() {
    sSelectedIrrGroupNo = -1;
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setText("");
        mGroupNameEditTextPtr->setTextColor(static_cast<int>(0x00FFFFFFU));
    }
    updateClearIrrButtonText();
    refreshChangeIrrListView();
}

static void selectIrrGroup(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }
    if (!canAssignCurrentW2ValveToIrrGroup(groupNo, true)) {
        return;
    }

    sSelectedIrrGroupNo = groupNo;
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setText("");
    }
    updateClearIrrButtonText();
    refreshChangeIrrListView();
}

static void renameSelectedIrrGroup(const std::string &name) {
    if (!isValidIrrGroupNo(sSelectedIrrGroupNo) || name.empty()) {
        return;
    }
    if (DeviceDataStore::setIrrGroupName(sSelectedIrrGroupNo, name.c_str())) {
        refreshChangeIrrListView();
    }
}

static void closeGroupRenameWindow() {
    if (mGroupRenameWindowPtr) {
        mGroupRenameWindowPtr->hideWnd();
    }
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setText("");
    }
    sGroupRenameWindowVisible = false;
}

static void openGroupRenameWindow() {
    if (!requireSelectedIrrGroup()) {
        return;
    }
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setTextColor(static_cast<int>(0xFF005BBBU));
        mGroupNameEditTextPtr->setText(getIrrGroupName(sSelectedIrrGroupNo));
    }
    if (mGroupRenameWindowPtr) {
        mGroupRenameWindowPtr->showWnd();
        sGroupRenameWindowVisible = true;
    }
}

static void saveGroupRenameWindow() {
    if (sGroupRenameWindowVisible && mGroupNameEditTextPtr) {
        const std::string name = mGroupNameEditTextPtr->getText();
        if (!name.empty()) {
            renameSelectedIrrGroup(name);
        }
    }
    closeGroupRenameWindow();
}

static void updateIrrCapacitySelection() {
    if (mIrrCapacity2ButtonPtr) {
        mIrrCapacity2ButtonPtr->setSelected(sIrrCapacityPending == 2);
    }
    if (mIrrCapacity3ButtonPtr) {
        mIrrCapacity3ButtonPtr->setSelected(sIrrCapacityPending == 3);
    }
    if (mIrrCapacity4ButtonPtr) {
        mIrrCapacity4ButtonPtr->setSelected(sIrrCapacityPending == 4);
    }
}

static void selectIrrCapacity(int capacity) {
    if (capacity < 2 || capacity > 4) {
        return;
    }
    sIrrCapacityPending = capacity;
    updateIrrCapacitySelection();
}

static void restoreW2AutoAssignSnapshot() {
    w2_DeviceDataList = sW2AutoAssignDeviceSnapshot;
    sIrrGroupNumbers = sW2AutoAssignGroupSnapshot;
    sW2AddingDevice = sW2AutoAssignOriginalAddingDevice;
    sW2EditingIndex = sW2AutoAssignOriginalEditingIndex;
    sSelectedIrrGroupNo = sW2AutoAssignOriginalSelectedGroupNo;
    sIrrGroupValveLimit = sW2AutoAssignOriginalValveLimit;
}

static void beginW2AutoAssignPreview() {
    if (sW2AutoAssignPreviewPending) {
        restoreW2AutoAssignSnapshot();
        return;
    }

    sW2AutoAssignDeviceSnapshot = w2_DeviceDataList;
    sW2AutoAssignGroupSnapshot = sIrrGroupNumbers;
    sW2AutoAssignOriginalAddingDevice = sW2AddingDevice;
    sW2AutoAssignOriginalEditingIndex = sW2EditingIndex;
    sW2AutoAssignOriginalSelectedGroupNo = sSelectedIrrGroupNo;
    sW2AutoAssignOriginalValveLimit = sIrrGroupValveLimit;
    sW2AutoAssignPreviewPending = true;
}

static void finishW2AutoAssignPreview(bool commit) {
    if (!sW2AutoAssignPreviewPending) {
        return;
    }
    if (!commit) {
        restoreW2AutoAssignSnapshot();
        refreshDeviceListViews();
        refreshChangeIrrListView();
    }
    sW2AutoAssignPreviewPending = false;
    sW2AutoAssignDeviceSnapshot.clear();
    sW2AutoAssignGroupSnapshot.clear();
}

static void openIrrCapacityWindow() {
    sIrrCapacityPending = sIrrCapacityLast;
    updateIrrCapacitySelection();
    if (mIrrCapacityWindowPtr) {
        mIrrCapacityWindowPtr->showWnd();
        sIrrCapacityWindowVisible = true;
    }
}

static int getIrrGroupValveCount(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
                && DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            ++count;
        }
    }
    return count;
}

static bool isCurrentW2ValveInIrrGroup(int groupNo) {
    if (!isValidIrrGroupNo(groupNo) || sW2SelectedTypeIndex != 0) {
        return false;
    }

    const SDATA* data = DeviceDataStore::getDevice(sW2EditingIndex);
    if (data && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
            && DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
        return true;
    }

    if (sW2CurrentAddress <= 0) {
        return false;
    }
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        data = DeviceDataStore::getDevice(i);
        if (data && data->address == sW2CurrentAddress
                && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
                && DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            return true;
        }
    }
    return false;
}

static bool canAssignCurrentW2ValveToIrrGroup(int groupNo, bool showTip) {
    if (!isValidIrrGroupNo(groupNo) || sW2SelectedTypeIndex != 0
            || isCurrentW2ValveInIrrGroup(groupNo)
            || getIrrGroupValveCount(groupNo) < sIrrGroupValveLimit) {
        return true;
    }

    if (showTip) {
        char tipText[96] = {0};
        snprintf(tipText, sizeof(tipText),
                "该阀组最多可添加%d个电磁阀", sIrrGroupValveLimit);
        showW2TipText(tipText);
    }
    return false;
}

static int saveCurrentW2ValveForAutoAssign() {
    if (!sW2SetWindowOpen || sW2SelectedTypeIndex != 0
            || sW2CurrentAddress <= 0) {
        return -1;
    }

    std::string nameText;
    if (mW2_NameEditTextPtr) {
        nameText = mW2_NameEditTextPtr->getText();
    }

    if (sW2AddingDevice) {
        if (!DeviceDataStore::addDevice(sW2CurrentAddress, nameText.c_str(),
                W2_DEVICE_TYPE_VALVE)) {
            return -1;
        }
        sW2EditingIndex = DeviceDataStore::getDeviceCount() - 1;
        sW2AddingDevice = false;
        return sW2EditingIndex;
    }

    if (!DeviceDataStore::isCustomDevice(sW2EditingIndex)) {
        return -1;
    }
    if (!DeviceDataStore::updateDevice(sW2EditingIndex, sW2CurrentAddress,
            nameText.c_str(), W2_DEVICE_TYPE_VALVE)) {
        return -1;
    }
    return sW2EditingIndex;
}

static void autoAssignUngroupedValves(int capacity) {
    if (capacity < 2 || capacity > 4) {
        return;
    }

    const int currentValveIndex = saveCurrentW2ValveForAutoAssign();
    std::vector<int> ungrouped;
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
                && !isValidIrrGroupNo(atoi(data->arre))) {
            ungrouped.push_back(i);
        }
    }
    std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
    size_t valveIndex = 0;
    int firstCreatedGroupNo = -1;
    for (size_t i = 0; i < sIrrGroupNumbers.size() && valveIndex < ungrouped.size(); ++i) {
        const int groupNo = sIrrGroupNumbers[i];
        int currentCount = getIrrGroupValveCount(groupNo);
        while (currentCount < capacity && valveIndex < ungrouped.size()) {
            DeviceDataStore::bindDeviceToIrrGroup(ungrouped[valveIndex], groupNo);
            ++valveIndex;
            ++currentCount;
        }
    }

    while (valveIndex < ungrouped.size()) {
        int groupNo = 1;
        while (groupNo <= 128 && std::find(sIrrGroupNumbers.begin(),
                sIrrGroupNumbers.end(), groupNo) != sIrrGroupNumbers.end()) {
            ++groupNo;
        }
        if (groupNo > 128) {
            break;
        }
        if (firstCreatedGroupNo < 0) {
            firstCreatedGroupNo = groupNo;
        }
        sIrrGroupNumbers.push_back(groupNo);
        for (int count = 0; count < capacity && valveIndex < ungrouped.size(); ++count) {
            DeviceDataStore::bindDeviceToIrrGroup(ungrouped[valveIndex], groupNo);
            ++valveIndex;
        }
    }

    std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
    int currentGroupNo = -1;
    const SDATA* currentValve = DeviceDataStore::getDevice(currentValveIndex);
    if (currentValve) {
        const int groupNo = atoi(currentValve->arre);
        if (isValidIrrGroupNo(groupNo)) {
            currentGroupNo = groupNo;
            sSelectedIrrGroupNo = groupNo;
            updateClearIrrButtonText();
        }
    }
    refreshDeviceListViews();
    refreshChangeIrrListView();
    const int selectionGroupNo = currentGroupNo > 0
            ? currentGroupNo : firstCreatedGroupNo;
    if (selectionGroupNo > 0 && mChangeIrr_ListViewPtr) {
        const std::vector<int>::const_iterator it = std::find(
                sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), selectionGroupNo);
        if (it != sIrrGroupNumbers.end()) {
            mChangeIrr_ListViewPtr->setSelection(
                    static_cast<int>(it - sIrrGroupNumbers.begin()));
        }
    }
}

static void closeIrrCapacityWindow(bool save) {
    const bool apply = save && sIrrCapacityWindowVisible;
    if (mIrrCapacityWindowPtr) {
        mIrrCapacityWindowPtr->hideWnd();
    }
    sIrrCapacityWindowVisible = false;
    if (apply) {
        sIrrCapacityLast = sIrrCapacityPending;
        beginW2AutoAssignPreview();
        sIrrGroupValveLimit = sIrrCapacityPending == 4
                ? 4 : DEFAULT_IRR_GROUP_VALVE_LIMIT;
        autoAssignUngroupedValves(sIrrCapacityPending);
    }
}

static int getChangeIrrListItemCount() {
    return static_cast<int>(sIrrGroupNumbers.size()) + 1;
}

static bool isIrrGroupEmptyRow(int index) {
    return index < 0 || index >= getChangeIrrListItemCount()
            || index == static_cast<int>(sIrrGroupNumbers.size());
}

static void setIrrGroupSubItemTexts(ZKListView::ZKListItem *pListItem, int index, int numSubItemId, int nameSubItemId) {
    ZKListView::ZKListSubItem* numSubItem = pListItem ? pListItem->findSubItemByID(numSubItemId) : NULL;
    ZKListView::ZKListSubItem* nameSubItem = pListItem ? pListItem->findSubItemByID(nameSubItemId) : NULL;
    if (!sIrrEmptyItemLayoutCaptured && numSubItem && nameSubItem) {
        sIrrNumSubItemPosition = numSubItem->getPosition();
        sIrrArrSubItemPosition = nameSubItem->getPosition();
        sIrrEmptyItemLayoutCaptured = true;
    }
    if (isIrrGroupEmptyRow(index)) {
        setListSubItemText(pListItem, numSubItemId, "");
        setListSubItemText(pListItem, nameSubItemId, "点击添加");
        setListSubItemAlignment(pListItem, nameSubItemId, ZKTextView::E_ALIGN_H_CENTER, ZKTextView::E_ALIGN_V_CENTER);
        if (numSubItem) {
        setListSubItemVisible(pListItem, numSubItemId, false);
        }
        if (nameSubItem) {
            LayoutPosition lp = sIrrEmptyItemLayoutCaptured ? sIrrArrSubItemPosition : nameSubItem->getPosition();
            lp.mLeft = 0;
            lp.mWidth = pListItem->getPosition().mWidth;
            setListSubItemPosition(pListItem, nameSubItemId, lp);
            setListSubItemVisible(pListItem, nameSubItemId, true);
        }
        return;
    }

    const int groupNo = sIrrGroupNumbers[index];
    char numText[16] = {0};
    char nameText[256] = {0};
    snprintf(numText, sizeof(numText), "%d", groupNo);
    buildIrrGroupDisplayText(groupNo, nameText, sizeof(nameText));

    setListSubItemText(pListItem, numSubItemId, numText);
    setListSubItemText(pListItem, nameSubItemId, nameText);
    setListSubItemAlignment(pListItem, nameSubItemId, ZKTextView::E_ALIGN_H_CENTER, ZKTextView::E_ALIGN_V_CENTER);
    setListSubItemVisible(pListItem, numSubItemId, true);
    setListSubItemVisible(pListItem, nameSubItemId, true);
    if (sIrrEmptyItemLayoutCaptured) {
        setListSubItemPosition(pListItem, numSubItemId, sIrrNumSubItemPosition);
        setListSubItemPosition(pListItem, nameSubItemId, sIrrArrSubItemPosition);
    } else if (numSubItem && nameSubItem) {
        setListSubItemPosition(pListItem, numSubItemId, numSubItem->getPosition());
        setListSubItemPosition(pListItem, nameSubItemId, nameSubItem->getPosition());
    }
}

static void obtainChangeIrrListItemData(ZKListView::ZKListItem *pListItem, int index) {
    setIrrGroupSubItemTexts(pListItem, index, ID_MAIN_IrrNum_SubItem, ID_MAIN_IrrArr_SubItem);
}

static void onChangeIrrListItemClick(int index) {
    if (index < 0 || index >= getChangeIrrListItemCount()) {
        return;
    }

    if (index == static_cast<int>(sIrrGroupNumbers.size())) {
        if (sIrrGroupNumbers.size() < 128U) {
            int newGroupNo = 1;
            while (std::find(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), newGroupNo)
                    != sIrrGroupNumbers.end()) {
                ++newGroupNo;
            }
            sIrrGroupNumbers.push_back(newGroupNo);
            std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
            refreshChangeIrrListView();
            showChangeIrrListEmptyRow();
        }
        return;
    }

    selectIrrGroup(sIrrGroupNumbers[index]);
}

static void clearSelectedIrrGroup() {
    if (!requireSelectedIrrGroup()) {
        return;
    }
    DeviceDataStore::clearIrrGroup(sSelectedIrrGroupNo);
    refreshDeviceListViews();
    resetIrrGroupSelection();
}

static bool isClearIrrWindowVisible() {
    return sClearIrrWindowVisible;
}

static void closeClearIrrWindow() {
    if (mWindow9Ptr) {
        mWindow9Ptr->hideWnd();
    }
    sClearIrrWindowVisible = false;
}

static void openClearIrrWindow() {
    if (!requireSelectedIrrGroup()) {
        return;
    }

    if (mClearIrrPromptTextViewPtr) {
        char text[160] = {0};
        snprintf(text, sizeof(text),
                "\xE6\xB8\x85\xE7\xA9\xBA\xE5\xBD\x93\xE5\x89\x8D\xE9\x80\x89\xE4\xB8\xAD\xE7\x9A\x84%s\xEF\xBC\x8C\n"
                "\xE8\xBF\x98\xE6\x98\xAF\xE6\xB8\x85\xE7\xA9\xBA\xE5\x85\xA8\xE9\x83\xA8\xE9\x98\x80\xE7\xBB\x84\xEF\xBC\x9F",
                getIrrGroupName(sSelectedIrrGroupNo));
        mClearIrrPromptTextViewPtr->setText(text);
    }
    if (mWindow9Ptr) {
        mWindow9Ptr->showWnd();
        sClearIrrWindowVisible = true;
        sW2ChoiceDialogMode = 1;
    }
}

static void openGroupBindScopeWindow() {
    if (!requireSelectedIrrGroup()) {
        return;
    }

    if (mClearIrrPromptTextViewPtr) {
        char text[160] = {0};
        snprintf(text, sizeof(text),
                "\xE7\xBB\x91\xE5\xAE\x9A\xE5\xBD\x93\xE5\x89\x8D\xE9\x80\x89\xE4\xB8\xAD\xE7\x9A\x84%s\xEF\xBC\x8C\n"
                "\xE8\xBF\x98\xE6\x98\xAF\xE7\xBB\x91\xE5\xAE\x9A\xE5\x85\xA8\xE9\x83\xA8\xE9\x98\x80\xE7\xBB\x84\xEF\xBC\x9F",
                getIrrGroupName(sSelectedIrrGroupNo));
        mClearIrrPromptTextViewPtr->setText(text);
    }
    if (mWindow9Ptr) {
        mWindow9Ptr->showWnd();
        sClearIrrWindowVisible = true;
        sW2ChoiceDialogMode = 2;
    }
}

static void clearCurrentIrrGroupFromDialog() {
    if (!sClearIrrWindowVisible) {
        return;
    }
    closeClearIrrWindow();
    clearSelectedIrrGroup();
}

static void clearAllIrrGroupsFromDialog() {
    if (!sClearIrrWindowVisible) {
        return;
    }
    closeClearIrrWindow();
    for (size_t i = 0; i < sIrrGroupNumbers.size(); ++i) {
        DeviceDataStore::clearIrrGroup(sIrrGroupNumbers[i]);
    }
    refreshDeviceListViews();
    resetIrrGroupSelection();
}

static bool prepareSelectedIrrGroupNameEdit() {
    if (!requireSelectedIrrGroup()) {
        return false;
    }
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setTextColor(static_cast<int>(0xFF005BBBU));
        mGroupNameEditTextPtr->setText(getIrrGroupName(sSelectedIrrGroupNo));
    }
    return true;
}

static bool isPumpDevice(const SDATA* data) {
    return data && (std::strcmp(data->type, "水泵") == 0 || std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0);
}

static bool isFactorySelectableSensorDevice(const SDATA* data) {
    return data && (data->address == 6 || data->address == 8);
}

static bool isGroupBindPumpDevice(const SDATA* data) {
    return data && data->address >= 1 && data->address <= 2 && isPumpDevice(data);
}

static bool isCustomSelectableDevice(const SDATA* data) {
    return data && data->address >= CUSTOM_DEVICE_START_ID;
}

static bool isSensorBindDevice(const SDATA* data) {
    return data
            && (std::strcmp(data->type, W2_DEVICE_TYPE_SENSOR) == 0
                    || std::strcmp(data->type, "传感器") == 0)
            && (isFactorySelectableSensorDevice(data) || isCustomSelectableDevice(data));
}

static bool isSensorTextDevice(const SDATA* data) {
    return data
            && (std::strcmp(data->type, W2_DEVICE_TYPE_SENSOR) == 0
                    || std::strcmp(data->type, "传感器") == 0);
}

static int getFilteredDeviceCount(bool pump) {
    int count = 0;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if ((pump && isGroupBindPumpDevice(data)) || (!pump && isSensorBindDevice(data))) {
            ++count;
        }
    }
    return count;
}

static int getFilteredDeviceIndex(bool pump, int rowIndex) {
    int count = 0;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if ((pump && isGroupBindPumpDevice(data)) || (!pump && isSensorBindDevice(data))) {
            if (count == rowIndex) {
                return i;
            }
            ++count;
        }
    }
    return -1;
}

static bool containsSelectedDevice(const std::vector<int>& selectedIndexes, int deviceIndex) {
    for (size_t i = 0; i < selectedIndexes.size(); ++i) {
        if (selectedIndexes[i] == deviceIndex) {
            return true;
        }
    }
    return false;
}

static void addSelectedDevice(std::vector<int>& selectedIndexes, int deviceIndex) {
    if (deviceIndex < 0 || containsSelectedDevice(selectedIndexes, deviceIndex)) {
        return;
    }

    std::vector<int>::iterator it = selectedIndexes.begin();
    for (; it != selectedIndexes.end(); ++it) {
        if (*it > deviceIndex) {
            break;
        }
    }
    selectedIndexes.insert(it, deviceIndex);
}

static void removeSelectedDevice(std::vector<int>& selectedIndexes, int deviceIndex) {
    for (std::vector<int>::iterator it = selectedIndexes.begin(); it != selectedIndexes.end(); ++it) {
        if (*it == deviceIndex) {
            selectedIndexes.erase(it);
            return;
        }
    }
}

static void buildSelectedDeviceText(
        const std::vector<int>& selectedIndexes, char* text, size_t textSize) {
    if (!text || textSize == 0) {
        return;
    }
    text[0] = '\0';
    size_t offset = 0;
    for (size_t i = 0; i < selectedIndexes.size(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(selectedIndexes[i]);
        if (!data) {
            continue;
        }

        const int written = snprintf(
                text + offset,
                textSize - offset,
                "%s%d",
                (offset == 0) ? "" : "-",
                data->address);
        if (written <= 0 || static_cast<size_t>(written) >= textSize - offset) {
            break;
        }
        offset += static_cast<size_t>(written);
    }
}

static void setSelectedDeviceText(ZKEditText* editText, const std::vector<int>& selectedIndexes) {
    if (!editText) {
        return;
    }

    char text[256] = {0};
    buildSelectedDeviceText(selectedIndexes, text, sizeof(text));
    editText->setText(text);
}

static int normalizePumpAdvanceSeconds(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > MAX_PUMP_ADVANCE_SECONDS) {
        return MAX_PUMP_ADVANCE_SECONDS;
    }
    return value;
}

static int getIrrGroupPumpAdvanceSeconds(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return DEFAULT_PUMP_ADVANCE_SECONDS;
    }
    if (!sIrrGroupPumpAdvanceSecondsSet[groupNo - 1]) {
        return DEFAULT_PUMP_ADVANCE_SECONDS;
    }
    return sIrrGroupPumpAdvanceSeconds[groupNo - 1];
}

static void setIrrGroupPumpAdvanceSeconds(int groupNo, int seconds) {
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }
    sIrrGroupPumpAdvanceSeconds[groupNo - 1] = normalizePumpAdvanceSeconds(seconds);
    sIrrGroupPumpAdvanceSecondsSet[groupNo - 1] = true;
}

static ZKEditText* getPumpAdvanceSecEditText() {
    if (!sPumpAdvanceSecEditTextPtr && mGroupBindValueWindowPtr) {
        sPumpAdvanceSecEditTextPtr =
                (ZKEditText*)mGroupBindValueWindowPtr->findControlByID(ID_MAIN_PumpAdvanceSecEditText);
    }
    return sPumpAdvanceSecEditTextPtr;
}

static int readPumpAdvanceSecondsFromEditText() {
    ZKEditText* editText = getPumpAdvanceSecEditText();
    if (!editText) {
        return DEFAULT_PUMP_ADVANCE_SECONDS;
    }
    const std::string text = editText->getText();
    if (text.empty()) {
        return DEFAULT_PUMP_ADVANCE_SECONDS;
    }
    return normalizePumpAdvanceSeconds(atoi(text.c_str()));
}

static void updatePumpAdvanceSecEditText() {
    ZKEditText* editText = getPumpAdvanceSecEditText();
    if (!editText) {
        return;
    }
    const int seconds = sGroupBindAllGroups ?
            DEFAULT_PUMP_ADVANCE_SECONDS :
            getIrrGroupPumpAdvanceSeconds(sSelectedIrrGroupNo);
    editText->setText(seconds);
}

static void savePumpAdvanceSecondsFromEditText() {
    const int seconds = readPumpAdvanceSecondsFromEditText();
    if (sGroupBindAllGroups) {
        for (size_t i = 0; i < sIrrGroupNumbers.size(); ++i) {
            setIrrGroupPumpAdvanceSeconds(sIrrGroupNumbers[i], seconds);
        }
    } else {
        setIrrGroupPumpAdvanceSeconds(sSelectedIrrGroupNo, seconds);
    }
}

static void updateGroupBindSelectionEditTexts() {
    setSelectedDeviceText(mAddPumpEditTextPtr, sSelectedPumpDeviceIndexes);
    setSelectedDeviceText(mAddSenserEditTextPtr, sSelectedSensorDeviceIndexes);

    if (mTextView7Ptr) {
        mTextView7Ptr->setText("");
    }
    if (mTextView6Ptr) {
        mTextView6Ptr->setText("");
    }
}

static void clearGroupBindSelections() {
    sSelectedPumpDeviceIndexes.clear();
    sSelectedSensorDeviceIndexes.clear();
    updateGroupBindSelectionEditTexts();
}

static void syncGroupBindSelectionsFromCurrentGroup() {
    clearGroupBindSelections();

    if (!isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return;
    }

    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        const bool matchesScope = data && (sGroupBindAllGroups
                ? std::strcmp(data->arre, "*") == 0
                : isDeviceBoundToIrrGroup(data, sSelectedIrrGroupNo));
        if (!matchesScope) {
            continue;
        }

        if (isGroupBindPumpDevice(data)) {
            addSelectedDevice(sSelectedPumpDeviceIndexes, i);
        } else if (isSensorBindDevice(data)) {
            addSelectedDevice(sSelectedSensorDeviceIndexes, i);
        }
    }

    updateGroupBindSelectionEditTexts();
}

static void refreshGroupBindListViews() {
    if (mSelectPumpListViewPtr) {
        mSelectPumpListViewPtr->refreshListView();
    }
    if (mSelectSenserListViewPtr) {
        mSelectSenserListViewPtr->refreshListView();
    }
}

static void updateGroupBindEditTexts() {
    char line[256] = {0};
    if (mGroupNumEditTextPtr) {
        if (sGroupBindAllGroups) {
            snprintf(line, sizeof(line), "\xE5\x85\xA8\xE9\x83\xA8");
            mGroupNumEditTextPtr->setText(line);
        } else if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            snprintf(line, sizeof(line), "%d", sSelectedIrrGroupNo);
            mGroupNumEditTextPtr->setText(line);
        } else {
            mGroupNumEditTextPtr->setText("");
        }
    }

    if (mTextView5Ptr) {
        mTextView5Ptr->setText("");
    }
    if (mGroupBindNameEditTextPtr) {
        mGroupBindNameEditTextPtr->setText(sGroupBindAllGroups
                ? "\xE6\x89\x80\xE6\x9C\x89\xE9\x98\x80\xE7\xBB\x84"
                : getIrrGroupName(sSelectedIrrGroupNo));
    }
}

static void openGroupBindSelectionWindow() {
    if (!requireSelectedIrrGroup()) {
        return;
    }
    updateGroupBindEditTexts();
    syncGroupBindSelectionsFromCurrentGroup();
    updatePumpAdvanceSecEditText();
    refreshGroupBindListViews();

    hideAllPageWindows();
    if (mGroupBindValueWindowPtr) {
        mGroupBindValueWindowPtr->showWnd();
    }
}

static void openGroupBindWindow() {
    openGroupBindScopeWindow();
}

static void chooseCurrentIrrGroupFromDialog() {
    if (!sClearIrrWindowVisible) {
        return;
    }
    const int mode = sW2ChoiceDialogMode;
    closeClearIrrWindow();
    sW2ChoiceDialogMode = 0;
    if (mode == 2) {
        sGroupBindAllGroups = false;
        openGroupBindSelectionWindow();
    } else if (mode == 1) {
        clearSelectedIrrGroup();
    }
}

static void chooseAllIrrGroupsFromDialog() {
    if (!sClearIrrWindowVisible) {
        return;
    }
    const int mode = sW2ChoiceDialogMode;
    closeClearIrrWindow();
    sW2ChoiceDialogMode = 0;
    if (mode == 2) {
        sGroupBindAllGroups = true;
        openGroupBindSelectionWindow();
    } else if (mode == 1) {
        for (size_t i = 0; i < sIrrGroupNumbers.size(); ++i) {
            DeviceDataStore::clearIrrGroup(sIrrGroupNumbers[i]);
        }
        refreshDeviceListViews();
        resetIrrGroupSelection();
    }
}

static void hideGroupBindWindowOnly() {
    if (mGroupBindValueWindowPtr) {
        mGroupBindValueWindowPtr->hideWnd();
    }
    clearGroupBindSelections();
    refreshGroupBindListViews();
}

static void closeGroupBindWindow() {
    hideGroupBindWindowOnly();
    if (mw2set_windowPtr) {
        mw2set_windowPtr->showWnd();
        sW2SetWindowOpen = true;
    }
}

static void deleteSelectedIrrGroup() {
    if (!isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        closeGroupBindWindow();
        return;
    }

    DeviceDataStore::removeIrrGroup(sSelectedIrrGroupNo);
    std::vector<int>::iterator groupIt = std::find(
            sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), sSelectedIrrGroupNo);
    if (groupIt != sIrrGroupNumbers.end()) {
        sIrrGroupNumbers.erase(groupIt);
    }
    refreshDeviceListViews();
    resetIrrGroupSelection();
    closeGroupBindWindow();
}

static void deleteSelectedIrrGroupFromOverview() {
    if (!requireSelectedIrrGroup()) {
        return;
    }

    DeviceDataStore::removeIrrGroup(sSelectedIrrGroupNo);
    std::vector<int>::iterator groupIt = std::find(
            sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), sSelectedIrrGroupNo);
    if (groupIt != sIrrGroupNumbers.end()) {
        sIrrGroupNumbers.erase(groupIt);
    }
    refreshDeviceListViews();
    resetIrrGroupSelection();
}

static void bindSelectedDevicesToIrrGroup() {
    if (!isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return;
    }

    savePumpAdvanceSecondsFromEditText();

    bool changed = false;
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && (sGroupBindAllGroups
                    ? std::strcmp(data->arre, "*") == 0
                    : isDeviceBoundToIrrGroup(data, sSelectedIrrGroupNo))
                && (isGroupBindPumpDevice(data) || isSensorTextDevice(data))) {
            changed = (sGroupBindAllGroups
                    ? DeviceDataStore::unbindDeviceFromIrrGroup(i)
                    : DeviceDataStore::unbindDeviceFromIrrGroup(i, sSelectedIrrGroupNo))
                    || changed;
        }
    }
    for (size_t i = 0; i < sSelectedPumpDeviceIndexes.size(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(sSelectedPumpDeviceIndexes[i]);
        if (!sGroupBindAllGroups && data && std::strcmp(data->arre, "*") == 0) {
            continue;
        }
        changed = (sGroupBindAllGroups
                    ? DeviceDataStore::bindDeviceToAllIrrGroups(sSelectedPumpDeviceIndexes[i])
                    : DeviceDataStore::bindDeviceToIrrGroup(sSelectedPumpDeviceIndexes[i], sSelectedIrrGroupNo)) || changed;
    }
    for (size_t i = 0; i < sSelectedSensorDeviceIndexes.size(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(sSelectedSensorDeviceIndexes[i]);
        if (!sGroupBindAllGroups && data && std::strcmp(data->arre, "*") == 0) {
            continue;
        }
        changed = (sGroupBindAllGroups
                    ? DeviceDataStore::bindDeviceToAllIrrGroups(sSelectedSensorDeviceIndexes[i])
                    : DeviceDataStore::bindDeviceToIrrGroup(sSelectedSensorDeviceIndexes[i], sSelectedIrrGroupNo)) || changed;
    }

    if (changed) {
        refreshDeviceListViews();
        refreshChangeIrrListView();
    }
    closeGroupBindWindow();
}

static int getGroupBindDeviceListItemCount(bool pump) {
    return getFilteredDeviceCount(pump);
}

static void obtainGroupBindDeviceListItemData(ZKListView::ZKListItem *pListItem, int index, bool pump) {
    const int deviceIndex = getFilteredDeviceIndex(pump, index);
    const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
    if (!pListItem || !data) {
        return;
    }

    pListItem->setText(data->name);
    pListItem->setSelected(
            pump
                    ? containsSelectedDevice(sSelectedPumpDeviceIndexes, deviceIndex)
                    : containsSelectedDevice(sSelectedSensorDeviceIndexes, deviceIndex));
}

static void onGroupBindDeviceListItemClick(int index, bool pump) {
    const int deviceIndex = getFilteredDeviceIndex(pump, index);
    if (deviceIndex < 0) {
        return;
    }

    if (pump) {
        if (containsSelectedDevice(sSelectedPumpDeviceIndexes, deviceIndex)) {
            removeSelectedDevice(sSelectedPumpDeviceIndexes, deviceIndex);
        } else {
            addSelectedDevice(sSelectedPumpDeviceIndexes, deviceIndex);
        }
    } else {
        if (containsSelectedDevice(sSelectedSensorDeviceIndexes, deviceIndex)) {
            removeSelectedDevice(sSelectedSensorDeviceIndexes, deviceIndex);
        } else {
            addSelectedDevice(sSelectedSensorDeviceIndexes, deviceIndex);
        }
    }
    updateGroupBindSelectionEditTexts();
    refreshGroupBindListViews();
}

static const char* getW2SelectedDeviceType() {
    return sW2SelectedTypeIndex == 0 ? W2_DEVICE_TYPE_VALVE : W2_DEVICE_TYPE_SENSOR;
}



static int getW2DefaultAddress() {
    return CUSTOM_DEVICE_START_ID + DeviceDataStore::getCustomDeviceCount();
}

static void hideW2SetWindowOnly() {
    hideW2AddDeviceWindowOnly();
    closeIrrCapacityWindow(false);
    closeClearIrrWindow();
    finishW2AutoAssignPreview(false);
    if (mw2set_windowPtr) {
        mw2set_windowPtr->hideWnd();
    }
    sW2SetWindowOpen = false;
    sW2EditingIndex = -1;
    sW2AddingDevice = false;
    sW2CurrentAddress = 0;
    resetIrrGroupSelection();
}

static void closeW2SetWindow() {
    hideW2SetWindowOnly();
    if (mButton1Ptr) mButton1Ptr->setSelected(false);
    if (mButton2Ptr) mButton2Ptr->setSelected(true);
    if (mWindow2Ptr) {
        mWindow2Ptr->showWnd();
    }
}

static void openW2SetWindow(int index) {
    sW2AddingDevice = DeviceDataStore::isEmptyRow(index);
    sW2EditingIndex = sW2AddingDevice ? -1 : index;
    sSelectedIrrGroupNo = -1;

    int address = getW2DefaultAddress();
    const char* name = "";
    const char* type = W2_DEVICE_TYPE_VALVE;

    if (!sW2AddingDevice) {
        const SDATA* data = DeviceDataStore::getDevice(index);
        if (!data || !DeviceDataStore::isCustomDevice(index)) {
            return;
        }
        address = data->address;
        name = data->name;
        type = data->type;
        const int boundGroupNo = atoi(data->arre);
        if (isValidIrrGroupNo(boundGroupNo)) {
            sSelectedIrrGroupNo = boundGroupNo;
        }
    }

    sW2SelectedTypeIndex = ((std::strcmp(type, W2_DEVICE_TYPE_SENSOR) == 0)
            || (std::strcmp(type, "传感器") == 0)) ? 1 : 0;

    updateW2AddressDisplay(address);
    if (mW2_NameEditTextPtr) {
        mW2_NameEditTextPtr->setText(sW2AddingDevice ? getW2SelectedDeviceType() : name);
    }
    hideAllPageWindows();
    if (mw2set_windowPtr) {
        mw2set_windowPtr->showWnd();
    }
    sW2SetWindowOpen = true;
    updateClearIrrButtonText();
    refreshChangeIrrListView();

}

static void saveW2SetWindow() {
    int address = 0;
    std::string nameText;

    address = sW2CurrentAddress;
    if (mW2_NameEditTextPtr) {
        nameText = mW2_NameEditTextPtr->getText();
    }

    if (isValidIrrGroupNo(sSelectedIrrGroupNo)
            && !canAssignCurrentW2ValveToIrrGroup(sSelectedIrrGroupNo, true)) {
        return;
    }

    bool changed = false;
    if (sW2AddingDevice) {
        changed = DeviceDataStore::addDevice(address, nameText.c_str(), getW2SelectedDeviceType());
        if (changed && isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            const int addedIndex = DeviceDataStore::getDeviceCount() - 1;
            DeviceDataStore::bindDeviceToIrrGroup(addedIndex, sSelectedIrrGroupNo);
        }
    } else if (DeviceDataStore::isCustomDevice(sW2EditingIndex)) {
        changed = DeviceDataStore::updateDevice(sW2EditingIndex, address, nameText.c_str(), getW2SelectedDeviceType());
        if (changed && isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            DeviceDataStore::bindDeviceToIrrGroup(sW2EditingIndex, sSelectedIrrGroupNo);
        }
    } else if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        changed = DeviceDataStore::bindDeviceAddressToIrrGroup(address, sSelectedIrrGroupNo);
    }

    const bool addedDevice = changed && sW2AddingDevice;
    if (changed) {
        refreshDeviceListViews();
        refreshChangeIrrListView();
    }
    finishW2AutoAssignPreview(true);
    closeW2SetWindow();
    if (addedDevice) {
        showDeviceListEmptyRow();
    }
}

static void deleteW2SetWindowDevice() {
    if (!sW2AddingDevice && DeviceDataStore::deleteDevice(sW2EditingIndex)) {
        refreshDeviceListViews();
    }
    closeW2SetWindow();
}

static void setListSubItemText(ZKListView::ZKListItem *pListItem, int id, const char* text) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* subItem = pListItem->findSubItemByID(id);
    if (subItem) {
        subItem->setText(text ? text : "");
    }
}

static void setListSubItemAlignment(ZKListView::ZKListItem *pListItem, int id, ZKTextView::EAlignH h, ZKTextView::EAlignV v) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* subItem = pListItem->findSubItemByID(id);
    if (subItem) {
        subItem->setAlignment(h, v);
    }
}

static void setListSubItemVisible(ZKListView::ZKListItem *pListItem, int id, bool visible) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* subItem = pListItem->findSubItemByID(id);
    if (subItem) {
        subItem->setVisible(visible);
    }
}

static void setListSubItemPosition(ZKListView::ZKListItem *pListItem, int id, const LayoutPosition &position) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* subItem = pListItem->findSubItemByID(id);
    if (subItem) {
        subItem->setPosition(position);
    }
}

static int getPage2DeviceListItemCount(const ZKListView *pListView) {
    return DeviceDataStore::getDeviceListRowCount();
}

static void obtainPage2DeviceListItemData(ZKListView *pListView,
                                          ZKListView::ZKListItem *pListItem,
                                          int index) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* addressItem = pListItem->findSubItemByID(ID_MAIN_AddressSubItem);
    ZKListView::ZKListSubItem* nameItem = pListItem->findSubItemByID(ID_MAIN_NameSubItem);
    ZKListView::ZKListSubItem* typeItem = pListItem->findSubItemByID(ID_MAIN_TypeSubItem);
    ZKListView::ZKListSubItem* arreItem = pListItem->findSubItemByID(ID_MAIN_ArreSubItem);
    ZKListView::ZKListSubItem* statusItem = pListItem->findSubItemByID(ID_MAIN_StatusSubItem);
    const bool isEmptyRow = DeviceDataStore::isEmptyRow(index);
    const bool isEditableDevice = DeviceDataStore::isCustomDevice(index);

    if (addressItem) addressItem->setTouchable(isEditableDevice);
    if (nameItem) nameItem->setTouchable(isEmptyRow || isEditableDevice);
    if (typeItem) typeItem->setTouchable(isEditableDevice);
    if (arreItem) arreItem->setTouchable(isEditableDevice);
    if (statusItem) statusItem->setTouchable(isEmptyRow || isEditableDevice);

    if (isEmptyRow) {
        const bool discoveryRunning = sPage2CachedDiscoveryRunning;
        if (addressItem) addressItem->setText("");
        if (nameItem) nameItem->setTextColor(static_cast<int>(0xFF168BFFU));
        if (nameItem) nameItem->setText("点击添加");
        if (typeItem) typeItem->setText("");
        if (arreItem) arreItem->setText("");
        if (statusItem) statusItem->setTextColor(static_cast<int>(0xFF168BFFU));
        if (statusItem) statusItem->setText(discoveryRunning ? "同步中" : "同步");
        return;
    }

    const SDATA* data = DeviceDataStore::getDevice(index);
    if (!data) {
        return;
    }

    char addressBuf[16] = {0};
    snprintf(addressBuf, sizeof(addressBuf), "%d", data->address);

    if (addressItem) addressItem->setText(addressBuf);
    if (nameItem) {
        nameItem->setTextColor(static_cast<int>(0xFF000000U));
        nameItem->setText(data->name);
    }
    if (typeItem) typeItem->setText(data->type);
    if (arreItem) arreItem->setText(data->arre);
    if (statusItem) {
        statusItem->setText(data->status);
        statusItem->setTextColor(
            data->connected ? static_cast<int>(0xFF248A3DU)
                            : static_cast<int>(0xFF737A84U));
    }
}

static void onPage2DeviceListItemClick(ZKListView *pListView, int index, int id) {
    if (isWindow5DeviceDiscoveryRunning()) {
        return;
    }

    if (DeviceDataStore::isEmptyRow(index)) {
        if (id == ID_MAIN_StatusSubItem) {
            if (requestWindow5DeviceDiscovery()) {
                startPage2DeviceDiscoveryTip();
            }
            return;
        }

        if (id == ID_MAIN_NameSubItem) {
            openW2AddDeviceWindow();
        }
        return;
    }

    if (DeviceDataStore::isCustomDevice(index)) {
        openW2SetWindow(index);
    }
}

static int getPage2DeviceTipListItemCount(const ZKListView *pListView) {
    return 1;
}

static void obtainPage2DeviceTipListItemData(ZKListView *pListView,
                                             ZKListView::ZKListItem *pListItem,
                                             int index) {
    setListSubItemText(pListItem, ID_MAIN_AddressTipSubItem, "地址");
    setListSubItemText(pListItem, ID_MAIN_NameTipSubItem, "名称");
    setListSubItemText(pListItem, ID_MAIN_TypeTipSubItem, "类型");
    setListSubItemText(pListItem, ID_MAIN_ArreTipSubItem, "阀组编号");
    setListSubItemText(pListItem, ID_MAIN_StatusTipSubItem, "状态");
}

static void onPage2DeviceTipListItemClick(ZKListView *pListView, int index, int id) {
}

static void onPage2Show() {
    sPage2Active = true;
    sW2TipCheckedThisVisit = false;
    hideW2TipWindowOnly();
    refreshDeviceListViews();
}

static void onPage2Hide() {
    sPage2Active = false;
    hideGroupBindWindowOnly();
    hideW2AddDeviceWindowOnly();
    hideW2SetWindowOnly();
    hideW2TipWindowOnly();
    hideW2ActionTipWindowOnly();
    closeGroupRenameWindow();
}
