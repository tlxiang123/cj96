#include "DeviceDataStore.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static const char* W2_DEVICE_TYPE_VALVE = "电磁阀";
static const char* W2_DEVICE_TYPE_SENSOR = "传感器";
static int sW2EditingIndex = -1;
static bool sW2AddingDevice = false;
static bool sW2SetWindowOpen = false;
static bool sW2TipWindowVisible = false;
static bool sW2TipCheckedThisVisit = false;
static int sW2SelectedTypeIndex = 0;
static int sSelectedIrrGroupNo = -1;
static int sIrrGroupRowCount = 5;
static bool sIrrEmptyItemLayoutCaptured = false;
static LayoutPosition sIrrNumSubItemPosition;
static LayoutPosition sIrrArrSubItemPosition;
static bool sDeviceEmptyItemLayoutCaptured = false;
static LayoutPosition sDeviceAddressSubItemPosition;
static LayoutPosition sDeviceNameSubItemPosition;
static LayoutPosition sDeviceArreSubItemPosition;
static LayoutPosition sDeviceStatusSubItemPosition;
static LayoutPosition sDeviceTypeSubItemPosition;
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
static const char* getIrrGroupName(int groupNo);
static void buildIrrGroupDisplayText(int groupNo, char* text, size_t size);
static void closeGroupBindWindow();
static void updateGroupBindSelectionEditTexts();
static bool isPumpDevice(const SDATA* data);
static bool isSensorBindDevice(const SDATA* data);
static bool isSensorTextDevice(const SDATA* data);
static void refreshRunTimeListView();
static bool isW2PreviewValveForGroup(int groupNo);
static int getW2PreviewAddress();
static int getW2DefaultAddress();

static void setW2TipText(const char* text);
static bool collectUngroupedValveAddresses(char* text, size_t size);

static void refreshDeviceListViews() {
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
        mWindow7Ptr,
        mw2set_windowPtr,
        mGroupBindValueWindowPtr,
    };

    for (int i = 0; i < static_cast<int>(sizeof(windows) / sizeof(windows[0])); ++i) {
        if (windows[i]) {
            windows[i]->hideWnd();
        }
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

static void hideW2TipWindowOnly() {
#if defined(ID_MAIN_W2TipWindow)
    if (mW2TipWindowPtr) {
        mW2TipWindowPtr->hideWnd();
    }
#endif
    sW2TipWindowVisible = false;
}

static bool hideW2TipWindowIfVisible() {
    if (!sW2TipWindowVisible) {
        return false;
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
    if (mChangeIrr_ListViewPtr && sIrrGroupRowCount > 0) {
        mChangeIrr_ListViewPtr->setSelection(sIrrGroupRowCount - 1);
    }
}

static void updateClearIrrButtonText() {
    if (mClearIrr_ButtonPtr) {
        mClearIrr_ButtonPtr->setText("清空阀组");
    }

    if (mIrrNum_TextViewPtr) {
        char text[32] = {0};
        if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            snprintf(text, sizeof(text), "%s", getIrrGroupName(sSelectedIrrGroupNo));
        } else {
            snprintf(text, sizeof(text), "-");
        }
        mIrrNum_TextViewPtr->setText(text);
    }
}

static bool isValidIrrGroupNo(int groupNo) {
    return groupNo > 0 && groupNo <= 128;
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
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) != 0) {
            continue;
        }

        if (isValidIrrGroupNo(atoi(data->arre))) {
            continue;
        }

        char addressText[16] = {0};
        snprintf(addressText, sizeof(addressText), "%d", data->address);
        appendTextPart(text, size, addressText, ", ");
    }

    return text[0] != '\0';
}

static bool showW2UngroupedValveTipIfNeeded() {
    char addresses[128] = {0};
    if (!collectUngroupedValveAddresses(addresses, sizeof(addresses))) {
        return false;
    }

    char tipText[192] = {0};
    snprintf(tipText, sizeof(tipText), "电磁阀[%s]未添加到阀组", addresses);
    setW2TipText(tipText);

#if defined(ID_MAIN_W2TipWindow)
    if (mW2TipWindowPtr) {
        mW2TipWindowPtr->showWnd();
        sW2TipWindowVisible = true;
        return true;
    }
#endif

    return false;
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
    if (mW2_AddressEditTextPtr) {
        const int address = atoi(mW2_AddressEditTextPtr->getText().c_str());
        if (address > 0) {
            return address;
        }
    }
    return getW2DefaultAddress();
}

