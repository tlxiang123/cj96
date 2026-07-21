/***********************************************
/gen auto by zuitools
***********************************************/
#ifndef __PAGE1TOPSETACTIVITY_H__
#define __PAGE1TOPSETACTIVITY_H__


#include "app/Activity.h"
#include "entry/EasyUIContext.h"

#include "uart/ProtocolData.h"
#include "uart/ProtocolParser.h"

#include "utils/Log.h"
#include "control/ZKDigitalClock.h"
#include "control/ZKButton.h"
#include "control/ZKCircleBar.h"
#include "control/ZKDiagram.h"
#include "control/ZKListView.h"
#include "control/ZKPointer.h"
#include "control/ZKQRCode.h"
#include "control/ZKTextView.h"
#include "control/ZKSeekBar.h"
#include "control/ZKEditText.h"
#include "control/ZKVideoView.h"
#include "window/ZKSlideWindow.h"

/*TAG:Macro宏ID*/
#define ID_PAGE1TOPSET_TextView7    50010
#define ID_PAGE1TOPSET_sys_back   100
#define ID_PAGE1TOPSET_nettopwind    110001
#define ID_PAGE1TOPSET_Button3    20007
#define ID_PAGE1TOPSET_Button2    20006
#define ID_PAGE1TOPSET_DebugPasswordWindow    110005
#define ID_PAGE1TOPSET_DebugPasswordTipText    50011
#define ID_PAGE1TOPSET_DebugPasswordEditText    51001
#define ID_PAGE1TOPSET_DebugPasswordOkButton    20011
#define ID_PAGE1TOPSET_DebugPasswordCancelButton    20010
#define ID_PAGE1TOPSET_DebugBtn    20009
#define ID_PAGE1TOPSET_LanBtn    20005
#define ID_PAGE1TOPSET_DisplayBtn    20008
#define ID_PAGE1TOPSET_SetSysTimeBtn    20001
#define ID_PAGE1TOPSET_Open4GButton    20004
#define ID_PAGE1TOPSET_OpenNetButton    20003
#define ID_PAGE1TOPSET_OpenWifiButton    20002
/*TAG:Macro宏ID END*/

class page1topsetActivity : public Activity, 
                     public ZKSeekBar::ISeekBarChangeListener, 
                     public ZKListView::IItemClickListener,
                     public ZKListView::AbsListAdapter,
                     public ZKSlideWindow::ISlideItemClickListener,
                     public EasyUIContext::ITouchListener,
                     public ZKEditText::ITextChangeListener,
                     public ZKVideoView::IVideoPlayerMessageListener
{
public:
    page1topsetActivity();
    virtual ~page1topsetActivity();

    /**
     * 注册定时器
     */
	void registerUserTimer(int id, int time);
	/**
	 * 取消定时器
	 */
	void unregisterUserTimer(int id);
	/**
	 * 重置定时器
	 */
	void resetUserTimer(int id, int time);

protected:
    /*TAG:PROTECTED_FUNCTION*/
    virtual const char* getAppName() const;
    virtual void onCreate();
    virtual void onClick(ZKBase *pBase);
    virtual void onResume();
    virtual void onPause();
    virtual void onIntent(const Intent *intentPtr);
    virtual bool onTimer(int id);

    virtual void onProgressChanged(ZKSeekBar *pSeekBar, int progress);

    virtual int getListItemCount(const ZKListView *pListView) const;
    virtual void obtainListItemData(ZKListView *pListView, ZKListView::ZKListItem *pListItem, int index);
    virtual void onItemClick(ZKListView *pListView, int index, int subItemIndex);

    virtual void onSlideItemClick(ZKSlideWindow *pSlideWindow, int index);

    virtual bool onTouchEvent(const MotionEvent &ev);

    virtual void onTextChanged(ZKTextView *pTextView, const string &text);

    void rigesterActivityTimer();

    virtual void onVideoPlayerMessage(ZKVideoView *pVideoView, int msg);
    void videoLoopPlayback(ZKVideoView *pVideoView, int msg, size_t callbackTabIndex);
    void startVideoLoopPlayback();
    void stopVideoLoopPlayback();
    bool parseVideoFileList(const char *pFileListPath, std::vector<string>& mediaFileList);
    int removeCharFromString(string& nString, char c);


private:
    /*TAG:PRIVATE_VARIABLE*/
    int mVideoLoopIndex;
    int mVideoLoopErrorCount;

};

#endif
