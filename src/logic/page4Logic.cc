// Page4 logic.

static void refreshWindow4ListViews() {
    if (mDeviceTestTipsListViewPtr) {
        mDeviceTestTipsListViewPtr->refreshListView();
    }
    if (mDeviceTestValueListViewPtr) {
        mDeviceTestValueListViewPtr->refreshListView();
    }
    if (mGroupTestTipsListViewPtr) {
        mGroupTestTipsListViewPtr->refreshListView();
    }
    if (mGroupTestValueListViewPtr) {
        mGroupTestValueListViewPtr->refreshListView();
    }
    if (mSenserTestTipsListViewPtr) {
        mSenserTestTipsListViewPtr->refreshListView();
    }
    if (mSenserTestValueListViewPtr) {
        mSenserTestValueListViewPtr->refreshListView();
    }
}

static bool sWindow4RoundIrrigationEnabled = false;
static bool sWindow4RoundIrrigationTipVisible = false;
static bool sWindow4RoundIrrigationConfirmMode = false;
static bool sWindow4RoundIrrigationUpdatingEditTexts = false;
static int sWindow4RoundIrrigationMinute = 0;
static int sWindow4RoundIrrigationSecond = 0;
static int sWindow4RoundIrrigationCurrentGroup = 0;
static int sWindow4RoundIrrigationPendingGroup = 0;
static long long sWindow4RoundIrrigationCloseAtMs = 0;
static long long sWindow4RoundIrrigationSwitchAtMs = 0;
static time_t sWindow4RoundIrrigationGroupEndTime = 0;
static bool sWindow4RoundIrrigationWaitingOpen = false;
static const long long kWindow4ValveReplyPollMs = 10LL;
static const long long kWindow4CloseActionReserveMs = 1000LL;
static const long long kWindow4ValveRspWaitLoops = 350LL;

static bool isWindow4OutputDevice(const SDATA* data);
static int getWindow4OutputDeviceIndex(int outputIndex);

static int getWindow4DeviceCount() {
    int count = 0;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (isWindow4OutputDevice(data)) {
            ++count;
        }
    }
    return count;
}

static int getWindow4GroupCount() {
    return static_cast<int>(sIrrGroupNumbers.size());
}

static int getWindow4GroupListRowCount() {
    const int groupCount = getWindow4GroupCount();
    return groupCount < 5 ? 5 : groupCount;
}

static bool isWindow4OutputDevice(const SDATA* data) {
    if (!data) {
        return false;
    }
    return (std::strcmp(data->type, "水泵") == 0) ||
           (std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0);
}

static int getWindow4OutputDeviceIndex(int outputIndex) {
    int count = 0;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!isWindow4OutputDevice(data)) {
            continue;
        }
        if (count == outputIndex) {
            return i;
        }
        ++count;
    }
    return -1;
}

static int clampWindow4RoundIrrigationTimeValue(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 59) {
        return 59;
    }
    return value;
}

static int parseWindow4RoundIrrigationTimeText(const std::string &text,
                                               int defaultValue) {
    if (text.empty()) {
        return defaultValue;
    }
    return clampWindow4RoundIrrigationTimeValue(atoi(text.c_str()));
}

static void updateWindow4RoundIrrigationEditTexts() {
    sWindow4RoundIrrigationUpdatingEditTexts = true;
    if (mWindow4RoundMinEditTextPtr) {
        mWindow4RoundMinEditTextPtr->setText(sWindow4RoundIrrigationMinute);
    }
    if (mWindow4RoundSecEditTextPtr) {
        mWindow4RoundSecEditTextPtr->setText(sWindow4RoundIrrigationSecond);
    }
    sWindow4RoundIrrigationUpdatingEditTexts = false;
}

static void updateWindow4RoundIrrigationToggleButton() {
    if (mWindow4RoundIrrigationToggleButtonPtr) {
        mWindow4RoundIrrigationToggleButtonPtr->setSelected(sWindow4RoundIrrigationEnabled);
    }
}

static int getWindow4RoundIrrigationDurationSeconds() {
    return sWindow4RoundIrrigationMinute * 60 + sWindow4RoundIrrigationSecond;
}

static long long getWindow4RoundIrrigationDurationMs() {
    return static_cast<long long>(getWindow4RoundIrrigationDurationSeconds()) * 1000LL;
}