static void buildIrrGroupDisplayText(int groupNo, char* text, size_t size) {
    if (!text || size == 0) {
        return;
    }

    text[0] = '\0';
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }

    char groupText[16] = {0};
    snprintf(groupText, sizeof(groupText), "%d", groupNo);

    char valveAddresses[96] = {0};
    char waterPumpAddresses[64] = {0};
    char sensorNames[160] = {0};
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (sW2SetWindowOpen && i == sW2EditingIndex && DeviceDataStore::isCustomDevice(i)) {
            continue;
        }
        if (!data || std::strcmp(data->arre, groupText) != 0) {
            continue;
        }

        if (std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0) {
            char addressText[16] = {0};
            snprintf(addressText, sizeof(addressText), "%d", data->address);
            appendTextPart(valveAddresses, sizeof(valveAddresses), addressText, ",");
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
        char addressText[16] = {0};
        snprintf(addressText, sizeof(addressText), "%d", getW2PreviewAddress());
        appendTextPart(valveAddresses, sizeof(valveAddresses), addressText, ",");
    }

    char summary[256] = {0};
    if (valveAddresses[0] != '\0') {
        char valveText[128] = {0};
        snprintf(valveText, sizeof(valveText), "电磁阀[%s]", valveAddresses);
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
        snprintf(text, size, "%s%s", getIrrGroupName(groupNo), summary);
    } else {
        snprintf(text, size, "%s     [空]", getIrrGroupName(groupNo));
    }
}

static void resetIrrGroupSelection() {
    sSelectedIrrGroupNo = -1;
    updateClearIrrButtonText();
    refreshChangeIrrListView();
}

static void selectIrrGroup(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }

    sSelectedIrrGroupNo = groupNo;
    updateClearIrrButtonText();
    refreshChangeIrrListView();
}

static int getChangeIrrListItemCount() {
    return sIrrGroupRowCount;
}

static bool isIrrGroupEmptyRow(int index) {
    return index < 0 || index >= sIrrGroupRowCount || index == sIrrGroupRowCount - 1;
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

    const int groupNo = index + 1;
    char numText[16] = {0};
    char nameText[256] = {0};
    snprintf(numText, sizeof(numText), "%d", groupNo);
    buildIrrGroupDisplayText(groupNo, nameText, sizeof(nameText));

    setListSubItemText(pListItem, numSubItemId, numText);
    setListSubItemText(pListItem, nameSubItemId, nameText);
    setListSubItemAlignment(pListItem, nameSubItemId, ZKTextView::E_ALIGN_H_LEFT, ZKTextView::E_ALIGN_V_CENTER);
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
    if (index < 0 || index >= sIrrGroupRowCount) {
        return;
    }

    if (index == sIrrGroupRowCount - 1) {
        if (sIrrGroupRowCount < 129) {
            ++sIrrGroupRowCount;
            refreshChangeIrrListView();
            showChangeIrrListEmptyRow();
        }
        return;
    }

    selectIrrGroup(index + 1);
}

static void clearSelectedIrrGroup() {
    if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        DeviceDataStore::clearIrrGroup(sSelectedIrrGroupNo);
    }
    refreshDeviceListViews();
    resetIrrGroupSelection();
}

static bool isPumpDevice(const SDATA* data) {
    return data && (std::strcmp(data->type, "水泵") == 0 || std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0);
}

