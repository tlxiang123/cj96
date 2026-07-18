#ifndef __DEVICELISTACTIVITY_H__
#define __DEVICELISTACTIVITY_H__

#include "app/Activity.h"
#include "control/ZKButton.h"
#include "control/ZKListView.h"

class deviceListActivity : public Activity,
                           public ZKListView::AbsListAdapter,
                           public ZKListView::IItemClickListener {
public:
    deviceListActivity();
    virtual ~deviceListActivity();

protected:
    virtual const char* getAppName() const;
    virtual void onCreate();
    virtual void onClick(ZKBase *pBase);

    // ListView callbacks
    virtual int getListItemCount(const ZKListView *pListView) const;
    virtual void obtainListItemData(ZKListView *pListView,
                                    ZKListView::ZKListItem *pListItem,
                                    int index);
    virtual void onItemClick(ZKListView *pListView, int index, int subItemIndex);

private:
    ZKListView* mDeviceListViewPtr;
    ZKListView* mDeviceTipListViewPtr;
    ZKButton* mButton2Ptr;
};

#endif