static bool isWindow4RoundIrrigationEnabled() {
    return sWindow4RoundIrrigationEnabled;
}

static int findWindow4RoundIrrigationGroupIndex(const std::vector<int> &groups,
                                                int groupNo) {
    for (int i = 0; i < static_cast<int>(groups.size()); ++i) {
        if (groups[i] == groupNo) {
            return i;
        }
    }
    return -1;
}

static long long getWindow4RoundIrrigationCloseLeadMs(int groupNo,
                                                      long long durationMs) {
    const int valveCount = getIrrGroupValveCount(groupNo);
    if (valveCount <= 0 || durationMs <= 0) {
        return 0;
    }

    long long leadMs = static_cast<long long>(valveCount) *
            kWindow4ValveRspWaitLoops * kWindow4ValveReplyPollMs;
    leadMs += kWindow4CloseActionReserveMs;
    if (leadMs > durationMs) {
        return durationMs;
    }
    return leadMs;
}

static long long getWindow4RoundIrrigationNowMs() {
    long long nowMs = getWindow5NowMs();
    if (nowMs <= 0) {
        nowMs = static_cast<long long>(time(NULL)) * 1000LL;
    }
    return nowMs;
}

static void scheduleWindow4RoundIrrigationGroup(int groupNo, long long nowMs) {
    const long long durationMs = getWindow4RoundIrrigationDurationMs();
    const long long closeLeadMs =
            getWindow4RoundIrrigationCloseLeadMs(groupNo, durationMs);
    sWindow4RoundIrrigationCloseAtMs = nowMs + durationMs - closeLeadMs;
    sWindow4RoundIrrigationSwitchAtMs = nowMs + durationMs;
    sWindow4RoundIrrigationGroupEndTime =
            time(NULL) + static_cast<time_t>(getWindow4RoundIrrigationDurationSeconds());
}

static bool isWindow4BlankGroupRow(int index) {
    return index >= getWindow4GroupCount();
}

static void handleWindow4RoundMinEditTextChanged(const std::string &text) {
    if (sWindow4RoundIrrigationUpdatingEditTexts) {
        return;
    }
    sWindow4RoundIrrigationMinute =
            parseWindow4RoundIrrigationTimeText(text, sWindow4RoundIrrigationMinute);
    updateWindow4RoundIrrigationEditTexts();
    refreshWindow4ListViews();
}

static void handleWindow4RoundSecEditTextChanged(const std::string &text) {
    if (sWindow4RoundIrrigationUpdatingEditTexts) {
        return;
    }
    sWindow4RoundIrrigationSecond =
            parseWindow4RoundIrrigationTimeText(text, sWindow4RoundIrrigationSecond);
    updateWindow4RoundIrrigationEditTexts();
    refreshWindow4ListViews();
}

static bool isWindow4GroupHasValve(int groupNo) {
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

static std::vector<int> collectWindow4ValidValveGroups() {
    std::vector<int> groups;
    const int count = getWindow4GroupCount();
    for (int index = 0; index < count; ++index) {
        const int groupNo = index + 1;
        if (isWindow4GroupHasValve(groupNo)) {
            groups.push_back(groupNo);
        }
    }
    return groups;
}

static int getWindow4ValidValveGroupCount() {
    return static_cast<int>(collectWindow4ValidValveGroups().size());
}

static int getWindow4SensorCount() {
    int count = 0;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (isSensorTextDevice(data)) {
            ++count;
        }
    }
    return count;
}

static int getWindow4SensorDeviceIndex(int sensorIndex) {
    int count = 0;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!isSensorTextDevice(data)) {
            continue;
        }
        if (count == sensorIndex) {
            return i;
        }
        ++count;
    }
    return -1;
}

static bool isWindow4GroupActionOn(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return false;
    }

    bool hasValve = false;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || (std::strcmp(data->type, "电磁阀") != 0) ||
            !DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            continue;
        }
        hasValve = true;
        if (!data->connected || !data->stateKnown || !data->state) {
            return false;
        }
    }
    return hasValve;
}

static void hideWindow4RoundIrrigationTipOnly() {
    if (mWindow4RoundIrrigationConfirmWindowPtr) {
        mWindow4RoundIrrigationConfirmWindowPtr->hideWnd();
    }
    sWindow4RoundIrrigationTipVisible = false;
    sWindow4RoundIrrigationConfirmMode = false;
}