static bool isFactorySelectableSensorDevice(const SDATA* data) {
    return data && (data->address == 6 || data->address == 8);
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
        if ((pump && isPumpDevice(data)) || (!pump && isSensorBindDevice(data))) {
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
        if ((pump && isPumpDevice(data)) || (!pump && isSensorBindDevice(data))) {
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

static void setSelectedDeviceText(ZKEditText* editText, const std::vector<int>& selectedIndexes) {
    if (!editText) {
        return;
    }

    char text[256] = {0};
    size_t offset = 0;
    for (size_t i = 0; i < selectedIndexes.size(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(selectedIndexes[i]);
        if (!data) {
            continue;
        }

        const int written = snprintf(
                text + offset,
                sizeof(text) - offset,
                "%s%d",
                (offset == 0) ? "" : ",",
                data->address);
        if (written <= 0 || static_cast<size_t>(written) >= sizeof(text) - offset) {
            break;
        }
        offset += static_cast<size_t>(written);
    }

    editText->setText(text);
}

static void updateGroupBindSelectionEditTexts() {
    setSelectedDeviceText(mAddPumpEditTextPtr, sSelectedPumpDeviceIndexes);
    setSelectedDeviceText(mAddSenserEditTextPtr, sSelectedSensorDeviceIndexes);
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

    char groupText[16] = {0};
    snprintf(groupText, sizeof(groupText), "%d", sSelectedIrrGroupNo);

    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || std::strcmp(data->arre, groupText) != 0) {
            continue;
        }

        if (isPumpDevice(data)) {
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
    if (mGroupNumEditTextPtr) {
        if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            mGroupNumEditTextPtr->setText(sSelectedIrrGroupNo);
        } else {
            mGroupNumEditTextPtr->setText("");
        }
    }

    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setText(getIrrGroupName(sSelectedIrrGroupNo));
    }
}

static void openGroupBindWindow() {
    updateGroupBindEditTexts();
    syncGroupBindSelectionsFromCurrentGroup();
    refreshGroupBindListViews();

    hideAllPageWindows();
    if (mGroupBindValueWindowPtr) {
        mGroupBindValueWindowPtr->showWnd();
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
    if (sIrrGroupRowCount > 1) {
        --sIrrGroupRowCount;
    }
    refreshDeviceListViews();
    resetIrrGroupSelection();
    closeGroupBindWindow();
}

static void bindSelectedDevicesToIrrGroup() {
    if (!isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return;
    }

    std::string groupNameText;
    if (mGroupNameEditTextPtr) {
        groupNameText = mGroupNameEditTextPtr->getText();
    }

    bool changed = DeviceDataStore::setIrrGroupName(sSelectedIrrGroupNo, groupNameText.c_str());
    changed = DeviceDataStore::clearIrrGroup(sSelectedIrrGroupNo) || changed;
    for (size_t i = 0; i < sSelectedPumpDeviceIndexes.size(); ++i) {
        changed = DeviceDataStore::bindDeviceToIrrGroup(sSelectedPumpDeviceIndexes[i], sSelectedIrrGroupNo) || changed;
    }
    for (size_t i = 0; i < sSelectedSensorDeviceIndexes.size(); ++i) {
        changed = DeviceDataStore::bindDeviceToIrrGroup(sSelectedSensorDeviceIndexes[i], sSelectedIrrGroupNo) || changed;
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

    char text[96] = {0};
    snprintf(text, sizeof(text), "%02d  %s", data->address, data->name);
    if (data->arre[0] != '\0' && std::strcmp(data->arre, "-") != 0) {
        size_t len = std::strlen(text);
        snprintf(text + len, sizeof(text) - len, "  %s", data->arre);
    }
    pListItem->setText(text);
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
    if (mw2set_windowPtr) {
        mw2set_windowPtr->hideWnd();
    }
    sW2SetWindowOpen = false;
    sW2EditingIndex = -1;
    sW2AddingDevice = false;
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
    }

    sW2SelectedTypeIndex = ((std::strcmp(type, W2_DEVICE_TYPE_SENSOR) == 0)
            || (std::strcmp(type, "传感器") == 0)) ? 1 : 0;

    if (mW2_AddressEditTextPtr) {
        mW2_AddressEditTextPtr->setText(address);
    }
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

    if (mW2_AddressEditTextPtr) {
        address = atoi(mW2_AddressEditTextPtr->getText().c_str());
    }
    if (mW2_NameEditTextPtr) {
        nameText = mW2_NameEditTextPtr->getText();
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

    if (changed) {
        refreshDeviceListViews();
        refreshChangeIrrListView();
        if (sW2AddingDevice) {
            showDeviceListEmptyRow();
        }
    }
    closeW2SetWindow();
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
    if (DeviceDataStore::isEmptyRow(index)) {
        //setDeviceListItemTextColor(pListItem, 0xFF000000);
        ZKListView::ZKListSubItem* nameSubItem = pListItem ? pListItem->findSubItemByID(ID_MAIN_NameSubItem) : NULL;
        ZKListView::ZKListSubItem* addressSubItem = pListItem ? pListItem->findSubItemByID(ID_MAIN_AddressSubItem) : NULL;
        ZKListView::ZKListSubItem* arreSubItem = pListItem ? pListItem->findSubItemByID(ID_MAIN_ArreSubItem) : NULL;
        ZKListView::ZKListSubItem* statusSubItem = pListItem ? pListItem->findSubItemByID(ID_MAIN_StatusSubItem) : NULL;
        ZKListView::ZKListSubItem* typeSubItem = pListItem ? pListItem->findSubItemByID(ID_MAIN_TypeSubItem) : NULL;
        if (!sDeviceEmptyItemLayoutCaptured && addressSubItem && nameSubItem && arreSubItem && statusSubItem && typeSubItem) {
            sDeviceAddressSubItemPosition = addressSubItem->getPosition();
            sDeviceNameSubItemPosition = nameSubItem->getPosition();
            sDeviceArreSubItemPosition = arreSubItem->getPosition();
            sDeviceStatusSubItemPosition = statusSubItem->getPosition();
            sDeviceTypeSubItemPosition = typeSubItem->getPosition();
            sDeviceEmptyItemLayoutCaptured = true;
        }
        setListSubItemText(pListItem, ID_MAIN_AddressSubItem, "");
        setListSubItemText(pListItem, ID_MAIN_NameSubItem, "点击添加");
        setListSubItemText(pListItem, ID_MAIN_ArreSubItem, "");
        setListSubItemText(pListItem, ID_MAIN_StatusSubItem, "");
        setListSubItemText(pListItem, ID_MAIN_TypeSubItem, "");
        setListSubItemAlignment(pListItem, ID_MAIN_NameSubItem, ZKTextView::E_ALIGN_H_CENTER, ZKTextView::E_ALIGN_V_CENTER);
        setListSubItemVisible(pListItem, ID_MAIN_AddressSubItem, false);
        setListSubItemVisible(pListItem, ID_MAIN_ArreSubItem, false);
        setListSubItemVisible(pListItem, ID_MAIN_StatusSubItem, false);
        setListSubItemVisible(pListItem, ID_MAIN_TypeSubItem, false);
        if (nameSubItem) {
            LayoutPosition lp = sDeviceEmptyItemLayoutCaptured ? sDeviceNameSubItemPosition : nameSubItem->getPosition();
            lp.mLeft = 0;
            lp.mWidth = pListItem->getPosition().mWidth;
            setListSubItemPosition(pListItem, ID_MAIN_NameSubItem, lp);
            setListSubItemVisible(pListItem, ID_MAIN_NameSubItem, true);
        }
        return;
    }

    const SDATA* data = DeviceDataStore::getDevice(index);
    if (!data) {
        return;
    }

    // setDeviceListItemTextColor(pListItem, DeviceDataStore::isDefaultDevice(index) ? 0xFF999999 : 0xFF000000);

    char addressBuf[16] = {0};
    snprintf(addressBuf, sizeof(addressBuf), "%d", data->address);
    setListSubItemText(pListItem, ID_MAIN_AddressSubItem, addressBuf);
    setListSubItemText(pListItem, ID_MAIN_NameSubItem, data->name);
    setListSubItemText(pListItem, ID_MAIN_TypeSubItem, data->type);
    setListSubItemText(pListItem, ID_MAIN_ArreSubItem, data->arre);
    setListSubItemAlignment(pListItem, ID_MAIN_NameSubItem, ZKTextView::E_ALIGN_H_LEFT, ZKTextView::E_ALIGN_V_CENTER);
    setListSubItemVisible(pListItem, ID_MAIN_AddressSubItem, true);
    setListSubItemVisible(pListItem, ID_MAIN_NameSubItem, true);
    setListSubItemVisible(pListItem, ID_MAIN_TypeSubItem, true);
    setListSubItemVisible(pListItem, ID_MAIN_ArreSubItem, true);
    setListSubItemVisible(pListItem, ID_MAIN_StatusSubItem, true);
    if (sDeviceEmptyItemLayoutCaptured) {
        setListSubItemPosition(pListItem, ID_MAIN_AddressSubItem, sDeviceAddressSubItemPosition);
        setListSubItemPosition(pListItem, ID_MAIN_NameSubItem, sDeviceNameSubItemPosition);
        setListSubItemPosition(pListItem, ID_MAIN_TypeSubItem, sDeviceTypeSubItemPosition);
        setListSubItemPosition(pListItem, ID_MAIN_ArreSubItem, sDeviceArreSubItemPosition);
        setListSubItemPosition(pListItem, ID_MAIN_StatusSubItem, sDeviceStatusSubItemPosition);
    }

    ZKListView::ZKListSubItem* statusItem = pListItem->findSubItemByID(ID_MAIN_StatusSubItem);
    if (statusItem) {
        statusItem->setText(data->status);
        statusItem->setSelected(data->state);
    }
}

static void onPage2DeviceListItemClick(ZKListView *pListView, int index, int id) {
    bool changed = false;

    if (DeviceDataStore::isEmptyRow(index)) {
        openW2SetWindow(index);
        return;
    } else if (DeviceDataStore::isCustomDevice(index)) {
        openW2SetWindow(index);
        return;
    } else if (DeviceDataStore::isDefaultDevice(index)) {
        changed = DeviceDataStore::toggleDeviceState(index);
    }

    if (changed) {
        refreshDeviceListViews();
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
    sW2TipCheckedThisVisit = false;
    hideW2TipWindowOnly();
    refreshDeviceListViews();
}

static void onPage2Hide() {
    hideGroupBindWindowOnly();
    hideW2SetWindowOnly();
    hideW2TipWindowOnly();
}
