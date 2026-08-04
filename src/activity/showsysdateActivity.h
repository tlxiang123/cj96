/***********************************************
/gen auto by zuitools
***********************************************/
#ifndef __SHOWSYSDATEACTIVITY_H__
#define __SHOWSYSDATEACTIVITY_H__


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
#define ID_SHOWSYSDATE_TextView7    50010
#define ID_SHOWSYSDATE_DigitalClock1    93001
#define ID_SHOWSYSDATE_AmPmText    50060
#define ID_SHOWSYSDATE_SyncFailureWindow    110020
#define ID_SHOWSYSDATE_SyncFailureOkButton    20021
#define ID_SHOWSYSDATE_SecEditText    51006
#define ID_SHOWSYSDATE_TextDate    50002
#define ID_SHOWSYSDATE_sys_back   100
#define ID_SHOWSYSDATE_Window1    110001
#define ID_SHOWSYSDATE_TextView6    50009
#define ID_SHOWSYSDATE_TextView5    50008
#define ID_SHOWSYSDATE_MinEditText    51005
#define ID_SHOWSYSDATE_TextView4    50007
#define ID_SHOWSYSDATE_HourEditText    51004
#define ID_SHOWSYSDATE_TextView3    50006
#define ID_SHOWSYSDATE_DayEditText    51003
#define ID_SHOWSYSDATE_TextView2    50005
#define ID_SHOWSYSDATE_TextView1    50004
#define ID_SHOWSYSDATE_MonthEditText    51002
#define ID_SHOWSYSDATE_YearEditText    51001
#define ID_SHOWSYSDATE_TextWeek    50003
#define ID_SHOWSYSDATE_TextTime    50001
#define ID_SHOWSYSDATE_DatePickerMonth    50020
#define ID_SHOWSYSDATE_DatePickerWindow    110010
#define ID_SHOWSYSDATE_TimePickerWindow    110011
#define ID_SHOWSYSDATE_DateRowButton    20010
#define ID_SHOWSYSDATE_TimeRowButton    20011
#define ID_SHOWSYSDATE_DatePrevButton    20012
#define ID_SHOWSYSDATE_DateNextButton    20013
#define ID_SHOWSYSDATE_DateCancelButton    20014
#define ID_SHOWSYSDATE_DateDoneButton    20015
#define ID_SHOWSYSDATE_TimeCancelButton    20016
#define ID_SHOWSYSDATE_TimeDoneButton    20017
#define ID_SHOWSYSDATE_DateDayFirst    20100
#define ID_SHOWSYSDATE_DateDayLast    20141
#define ID_SHOWSYSDATE_SyncTimeButton    20018
#define ID_SHOWSYSDATE_TwentyFourHourButton    20019
#define ID_SHOWSYSDATE_TwentyFourHourSwitchButton    20020
#define ID_SHOWSYSDATE_TimezoneTitleText    50270
#define ID_SHOWSYSDATE_TimezoneSelectButton    20600
#define ID_SHOWSYSDATE_TimezoneDropdownWindow    110070
#define ID_SHOWSYSDATE_TimezoneOptionButtonFirst    20601
#define ID_SHOWSYSDATE_TimezoneOptionButtonLast    20610
/*TAG:Macro宏ID END*/

class showsysdateActivity : public Activity, 
                     public ZKSeekBar::ISeekBarChangeListener, 
                     public ZKListView::IItemClickListener,
                     public ZKListView::AbsListAdapter,
                     public ZKSlideWindow::ISlideItemClickListener,
                     public EasyUIContext::ITouchListener,
                     public ZKEditText::ITextChangeListener,
                     public ZKVideoView::IVideoPlayerMessageListener
{
public:
    showsysdateActivity();
    virtual ~showsysdateActivity();

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