static bool hideWindow4RoundIrrigationTipIfVisible() {
    if (!sWindow4RoundIrrigationTipVisible) {
        return false;
    }
    if (sWindow4RoundIrrigationConfirmMode) {
        return false;
    }
    hideWindow4RoundIrrigationTipOnly();
    return true;
}

static void showWindow4RoundIrrigationTip(const char *pText, bool confirmMode) {
    if (mWindow4RoundIrrigationTipTextPtr) {
        mWindow4RoundIrrigationTipTextPtr->setText(pText ? pText : "");
    }
    if (mWindow4RoundIrrigationOkButtonPtr) {
        mWindow4RoundIrrigationOkButtonPtr->setVisible(confirmMode);
    }
    if (mWindow4RoundIrrigationCancelButtonPtr) {
        mWindow4RoundIrrigationCancelButtonPtr->setVisible(confirmMode);
    }
    if (mWindow4RoundIrrigationConfirmWindowPtr) {
        mWindow4RoundIrrigationConfirmWindowPtr->showWnd();
    }
    sWindow4RoundIrrigationTipVisible = true;
    sWindow4RoundIrrigationConfirmMode = confirmMode;
}

static void stopWindow4RoundIrrigation(bool closeCurrentGroup) {
    if (closeCurrentGroup && (sWindow4RoundIrrigationCurrentGroup > 0)) {
        (void)requestWindow5GroupValveState(sWindow4RoundIrrigationCurrentGroup, false);
    }
    sWindow4RoundIrrigationEnabled = false;
    sWindow4RoundIrrigationCurrentGroup = 0;
    sWindow4RoundIrrigationPendingGroup = 0;
    sWindow4RoundIrrigationCloseAtMs = 0;
    sWindow4RoundIrrigationSwitchAtMs = 0;
    sWindow4RoundIrrigationGroupEndTime = 0;
    sWindow4RoundIrrigationWaitingOpen = false;
    updateWindow4RoundIrrigationToggleButton();
}

static void startWindow4RoundIrrigation() {
    const int durationSeconds = getWindow4RoundIrrigationDurationSeconds();
    const std::vector<int> groups = collectWindow4ValidValveGroups();
    if ((durationSeconds <= 0) || groups.empty()) {
        stopWindow4RoundIrrigation(true);
        return;
    }

    if (!requestWindow5GroupValveState(groups[0], true)) {
        stopWindow4RoundIrrigation(false);
        return;
    }

    sWindow4RoundIrrigationEnabled = true;
    sWindow4RoundIrrigationCurrentGroup = groups[0];
    sWindow4RoundIrrigationPendingGroup = 0;
    sWindow4RoundIrrigationWaitingOpen = false;
    scheduleWindow4RoundIrrigationGroup(
            sWindow4RoundIrrigationCurrentGroup,
            getWindow4RoundIrrigationNowMs());
    updateWindow4RoundIrrigationToggleButton();
    refreshWindow8IrrigationState();
    refreshRunStatusValueText();
}

static void confirmWindow4RoundIrrigation() {
    if (!sWindow4RoundIrrigationConfirmMode) {
        hideWindow4RoundIrrigationTipOnly();
        return;
    }
    hideWindow4RoundIrrigationTipOnly();
    startWindow4RoundIrrigation();
    refreshWindow4ListViews();
}

static void requestWindow4RoundIrrigationEnable() {
    if (getWindow4RoundIrrigationDurationSeconds() <= 0) {
        showWindow4RoundIrrigationTip("请设置开启时间", false);
        return;
    }

    const int validGroupCount = getWindow4ValidValveGroupCount();
    char text[128] = {0};
    snprintf(text, sizeof(text),
             "有效阀组%d个\n即将启用轮流灌溉。", validGroupCount);
    showWindow4RoundIrrigationTip(text, true);
}

