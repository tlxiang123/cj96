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

static int getWindow4DeviceCount() {
    return DeviceDataStore::getDeviceCount();
}

static int getWindow4GroupCount() {
    return static_cast<int>(sIrrGroupNumbers.size());
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
            (atoi(data->arre) != groupNo)) {
            continue;
        }
        hasValve = true;
        if (!data->connected || !data->stateKnown || !data->state) {
            return false;
        }
    }
    return hasValve;
}

static void setWindow4DeviceValueItem(ZKListView::ZKListItem *pListItem, int index) {
    const SDATA* data = DeviceDataStore::getDevice(index);
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
    const int groupNo = index + 1;
    if (!pListItem || !isValidIrrGroupNo(groupNo)) {
        return;
    }

    ZKListView::ZKListSubItem* numItem = pListItem->findSubItemByID(ID_MAIN_GroupTestNumValueSubItem);
    ZKListView::ZKListSubItem* nameItem = pListItem->findSubItemByID(ID_MAIN_GroupTestNameValueSubItem);
    ZKListView::ZKListSubItem* actionItem = pListItem->findSubItemByID(ID_MAIN_GroupTestActionValueSubItem);
    if (numItem) {
        numItem->setText(groupNo);
    }
    if (nameItem) {
        nameItem->setText(DeviceDataStore::getIrrGroupName(groupNo));
    }
    if (actionItem) {
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
    refreshWindow4ListViews();
}

static void onPage4Hide() {
}
