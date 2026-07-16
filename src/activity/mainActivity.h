/***********************************************
/gen auto by zuitools
***********************************************/
#ifndef __MAINACTIVITY_H__
#define __MAINACTIVITY_H__


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
#include "control/ZKCheckbox.h"
#include "control/ZKRadioGroup.h"
#include "window/ZKSlideWindow.h"

/*TAG:Macro宏ID*/
#define ID_MAIN_AddressTipSubItem    24006
#define ID_MAIN_NameTipSubItem    24007
#define ID_MAIN_TypeTipSubItem    24008
#define ID_MAIN_ArreTipSubItem    24009
#define ID_MAIN_StatusTipSubItem    24010
#define ID_MAIN_DeviceTipListView    80002
#define ID_MAIN_Window8    110015
#define ID_MAIN_Window8CompletedGroupText    50055
#define ID_MAIN_Window8RunningGroupText    50056
#define ID_MAIN_Window8WaitingGroupText    50057
#define ID_MAIN_Window8StatusLine1Text    50058
#define ID_MAIN_Window8StatusLine2Text    50059
#define ID_MAIN_Window8StatusLine3Text    50060
#define ID_MAIN_Window8LegendRunningText    50061
#define ID_MAIN_Window8LegendWaitingText    50062
#define ID_MAIN_Window8LegendCompletedText    50063
#define ID_MAIN_PumpLabel5    50054
#define ID_MAIN_PumpIcon5    20915
#define ID_MAIN_PumpLabel4    50053
#define ID_MAIN_PumpIcon4    20914
#define ID_MAIN_PumpLabel3    50052
#define ID_MAIN_PumpIcon3    20913
#define ID_MAIN_PumpLabel2    50051
#define ID_MAIN_PumpIcon2    20912
#define ID_MAIN_PumpLabel1    50050
#define ID_MAIN_PumpIcon1    20911
#define ID_MAIN_FlowIcon    20903
#define ID_MAIN_WaterPressureIcon    20902
#define ID_MAIN_RunStatusIcon    20901
#define ID_MAIN_FlowValueText    50042
#define ID_MAIN_FlowLabel    50041
#define ID_MAIN_WaterPressureValueText    50040
#define ID_MAIN_WaterPressureLabel    50039
#define ID_MAIN_RunStatusValueText    50038
#define ID_MAIN_RunStatusLabel    50037
#define ID_MAIN_Window7    110014
#define ID_MAIN_Window6    110007
#define ID_MAIN_TestAdressTipsWindow    110013
#define ID_MAIN_MustChangeAdressButton    20054
#define ID_MAIN_Button40    20053
#define ID_MAIN_ValueRadioButton    22002
#define ID_MAIN_SenserRadioButton    22001
#define ID_MAIN_RadioGroup1    94001
#define ID_MAIN_ChangeAdressOkButton    20077
#define ID_MAIN_TestAdressTipsText    50036
#define ID_MAIN_TestAdressOkButton    20075
#define ID_MAIN_TestAdressEditText    51036
#define ID_MAIN_DestAdressEditText    51036
#define ID_MAIN_SrouceAddressEditText    51037
#define ID_MAIN_CycleTipTextView    50031
#define ID_MAIN_CycleOKButton    20076
#define ID_MAIN_Button39    20074
#define ID_MAIN_IntervalEditText    51035
#define ID_MAIN_Button38    20073
#define ID_MAIN_Button37    20072
#define ID_MAIN_Button36    20071
#define ID_MAIN_SoakMinEditText    51034
#define ID_MAIN_Button35    20070
#define ID_MAIN_SoakHourEditText    51033
#define ID_MAIN_SoakSecEditText    51032
#define ID_MAIN_IrrigationSecEditText    51031
#define ID_MAIN_IrrigationMinEditText    51023
#define ID_MAIN_Button34    20069
#define ID_MAIN_Button33    20068
#define ID_MAIN_Button32    20067
#define ID_MAIN_IrrigationHourEditText    51016
#define ID_MAIN_Button31    20066
#define ID_MAIN_Button24    20061
#define ID_MAIN_Cycle2HourEndEditText    51030
#define ID_MAIN_TextView16    50035
#define ID_MAIN_Button30    20065
#define ID_MAIN_TextView15    50034
#define ID_MAIN_Cycle2MinStartEditText    51029
#define ID_MAIN_Cycle2HourStartEditText    51028
#define ID_MAIN_Button29    20064
#define ID_MAIN_Cycle2MinEndEditText    51027
#define ID_MAIN_Cycle1MinEndEditText    51026
#define ID_MAIN_TextView14    50033
#define ID_MAIN_Cycle1HourEndEditText    51025
#define ID_MAIN_Button28    20063
#define ID_MAIN_Button25    20062
#define ID_MAIN_Cycle1HourStartEditText    51024
#define ID_MAIN_TextView13    50032
#define ID_MAIN_Cycle1MinStartEditText    51022
#define ID_MAIN_Button23    20059
#define ID_MAIN_CycleWindow    110006
#define ID_MAIN_CycleButton    20060
#define ID_MAIN_UartValueOnButton    20056
#define ID_MAIN_UartValueOffButton    20055
#define ID_MAIN_Button41    20057
#define ID_MAIN_Button42    20058
#define ID_MAIN_Button22    20052
#define ID_MAIN_SenserTestValueSubItem    24032
#define ID_MAIN_SenserTestNameValueSubItem    24031
#define ID_MAIN_SenserTestNumValueSubItem    24030
#define ID_MAIN_SenserTestValueListView    80012
#define ID_MAIN_SubItem15    24029
#define ID_MAIN_SubItem14    24028
#define ID_MAIN_SubItem13    24027
#define ID_MAIN_SenserTestTipsListView    80011
#define ID_MAIN_Button21    20051
#define ID_MAIN_GroupTestActionValueSubItem    24026
#define ID_MAIN_GroupTestNameValueSubItem    24025
#define ID_MAIN_GroupTestNumValueSubItem    24024
#define ID_MAIN_GroupTestValueListView    80010
#define ID_MAIN_GroupTestActionTipSubItem    24023
#define ID_MAIN_GroupTestNameTipSubItem    24022
#define ID_MAIN_GroupTestNumTipSubItem    24021
#define ID_MAIN_GroupTestTipsListView    80009
#define ID_MAIN_DeviceTestActionValueSubItem    24020
#define ID_MAIN_DeviceTestNameValueSubItem    24019
#define ID_MAIN_DeviceTestNumValueSubItem    24018
#define ID_MAIN_DeviceTestValueListView    80008
#define ID_MAIN_DeviceTestNumTipSubItem    24015
#define ID_MAIN_DeviceTestActionTipSubItem    24017
#define ID_MAIN_DeviceTestNameTipSubItem    24016
#define ID_MAIN_DeviceTestTipsListView    80007
#define ID_MAIN_Button20    20050
#define ID_MAIN_Button15    20026
#define ID_MAIN_W3TipTextView    50030
#define ID_MAIN_W3TipWindow    110012
#define ID_MAIN_W2TextView    50029
#define ID_MAIN_W2TipWindow    110011
#define ID_MAIN_SetRunTimeZeroButton    20049
#define ID_MAIN_SetRunTimeCencelButton    20048
#define ID_MAIN_SetRunTimeOKButton    20047
#define ID_MAIN_SecEditText    51021
#define ID_MAIN_Button19    20046
#define ID_MAIN_MinEditText    51020
#define ID_MAIN_Button18    20045
#define ID_MAIN_Button17    20044
#define ID_MAIN_HourEditText    51019
#define ID_MAIN_Button16    20043
#define ID_MAIN_SetRunTimeWindow    110010
#define ID_MAIN_RunTimeValueSubItem    24014
#define ID_MAIN_RunTimeNameSubItem    24013
#define ID_MAIN_RunTimeListView    80006
#define ID_MAIN_OnOffProgTextButton    20042
#define ID_MAIN_AfterDayEditText    51018
#define ID_MAIN_Button27    20041
#define ID_MAIN_Button26    20040
#define ID_MAIN_IntervalDayEditText    51017
#define ID_MAIN_TuesdayButton    20039
#define ID_MAIN_MondayButton    20038
#define ID_MAIN_SundayButton    20037
#define ID_MAIN_EverDayButton    20036
#define ID_MAIN_SaturdayButton    20035
#define ID_MAIN_FridayButton    20033
#define ID_MAIN_ThursdayButton    20032
#define ID_MAIN_WednesdayButton    20034
#define ID_MAIN_IntervalModeButton    20031
#define ID_MAIN_WeekModeButton    20030
#define ID_MAIN_OnOffProgButton    20029
#define ID_MAIN_NextProgButton    20028
#define ID_MAIN_PervProgButton    20027
#define ID_MAIN_ShowProgEditText    51015
#define ID_MAIN_Button14    20025
#define ID_MAIN_StartTimeMin4EditText    51014
#define ID_MAIN_Button13    20024
#define ID_MAIN_TextView11    50028
#define ID_MAIN_StartTimeHour4EditText    51013
#define ID_MAIN_StartTimeMin3EditText    51012
#define ID_MAIN_Button12    20023
#define ID_MAIN_TextView10    50027
#define ID_MAIN_StartTimeHour3EditText    51011
#define ID_MAIN_StartTimeMin2EditText    51010
#define ID_MAIN_Button11    20022
#define ID_MAIN_TextView9    50026
#define ID_MAIN_StartTimeHour2EditText    51009
#define ID_MAIN_TextView8    50025
#define ID_MAIN_StartTimeMin1EditText    51008
#define ID_MAIN_StartTimeHour1EditText    51007
#define ID_MAIN_Button10    20021
#define ID_MAIN_GroupBindValueWindow    110008
#define ID_MAIN_SelectSenserListView    80005
#define ID_MAIN_SelectPumpListView    80004
#define ID_MAIN_SelectSenserButton    20020
#define ID_MAIN_SelectPumpButton    20019
#define ID_MAIN_TextView7    50024
#define ID_MAIN_TextView6    50023
#define ID_MAIN_AddSenserEditText    51006
#define ID_MAIN_AddPumpEditText    51005
#define ID_MAIN_GroupAddButton    20018
#define ID_MAIN_GroupDelButton    20017
#define ID_MAIN_GroupCencelButton    20015
#define ID_MAIN_TextView5    50022
#define ID_MAIN_GroupNameEditText    51004
#define ID_MAIN_TextView4    50021
#define ID_MAIN_GroupNumEditText    51003
#define ID_MAIN_IrrNum_TextView    50020
#define ID_MAIN_GroupBind_Button    20014
#define ID_MAIN_ClearIrr_Button    20016
#define ID_MAIN_IrrNum_SubItem    24012
#define ID_MAIN_IrrArr_SubItem    24011
#define ID_MAIN_ChangeIrr_ListView    80003
#define ID_MAIN_W2_CencelButton    20013
#define ID_MAIN_W2_DelButton    20012
#define ID_MAIN_W2_OkButton    20011
#define ID_MAIN_TextView3    50019
#define ID_MAIN_W2_NameEditText    51002
#define ID_MAIN_TextView1    50018
#define ID_MAIN_W2_AddressEditText    51001
#define ID_MAIN_w2set_window    110009
#define ID_MAIN_StatusSubItem    24005
#define ID_MAIN_DeviceListView    80001
#define ID_MAIN_TypeSubItem    24003
#define ID_MAIN_ArreSubItem    24004
#define ID_MAIN_NameSubItem    24002
#define ID_MAIN_AddressSubItem    24001
#define ID_MAIN_Window5    110005
#define ID_MAIN_Window4    110004
#define ID_MAIN_Window3    110003
#define ID_MAIN_Window2    110002
#define ID_MAIN_TextView2    50017
#define ID_MAIN_WaterBar    50016
#define ID_MAIN_PumpCheckbox4    21004
#define ID_MAIN_PumpCheckbox5    21003
#define ID_MAIN_PumpCheckbox3    21002
#define ID_MAIN_PumpCheckbox2    21006
#define ID_MAIN_PumpCheckbox1    21001
#define ID_MAIN_wifistatus    20010
#define ID_MAIN_DigitalClock1    93001
#define ID_MAIN_Button9    20009
#define ID_MAIN_Window1    110001
#define ID_MAIN_Button8    20008
#define ID_MAIN_Button7    20007
#define ID_MAIN_Button5    20005
#define ID_MAIN_Button4    20004
#define ID_MAIN_Button3    20003
#define ID_MAIN_Button2    20002
#define ID_MAIN_Button1    20001
/*TAG:Macro宏ID END*/

class mainActivity : public Activity, 
                     public ZKSeekBar::ISeekBarChangeListener, 
                     public ZKListView::IItemClickListener,
                     public ZKListView::AbsListAdapter,
                     public ZKSlideWindow::ISlideItemClickListener,
                     public EasyUIContext::ITouchListener,
                     public ZKRadioGroup::ICheckedChangeListener,
                     public ZKCheckBox::ICheckedChangeListener,
                     public ZKEditText::ITextChangeListener,
                     public ZKVideoView::IVideoPlayerMessageListener
{
public:
    mainActivity();
    virtual ~mainActivity();

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
    virtual void onCheckedChanged(ZKRadioGroup* pRadioGroup, int checkedID);
    virtual void onCheckedChanged(ZKCheckBox* pCheckBox, bool isChecked);

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