static void updateWindow4RoundIrrigation() {
    if (!sWindow4RoundIrrigationEnabled) {
        return;
    }

    const int durationSeconds = getWindow4RoundIrrigationDurationSeconds();
    const std::vector<int> groups = collectWindow4ValidValveGroups();
    if ((durationSeconds <= 0) || groups.empty()) {
        stopWindow4RoundIrrigation(true);
        refreshWindow4ListViews();
        return;
    }

    if (sWindow4RoundIrrigationWaitingOpen) {
        const long long nowMs = getWindow4RoundIrrigationNowMs();
        if (isWindow5ValveCommandBusy() ||
                (sWindow4RoundIrrigationSwitchAtMs > 0 &&
                 nowMs < sWindow4RoundIrrigationSwitchAtMs)) {
            return;
        }
        const int pendingIndex = findWindow4RoundIrrigationGroupIndex(
                groups, sWindow4RoundIrrigationPendingGroup);
        if (pendingIndex < 0) {
            stopWindow4RoundIrrigation(false);
            refreshWindow4ListViews();
            refreshWindow8IrrigationState();
            refreshRunStatusValueText();
            return;
        }
        if (!requestWindow5GroupValveState(sWindow4RoundIrrigationPendingGroup, true)) {
            return;
        }
        sWindow4RoundIrrigationCurrentGroup = sWindow4RoundIrrigationPendingGroup;
        sWindow4RoundIrrigationPendingGroup = 0;
        sWindow4RoundIrrigationWaitingOpen = false;
        scheduleWindow4RoundIrrigationGroup(
                sWindow4RoundIrrigationCurrentGroup, nowMs);
        updateWindow4RoundIrrigationToggleButton();
        refreshWindow4ListViews();
        refreshWindow8IrrigationState();
        refreshRunStatusValueText();
        return;
    }

    int currentIndex = findWindow4RoundIrrigationGroupIndex(
            groups, sWindow4RoundIrrigationCurrentGroup);
    if (currentIndex < 0) {
        if (!isWindow5ValveCommandBusy()) {
            sWindow4RoundIrrigationPendingGroup = groups[0];
            sWindow4RoundIrrigationWaitingOpen = true;
        }
        return;
    }

    const long long nowMs = getWindow4RoundIrrigationNowMs();
    if ((sWindow4RoundIrrigationCloseAtMs > 0) &&
            (nowMs < sWindow4RoundIrrigationCloseAtMs)) {
        return;
    }

    if (isWindow5ValveCommandBusy()) {
        return;
    }

    if (!requestWindow5GroupValveState(sWindow4RoundIrrigationCurrentGroup, false)) {
        return;
    }

    const int nextIndex = currentIndex + 1;
    if (nextIndex >= static_cast<int>(groups.size())) {
        sWindow4RoundIrrigationEnabled = false;
        sWindow4RoundIrrigationCurrentGroup = 0;
        sWindow4RoundIrrigationPendingGroup = 0;
        sWindow4RoundIrrigationCloseAtMs = 0;
        sWindow4RoundIrrigationSwitchAtMs = 0;
        sWindow4RoundIrrigationGroupEndTime = 0;
        sWindow4RoundIrrigationWaitingOpen = false;
        updateWindow4RoundIrrigationToggleButton();
        refreshWindow4ListViews();
        refreshWindow8IrrigationState();
        refreshRunStatusValueText();
        return;
    }

    sWindow4RoundIrrigationCurrentGroup = 0;
    sWindow4RoundIrrigationPendingGroup = groups[nextIndex];
    sWindow4RoundIrrigationWaitingOpen = true;
    updateWindow4RoundIrrigationToggleButton();
    refreshWindow4ListViews();
    refreshWindow8IrrigationState();
    refreshRunStatusValueText();
}

static bool getWindow4RoundIrrigationDisplayState(int *pCompletedGroup,
                                                  int *pRunningGroup,
                                                  int *pWaitingGroup,
                                                  int *pRemainingSeconds,
                                                  time_t *pStopTime) {
    if (!sWindow4RoundIrrigationEnabled) {
        return false;
    }

    const std::vector<int> groups = collectWindow4ValidValveGroups();
    if (groups.empty()) {
        return false;
    }

    const int displayGroup = sWindow4RoundIrrigationWaitingOpen ?
            sWindow4RoundIrrigationPendingGroup : sWindow4RoundIrrigationCurrentGroup;
    const int index = findWindow4RoundIrrigationGroupIndex(groups, displayGroup);
    if (index < 0) {
        return false;
    }

    if (pCompletedGroup) {
        *pCompletedGroup = (index > 0) ? groups[index - 1] : 0;
    }
    if (pRunningGroup) {
        *pRunningGroup = displayGroup;
    }
    if (pWaitingGroup) {
        *pWaitingGroup = (index + 1 < static_cast<int>(groups.size())) ?
                groups[index + 1] : 0;
    }

    const time_t now = time(NULL);
    int remainingSeconds = 0;
    if (sWindow4RoundIrrigationGroupEndTime > now) {
        remainingSeconds =
                static_cast<int>(sWindow4RoundIrrigationGroupEndTime - now);
    } else {
        remainingSeconds = 0;
    }
    if (pRemainingSeconds) {
        *pRemainingSeconds = remainingSeconds;
    }
    if (pStopTime) {
        *pStopTime = sWindow4RoundIrrigationGroupEndTime;
    }
    return true;
}

