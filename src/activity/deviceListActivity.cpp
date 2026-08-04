#include "deviceListActivity.h"

#include "../logic/DeviceDataStore.h"
#include "entry/EasyUIContext.h"
#include "logic/globalScreenshotLogic.cc"

#include <cstdio>

#define ID_DEVICELIST_DeviceListView 80001
#define ID_DEVICELIST_DeviceTipListView 80002
#define ID_DEVICELIST_AddressSubItem 24001
#define ID_DEVICELIST_NameSubItem 24002
#define ID_DEVICELIST_TypeSubItem 24003
#define ID_DEVICELIST_ArreSubItem 24004
#define ID_DEVICELIST_StatusSubItem 24005
#define ID_DEVICELIST_GlobalScreenshotButton 20120

#define ID_DEVICELIST_Window1 110001
#define ID_DEVICELIST_Window2 110002
#define ID_DEVICELIST_Window3 110003
#define ID_DEVICELIST_Window4 110004
#define ID_DEVICELIST_Window5 110005

#define ID_DEVICELIST_Button1 20001
#define ID_DEVICELIST_Button2 20002
#define ID_DEVICELIST_Button3 20003
#define ID_DEVICELIST_Button4 20004
#define ID_DEVICELIST_Button5 20005
#define ID_DEVICELIST_Button7 20007
#define ID_DEVICELIST_Button8 20008
#define ID_DEVICELIST_Button9 20009

REGISTER_ACTIVITY(deviceListActivity);

deviceListActivity::deviceListActivity()
    : mDeviceListViewPtr(NULL), mDeviceTipListViewPtr(NULL), mButton2Ptr(NULL) {
}

deviceListActivity::~deviceListActivity() {
    mDeviceListViewPtr = NULL;
    mDeviceTipListViewPtr = NULL;
    mButton2Ptr = NULL;
}

const char* deviceListActivity::getAppName() const {
    return "deviceList.ftu";
}

void deviceListActivity::onCreate() {
    Activity::onCreate();

    // 隐藏所有其他 Window，只显示 Window2
    ZKBase* window1 = findControlByID(ID_DEVICELIST_Window1);
    ZKBase* window2 = findControlByID(ID_DEVICELIST_Window2);
    ZKBase* window3 = findControlByID(ID_DEVICELIST_Window3);
    ZKBase* window4 = findControlByID(ID_DEVICELIST_Window4);
    ZKBase* window5 = findControlByID(ID_DEVICELIST_Window5);

    if (window1) window1->setVisible(false);
    if (window2) window2->setVisible(true);
    if (window3) window3->setVisible(false);
    if (window4) window4->setVisible(false);
    if (window5) window5->setVisible(false);

    mDeviceListViewPtr = static_cast<ZKListView*>(
            findControlByID(ID_DEVICELIST_DeviceListView));
    mDeviceTipListViewPtr = static_cast<ZKListView*>(
            findControlByID(ID_DEVICELIST_DeviceTipListView));
    mButton2Ptr = static_cast<ZKButton*>(findControlByID(ID_DEVICELIST_Button2));

    if (mDeviceListViewPtr) {
        mDeviceListViewPtr->setListAdapter(this);
        mDeviceListViewPtr->setItemClickListener(this);
        mDeviceListViewPtr->refreshListView();
    }
    if (mDeviceTipListViewPtr) {
        mDeviceTipListViewPtr->setListAdapter(this);
        mDeviceTipListViewPtr->refreshListView();
    }
    if (mButton2Ptr) {
        mButton2Ptr->setSelected(true);
    }
}

void deviceListActivity::onClick(ZKBase *pBase) {
    if (!pBase) {
        return;
    }

    switch (pBase->getID()) {
    case ID_DEVICELIST_GlobalScreenshotButton:
        onButtonClick_GlobalScreenshotButton(static_cast<ZKButton*>(pBase));
        return;
    case ID_DEVICELIST_Button2:
        return;
    case ID_DEVICELIST_Button9:
        EASYUICONTEXT->openActivity("page1topsetActivity");
        return;
    case ID_DEVICELIST_Button1:
    case ID_DEVICELIST_Button3:
    case ID_DEVICELIST_Button4:
    case ID_DEVICELIST_Button5:
    case ID_DEVICELIST_Button7:
    case ID_DEVICELIST_Button8:
        EASYUICONTEXT->goBack();
        return;
    default:
        break;
    }
    Activity::onClick(pBase);
}

int deviceListActivity::getListItemCount(const ZKListView *pListView) const {
    if (pListView && pListView->getID() == ID_DEVICELIST_DeviceTipListView) {
        return 1;
    }
    return DeviceDataStore::getDeviceListRowCount();
}

void deviceListActivity::obtainListItemData(ZKListView *pListView,
                                            ZKListView::ZKListItem *pListItem,
                                            int index) {
    if (!pListItem) {
        return;
    }
    if (pListView && pListView->getID() == ID_DEVICELIST_DeviceTipListView) {
        return;
    }

    ZKListView::ZKListSubItem* addressItem =
            pListItem->findSubItemByID(ID_DEVICELIST_AddressSubItem);
    ZKListView::ZKListSubItem* nameItem =
            pListItem->findSubItemByID(ID_DEVICELIST_NameSubItem);
    ZKListView::ZKListSubItem* typeItem =
            pListItem->findSubItemByID(ID_DEVICELIST_TypeSubItem);
    ZKListView::ZKListSubItem* arreItem =
            pListItem->findSubItemByID(ID_DEVICELIST_ArreSubItem);
    ZKListView::ZKListSubItem* statusItem =
            pListItem->findSubItemByID(ID_DEVICELIST_StatusSubItem);

    if (DeviceDataStore::isEmptyRow(index)) {
        if (addressItem) addressItem->setText("");
        if (nameItem) nameItem->setText("点击添加");
        if (typeItem) typeItem->setText("");
        if (arreItem) arreItem->setText("");
        if (statusItem) statusItem->setText("同步");
        return;
    }

    const SDATA* data = DeviceDataStore::getDevice(index);
    if (!data) {
        return;
    }

    char address[16] = {0};
    snprintf(address, sizeof(address), "%d", data->address);

    if (addressItem) addressItem->setText(address);
    if (nameItem) nameItem->setText(data->name);
    if (typeItem) typeItem->setText(data->type);
    if (arreItem) arreItem->setText(data->arre);
    if (statusItem) {
        statusItem->setText(data->status);
        statusItem->setTextColor(data->connected
                ? static_cast<int>(0xFF248A3DU)
                : static_cast<int>(0xFF737A84U));
    }
}

void deviceListActivity::onItemClick(ZKListView *pListView,
                                     int index,
                                     int subItemIndex) {
}