static void setWindow4DeviceValueItem(ZKListView::ZKListItem *pListItem, int index) {
    const int deviceIndex = getWindow4OutputDeviceIndex(index);
    const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
    if (!pListItem || !data) {
        return;
    }

    ZKListView::ZKListSubItem* numItem = pListItem->findSubItemByID(ID_MAIN_DeviceTestNumValueSubItem);
    ZKListView::ZKListSubItem* nameItem = pListItem->findSubItemByID(ID_MAIN_DeviceTestNameValueSubItem);
    ZKListView::ZKListSubItem* actionItem = pListItem->findSubItemByID(ID_MAIN_DeviceTestActionValueSubItem);
    if (numItem) {
        numItem->setText(data->address);
    }
    if (nameItem) {
        nameItem->setText(data->name);
    }
    if (actionItem) {
        actionItem->setSelected(data->state);
    }
}

static void setWindow4GroupValueItem(ZKListView::ZKListItem *pListItem, int index) {
    if (isWindow4BlankGroupRow(index)) {
        ZKListView::ZKListSubItem* numItem = pListItem ? pListItem->findSubItemByID(ID_MAIN_GroupTestNumValueSubItem) : NULL;
        ZKListView::ZKListSubItem* nameItem = pListItem ? pListItem->findSubItemByID(ID_MAIN_GroupTestNameValueSubItem) : NULL;
        ZKListView::ZKListSubItem* actionItem = pListItem ? pListItem->findSubItemByID(ID_MAIN_GroupTestActionValueSubItem) : NULL;
        if (numItem) numItem->setVisible(false);
        if (nameItem) nameItem->setVisible(false);
        if (actionItem) actionItem->setVisible(false);
        return;
    }

    const int groupNo = index + 1;
    if (!pListItem || !isValidIrrGroupNo(groupNo)) {
        return;
    }

    ZKListView::ZKListSubItem* numItem = pListItem->findSubItemByID(ID_MAIN_GroupTestNumValueSubItem);
    ZKListView::ZKListSubItem* nameItem = pListItem->findSubItemByID(ID_MAIN_GroupTestNameValueSubItem);
    ZKListView::ZKListSubItem* actionItem = pListItem->findSubItemByID(ID_MAIN_GroupTestActionValueSubItem);
    if (numItem) {
        numItem->setVisible(true);
        numItem->setText(groupNo);
    }
    if (nameItem) {
        nameItem->setVisible(true);
        nameItem->setText(DeviceDataStore::getIrrGroupName(groupNo));
    }
    if (actionItem) {
        actionItem->setVisible(true);
        actionItem->setSelected(isWindow4GroupActionOn(groupNo));
    }
}

static void setWindow4SensorValueItem(ZKListView::ZKListItem *pListItem, int index) {
    const int deviceIndex = getWindow4SensorDeviceIndex(index);
    const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
    if (!pListItem || !data) {
        return;
    }

    ZKListView::ZKListSubItem* numItem = pListItem->findSubItemByID(ID_MAIN_SenserTestNumValueSubItem);
    ZKListView::ZKListSubItem* nameItem = pListItem->findSubItemByID(ID_MAIN_SenserTestNameValueSubItem);
    ZKListView::ZKListSubItem* valueItem = pListItem->findSubItemByID(ID_MAIN_SenserTestValueSubItem);
    if (numItem) {
        numItem->setText(data->address);
    }
    if (nameItem) {
        nameItem->setText(data->name);
    }
    if (valueItem) {
        valueItem->setText(data->status[0] != '\0' ? data->status : "-");
    }
}

static void onPage4Show() {
    updateWindow4RoundIrrigationEditTexts();
    updateWindow4RoundIrrigationToggleButton();
    refreshWindow4ListViews();
}

static void onPage4Hide() {
}
