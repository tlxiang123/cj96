/***********************************************
/gen auto by zuitools
***********************************************/
#include "mainActivity.h"
#include "utils/BrightnessHelper.h"
/*TAG:GlobalVariable全局变量*/
static ZKListView* mDeviceTipListViewPtr;
static ZKWindow* mWindow8Ptr;
static ZKTextView* mWindow8CompletedGroupTextPtr;
static ZKTextView* mWindow8RunningGroupTextPtr;
static ZKTextView* mWindow8WaitingGroupTextPtr;
static ZKTextView* mWindow8StatusLine1TextPtr;
static ZKTextView* mWindow8StatusLine2TextPtr;
static ZKTextView* mWindow8StatusLine3TextPtr;
static ZKTextView* mPumpLabel5Ptr;
static ZKButton* mPumpIcon5Ptr;
static ZKTextView* mPumpLabel4Ptr;
static ZKButton* mPumpIcon4Ptr;
static ZKTextView* mPumpLabel3Ptr;
static ZKButton* mPumpIcon3Ptr;
static ZKTextView* mPumpLabel2Ptr;
static ZKButton* mPumpIcon2Ptr;
static ZKTextView* mPumpLabel1Ptr;
static ZKButton* mPumpIcon1Ptr;
static ZKButton* mFlowIconPtr;
static ZKButton* mWaterPressureIconPtr;
static ZKButton* mRunStatusIconPtr;
static ZKTextView* mFlowValueTextPtr;
static ZKTextView* mFlowLabelPtr;
static ZKTextView* mWaterPressureValueTextPtr;
static ZKTextView* mWaterPressureLabelPtr;
static ZKTextView* mRunStatusValueTextPtr;
static ZKTextView* mRunStatusLabelPtr;
static ZKWindow* mWindow7Ptr;
static ZKWindow* mWindow6Ptr;
static ZKWindow* mTestAdressTipsWindowPtr;
static ZKButton* mButton41Ptr;
static ZKButton* mButton40Ptr;
static ZKRadioGroup* mRadioGroup1Ptr;
static ZKButton* mChangeAdressOkButtonPtr;
static ZKTextView* mTestAdressTipsTextPtr;
static ZKButton* mTestAdressOkButtonPtr;
static ZKEditText* mTestAdressEditTextPtr;
static ZKEditText* mSrouceAddressEditTextPtr;
static ZKTextView* mCycleTipTextViewPtr;
static ZKButton* mCycleOKButtonPtr;
static ZKButton* mButton39Ptr;
static ZKEditText* mIntervalEditTextPtr;
static ZKButton* mButton38Ptr;
static ZKButton* mButton37Ptr;
static ZKButton* mButton36Ptr;
static ZKEditText* mSoakMinEditTextPtr;
static ZKButton* mButton35Ptr;
static ZKEditText* mSoakHourEditTextPtr;
static ZKEditText* mSoakSecEditTextPtr;
static ZKEditText* mIrrigationSecEditTextPtr;
static ZKEditText* mIrrigationMinEditTextPtr;
static ZKButton* mButton34Ptr;
static ZKButton* mButton33Ptr;
static ZKButton* mButton32Ptr;
static ZKEditText* mIrrigationHourEditTextPtr;
static ZKButton* mButton31Ptr;
static ZKButton* mButton24Ptr;
static ZKEditText* mCycle2HourEndEditTextPtr;
static ZKTextView* mTextView16Ptr;
static ZKButton* mButton30Ptr;
static ZKTextView* mTextView15Ptr;
static ZKEditText* mCycle2MinStartEditTextPtr;
static ZKEditText* mCycle2HourStartEditTextPtr;
static ZKButton* mButton29Ptr;
static ZKEditText* mCycle2MinEndEditTextPtr;
static ZKEditText* mCycle1MinEndEditTextPtr;
static ZKTextView* mTextView14Ptr;
static ZKEditText* mCycle1HourEndEditTextPtr;
static ZKButton* mButton28Ptr;
static ZKButton* mButton25Ptr;
static ZKEditText* mCycle1HourStartEditTextPtr;
static ZKTextView* mTextView13Ptr;
static ZKEditText* mCycle1MinStartEditTextPtr;
static ZKButton* mButton23Ptr;
static ZKWindow* mCycleWindowPtr;
static ZKButton* mCycleButtonPtr;
static ZKButton* mUartValueOffButtonPtr;
static ZKButton* mUartValueOnButtonPtr;
static ZKButton* mButton22Ptr;
static ZKListView* mSenserTestValueListViewPtr;
static ZKListView* mSenserTestTipsListViewPtr;
static ZKButton* mButton21Ptr;
static ZKListView* mGroupTestValueListViewPtr;
static ZKListView* mGroupTestTipsListViewPtr;
static ZKListView* mDeviceTestValueListViewPtr;
static ZKListView* mDeviceTestTipsListViewPtr;
static ZKButton* mButton20Ptr;
static ZKButton* mButton15Ptr;
static ZKTextView* mW3TipTextViewPtr;
static ZKWindow* mW3TipWindowPtr;
static ZKTextView* mW2TextViewPtr;
static ZKWindow* mW2TipWindowPtr;
static ZKButton* mSetRunTimeZeroButtonPtr;
static ZKButton* mSetRunTimeCencelButtonPtr;
static ZKButton* mSetRunTimeOKButtonPtr;
static ZKEditText* mSecEditTextPtr;
static ZKButton* mButton19Ptr;
static ZKEditText* mMinEditTextPtr;
static ZKButton* mButton18Ptr;
static ZKButton* mButton17Ptr;
static ZKEditText* mHourEditTextPtr;
static ZKButton* mButton16Ptr;
static ZKWindow* mSetRunTimeWindowPtr;
static ZKListView* mRunTimeListViewPtr;
static ZKButton* mOnOffProgTextButtonPtr;
static ZKEditText* mAfterDayEditTextPtr;
static ZKButton* mButton27Ptr;
static ZKButton* mButton26Ptr;
static ZKEditText* mIntervalDayEditTextPtr;
static ZKButton* mTuesdayButtonPtr;
static ZKButton* mMondayButtonPtr;
static ZKButton* mSundayButtonPtr;
static ZKButton* mEverDayButtonPtr;
static ZKButton* mSaturdayButtonPtr;
static ZKButton* mFridayButtonPtr;
static ZKButton* mThursdayButtonPtr;
static ZKButton* mWednesdayButtonPtr;
static ZKButton* mIntervalModeButtonPtr;
static ZKButton* mWeekModeButtonPtr;
static ZKButton* mOnOffProgButtonPtr;
static ZKButton* mNextProgButtonPtr;
static ZKButton* mPervProgButtonPtr;
static ZKEditText* mShowProgEditTextPtr;
static ZKButton* mButton14Ptr;
static ZKEditText* mStartTimeMin4EditTextPtr;
static ZKButton* mButton13Ptr;
static ZKTextView* mTextView11Ptr;
static ZKEditText* mStartTimeHour4EditTextPtr;
static ZKEditText* mStartTimeMin3EditTextPtr;
static ZKButton* mButton12Ptr;
static ZKTextView* mTextView10Ptr;
static ZKEditText* mStartTimeHour3EditTextPtr;
static ZKEditText* mStartTimeMin2EditTextPtr;
static ZKButton* mButton11Ptr;
static ZKTextView* mTextView9Ptr;
static ZKEditText* mStartTimeHour2EditTextPtr;
static ZKTextView* mTextView8Ptr;
static ZKEditText* mStartTimeMin1EditTextPtr;
static ZKEditText* mStartTimeHour1EditTextPtr;
static ZKButton* mButton10Ptr;
static ZKWindow* mGroupBindValueWindowPtr;
static ZKListView* mSelectSenserListViewPtr;
static ZKListView* mSelectPumpListViewPtr;
static ZKButton* mSelectSenserButtonPtr;
static ZKButton* mSelectPumpButtonPtr;
static ZKTextView* mTextView7Ptr;
static ZKTextView* mTextView6Ptr;
static ZKEditText* mAddSenserEditTextPtr;
static ZKEditText* mAddPumpEditTextPtr;
static ZKButton* mGroupAddButtonPtr;
static ZKButton* mGroupDelButtonPtr;
static ZKButton* mGroupCencelButtonPtr;
static ZKTextView* mTextView5Ptr;
static ZKEditText* mGroupNameEditTextPtr;
static ZKTextView* mTextView4Ptr;
static ZKEditText* mGroupNumEditTextPtr;
static ZKTextView* mIrrNum_TextViewPtr;
static ZKButton* mGroupBind_ButtonPtr;
static ZKButton* mClearIrr_ButtonPtr;
static ZKListView* mChangeIrr_ListViewPtr;
static ZKButton* mW2_CencelButtonPtr;
static ZKButton* mW2_DelButtonPtr;
static ZKButton* mW2_OkButtonPtr;
static ZKTextView* mTextView3Ptr;
static ZKEditText* mW2_NameEditTextPtr;
static ZKTextView* mTextView1Ptr;
static ZKEditText* mW2_AddressEditTextPtr;
static ZKWindow* mw2set_windowPtr;
static ZKListView* mDeviceListViewPtr;
static ZKWindow* mWindow5Ptr;
static ZKWindow* mWindow4Ptr;
static ZKWindow* mWindow3Ptr;
static ZKWindow* mWindow2Ptr;
static ZKTextView* mTextView2Ptr;
static ZKTextView* mWaterBarPtr;
static ZKCheckBox* mPumpCheckbox4Ptr;
static ZKCheckBox* mPumpCheckbox5Ptr;
static ZKCheckBox* mPumpCheckbox3Ptr;
static ZKCheckBox* mPumpCheckbox2Ptr;
static ZKCheckBox* mPumpCheckbox1Ptr;

static ZKDigitalClock* mDigitalClock1Ptr;
static ZKButton* mwifistatusPtr;
static ZKButton* mButton9Ptr;
static ZKWindow* mWindow1Ptr;
static ZKButton* mButton8Ptr;
static ZKButton* mButton7Ptr;
static ZKButton* mButton5Ptr;
static ZKButton* mButton4Ptr;
static ZKButton* mButton3Ptr;
static ZKButton* mButton2Ptr;
static ZKButton* mButton1Ptr;

static mainActivity* mActivityPtr;


/*register activity*/
REGISTER_ACTIVITY(mainActivity);

typedef struct {
	int id; // 定时器ID ， 不能重复
	int time; // 定时器  时间间隔  单位 毫秒
}S_ACTIVITY_TIMEER;

#include "logic/mainLogic.cc"

/***********/
typedef struct {
    int id;
    const char *pApp;
} SAppInfo;

/**
 *点击跳转window
 */
static SAppInfo sAppInfoTab[] = {
//  { ID_MAIN_TEXT, "TextViewActivity" },
};

/***************/
typedef bool (*ButtonCallback)(ZKButton *pButton);
/**
 * button onclick表
 */
typedef struct {
    int id;
    ButtonCallback callback;
}S_ButtonCallback;

/*TAG:ButtonCallbackTab按键映射表*/
static S_ButtonCallback sButtonCallbackTab[] = {
    ID_MAIN_Button41, onButtonClick_Button41,
    ID_MAIN_Button42, onButtonClick_Button42,
    ID_MAIN_PumpIcon5, onButtonClick_PumpIcon5,
    ID_MAIN_PumpIcon4, onButtonClick_PumpIcon4,
    ID_MAIN_PumpIcon3, onButtonClick_PumpIcon3,
    ID_MAIN_PumpIcon2, onButtonClick_PumpIcon2,
    ID_MAIN_PumpIcon1, onButtonClick_PumpIcon1,
    ID_MAIN_FlowIcon, onButtonClick_FlowIcon,
    ID_MAIN_WaterPressureIcon, onButtonClick_WaterPressureIcon,
    ID_MAIN_RunStatusIcon, onButtonClick_RunStatusIcon,
    ID_MAIN_MustChangeAdressButton, onButtonClick_MustChangeAdressButton,
    ID_MAIN_Button40, onButtonClick_Button40,
    ID_MAIN_ChangeAdressOkButton, onButtonClick_ChangeAdressOkButton,
    ID_MAIN_TestAdressOkButton, onButtonClick_TestAdressOkButton,
    ID_MAIN_CycleOKButton, onButtonClick_CycleOKButton,
    ID_MAIN_Button39, onButtonClick_Button39,
    ID_MAIN_Button38, onButtonClick_Button38,
    ID_MAIN_Button37, onButtonClick_Button37,
    ID_MAIN_Button36, onButtonClick_Button36,
    ID_MAIN_Button35, onButtonClick_Button35,
    ID_MAIN_Button34, onButtonClick_Button34,
    ID_MAIN_Button33, onButtonClick_Button33,
    ID_MAIN_Button32, onButtonClick_Button32,
    ID_MAIN_Button31, onButtonClick_Button31,
    ID_MAIN_Button24, onButtonClick_Button24,
    ID_MAIN_Button30, onButtonClick_Button30,
    ID_MAIN_Button29, onButtonClick_Button29,
    ID_MAIN_Button28, onButtonClick_Button28,
    ID_MAIN_Button25, onButtonClick_Button25,
    ID_MAIN_Button23, onButtonClick_Button23,
    ID_MAIN_CycleButton, onButtonClick_CycleButton,
    ID_MAIN_UartValueOffButton, onButtonClick_UartValueOffButton,
    ID_MAIN_UartValueOnButton, onButtonClick_UartValueOnButton,
    ID_MAIN_Button22, onButtonClick_Button22,
    ID_MAIN_Button21, onButtonClick_Button21,
    ID_MAIN_Button20, onButtonClick_Button20,
    ID_MAIN_Button15, onButtonClick_Button15,
    ID_MAIN_SetRunTimeZeroButton, onButtonClick_SetRunTimeZeroButton,
    ID_MAIN_SetRunTimeCencelButton, onButtonClick_SetRunTimeCencelButton,
    ID_MAIN_SetRunTimeOKButton, onButtonClick_SetRunTimeOKButton,
    ID_MAIN_Button19, onButtonClick_Button19,
    ID_MAIN_Button18, onButtonClick_Button18,
    ID_MAIN_Button17, onButtonClick_Button17,
    ID_MAIN_Button16, onButtonClick_Button16,
    ID_MAIN_OnOffProgTextButton, onButtonClick_OnOffProgTextButton,
    ID_MAIN_Button27, onButtonClick_Button27,
    ID_MAIN_Button26, onButtonClick_Button26,
    ID_MAIN_TuesdayButton, onButtonClick_TuesdayButton,
    ID_MAIN_MondayButton, onButtonClick_MondayButton,
    ID_MAIN_SundayButton, onButtonClick_SundayButton,
    ID_MAIN_EverDayButton, onButtonClick_EverDayButton,
    ID_MAIN_SaturdayButton, onButtonClick_SaturdayButton,
    ID_MAIN_FridayButton, onButtonClick_FridayButton,
    ID_MAIN_ThursdayButton, onButtonClick_ThursdayButton,
    ID_MAIN_WednesdayButton, onButtonClick_WednesdayButton,
    ID_MAIN_IntervalModeButton, onButtonClick_IntervalModeButton,
    ID_MAIN_WeekModeButton, onButtonClick_WeekModeButton,
    ID_MAIN_OnOffProgButton, onButtonClick_OnOffProgButton,
    ID_MAIN_NextProgButton, onButtonClick_NextProgButton,
    ID_MAIN_PervProgButton, onButtonClick_PervProgButton,
    ID_MAIN_Button14, onButtonClick_Button14,
    ID_MAIN_Button13, onButtonClick_Button13,
    ID_MAIN_Button12, onButtonClick_Button12,
    ID_MAIN_Button11, onButtonClick_Button11,
    ID_MAIN_Button10, onButtonClick_Button10,
    ID_MAIN_SelectSenserButton, onButtonClick_SelectSenserButton,
    ID_MAIN_SelectPumpButton, onButtonClick_SelectPumpButton,
    ID_MAIN_GroupAddButton, onButtonClick_GroupAddButton,
    ID_MAIN_GroupDelButton, onButtonClick_GroupDelButton,
    ID_MAIN_GroupCencelButton, onButtonClick_GroupCencelButton,
    ID_MAIN_GroupBind_Button, onButtonClick_GroupBind_Button,
    ID_MAIN_ClearIrr_Button, onButtonClick_ClearIrr_Button,
    ID_MAIN_W2_CencelButton, onButtonClick_W2_CencelButton,
    ID_MAIN_W2_DelButton, onButtonClick_W2_DelButton,
    ID_MAIN_W2_OkButton, onButtonClick_W2_OkButton,
    ID_MAIN_wifistatus, onButtonClick_wifistatus,
    ID_MAIN_Button9, onButtonClick_Button9,
    ID_MAIN_Button8, onButtonClick_Button8,
    ID_MAIN_Button7, onButtonClick_Button7,
    ID_MAIN_Button5, onButtonClick_Button5,
    ID_MAIN_Button4, onButtonClick_Button4,
    ID_MAIN_Button3, onButtonClick_Button3,
    ID_MAIN_Button2, onButtonClick_Button2,
    ID_MAIN_Button1, onButtonClick_Button1,
};
/***************/


typedef void (*SeekBarCallback)(ZKSeekBar *pSeekBar, int progress);
typedef struct {
    int id;
    SeekBarCallback callback;
}S_ZKSeekBarCallback;
/*TAG:SeekBarCallbackTab*/
static S_ZKSeekBarCallback SZKSeekBarCallbackTab[] = {
};


typedef int (*ListViewGetItemCountCallback)(const ZKListView *pListView);
typedef void (*ListViewobtainListItemDataCallback)(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index);
typedef void (*ListViewonItemClickCallback)(ZKListView *pListView, int index, int id);
typedef struct {
    int id;
    ListViewGetItemCountCallback getListItemCountCallback;
    ListViewobtainListItemDataCallback obtainListItemDataCallback;
    ListViewonItemClickCallback onItemClickCallback;
}S_ListViewFunctionsCallback;
/*TAG:ListViewFunctionsCallback*/
static S_ListViewFunctionsCallback SListViewFunctionsCallbackTab[] = {
    ID_MAIN_DeviceTipListView, getListItemCount_DeviceTipListView, obtainListItemData_DeviceTipListView, onListItemClick_DeviceTipListView,
    ID_MAIN_SenserTestValueListView, getListItemCount_SenserTestValueListView, obtainListItemData_SenserTestValueListView, onListItemClick_SenserTestValueListView,
    ID_MAIN_SenserTestTipsListView, getListItemCount_SenserTestTipsListView, obtainListItemData_SenserTestTipsListView, onListItemClick_SenserTestTipsListView,
    ID_MAIN_GroupTestValueListView, getListItemCount_GroupTestValueListView, obtainListItemData_GroupTestValueListView, onListItemClick_GroupTestValueListView,
    ID_MAIN_GroupTestTipsListView, getListItemCount_GroupTestTipsListView, obtainListItemData_GroupTestTipsListView, onListItemClick_GroupTestTipsListView,
    ID_MAIN_DeviceTestValueListView, getListItemCount_DeviceTestValueListView, obtainListItemData_DeviceTestValueListView, onListItemClick_DeviceTestValueListView,
    ID_MAIN_DeviceTestTipsListView, getListItemCount_DeviceTestTipsListView, obtainListItemData_DeviceTestTipsListView, onListItemClick_DeviceTestTipsListView,
    ID_MAIN_RunTimeListView, getListItemCount_RunTimeListView, obtainListItemData_RunTimeListView, onListItemClick_RunTimeListView,
    ID_MAIN_SelectSenserListView, getListItemCount_SelectSenserListView, obtainListItemData_SelectSenserListView, onListItemClick_SelectSenserListView,
    ID_MAIN_SelectPumpListView, getListItemCount_SelectPumpListView, obtainListItemData_SelectPumpListView, onListItemClick_SelectPumpListView,
    ID_MAIN_ChangeIrr_ListView, getListItemCount_ChangeIrr_ListView, obtainListItemData_ChangeIrr_ListView, onListItemClick_ChangeIrr_ListView,
    ID_MAIN_DeviceListView, getListItemCount_DeviceListView, obtainListItemData_DeviceListView, onListItemClick_DeviceListView,
};


typedef void (*SlideWindowItemClickCallback)(ZKSlideWindow *pSlideWindow, int index);
typedef struct {
    int id;
    SlideWindowItemClickCallback onSlideItemClickCallback;
}S_SlideWindowItemClickCallback;
/*TAG:SlideWindowFunctionsCallbackTab*/
static S_SlideWindowItemClickCallback SSlideWindowItemClickCallbackTab[] = {
};


typedef void (*EditTextInputCallback)(const std::string &text);
typedef struct {
    int id;
    EditTextInputCallback onEditTextChangedCallback;
}S_EditTextInputCallback;
/*TAG:EditTextInputCallback*/
static S_EditTextInputCallback SEditTextInputCallbackTab[] = {
    ID_MAIN_SrouceAddressEditText, onEditTextChanged_SrouceAddressEditText,
    ID_MAIN_TestAdressEditText, onEditTextChanged_TestAdressEditText,
    ID_MAIN_IntervalEditText, onEditTextChanged_IntervalEditText,
    ID_MAIN_SoakMinEditText, onEditTextChanged_SoakMinEditText,
    ID_MAIN_SoakHourEditText, onEditTextChanged_SoakHourEditText,
    ID_MAIN_SoakSecEditText, onEditTextChanged_SoakSecEditText,
    ID_MAIN_IrrigationSecEditText, onEditTextChanged_IrrigationSecEditText,
    ID_MAIN_IrrigationMinEditText, onEditTextChanged_IrrigationMinEditText,
    ID_MAIN_IrrigationHourEditText, onEditTextChanged_IrrigationHourEditText,
    ID_MAIN_Cycle2HourEndEditText, onEditTextChanged_Cycle2HourEndEditText,
    ID_MAIN_Cycle2MinStartEditText, onEditTextChanged_Cycle2MinStartEditText,
    ID_MAIN_Cycle2HourStartEditText, onEditTextChanged_Cycle2HourStartEditText,
    ID_MAIN_Cycle2MinEndEditText, onEditTextChanged_Cycle2MinEndEditText,
    ID_MAIN_Cycle1MinEndEditText, onEditTextChanged_Cycle1MinEndEditText,
    ID_MAIN_Cycle1HourEndEditText, onEditTextChanged_Cycle1HourEndEditText,
    ID_MAIN_Cycle1HourStartEditText, onEditTextChanged_Cycle1HourStartEditText,
    ID_MAIN_Cycle1MinStartEditText, onEditTextChanged_Cycle1MinStartEditText,
    ID_MAIN_SecEditText, onEditTextChanged_SecEditText,
    ID_MAIN_MinEditText, onEditTextChanged_MinEditText,
    ID_MAIN_HourEditText, onEditTextChanged_HourEditText,
    ID_MAIN_AfterDayEditText, onEditTextChanged_AfterDayEditText,
    ID_MAIN_IntervalDayEditText, onEditTextChanged_IntervalDayEditText,
    ID_MAIN_ShowProgEditText, onEditTextChanged_ShowProgEditText,
    ID_MAIN_StartTimeMin4EditText, onEditTextChanged_StartTimeMin4EditText,
    ID_MAIN_StartTimeHour4EditText, onEditTextChanged_StartTimeHour4EditText,
    ID_MAIN_StartTimeMin3EditText, onEditTextChanged_StartTimeMin3EditText,
    ID_MAIN_StartTimeHour3EditText, onEditTextChanged_StartTimeHour3EditText,
    ID_MAIN_StartTimeMin2EditText, onEditTextChanged_StartTimeMin2EditText,
    ID_MAIN_StartTimeHour2EditText, onEditTextChanged_StartTimeHour2EditText,
    ID_MAIN_StartTimeMin1EditText, onEditTextChanged_StartTimeMin1EditText,
    ID_MAIN_StartTimeHour1EditText, onEditTextChanged_StartTimeHour1EditText,
    ID_MAIN_AddSenserEditText, onEditTextChanged_AddSenserEditText,
    ID_MAIN_AddPumpEditText, onEditTextChanged_AddPumpEditText,
    ID_MAIN_GroupNameEditText, onEditTextChanged_GroupNameEditText,
    ID_MAIN_GroupNumEditText, onEditTextChanged_GroupNumEditText,
    ID_MAIN_W2_NameEditText, onEditTextChanged_W2_NameEditText,
    ID_MAIN_W2_AddressEditText, onEditTextChanged_W2_AddressEditText,
};

typedef void (*VideoViewCallback)(ZKVideoView *pVideoView, int msg);
typedef struct {
    int id; //VideoView ID
    bool loop; // 是否是轮播类型
    int defaultvolume;//轮播类型时,默认视频音量
    VideoViewCallback onVideoViewCallback;
}S_VideoViewCallback;
/*TAG:VideoViewCallback*/
static S_VideoViewCallback SVideoViewCallbackTab[] = {
};


typedef void (*CheckboxCallback)(ZKCheckBox*, bool);
typedef struct {
  int id;
  CheckboxCallback onCheckedChanged;
}S_CheckboxCallback;
/*TAG:CheckboxCallbackTab*/
static S_CheckboxCallback SCheckboxCallbackTab[] = {
    ID_MAIN_PumpCheckbox4, onCheckedChanged_PumpCheckbox4,
    ID_MAIN_PumpCheckbox5, onCheckedChanged_PumpCheckbox5,
    ID_MAIN_PumpCheckbox3, onCheckedChanged_PumpCheckbox3,
    ID_MAIN_PumpCheckbox2, onCheckedChanged_PumpCheckbox2,
    ID_MAIN_PumpCheckbox1, onCheckedChanged_PumpCheckbox1,
};

typedef void (*RadioGroupCallback)(ZKRadioGroup*, int);
typedef struct {
  int id;
  RadioGroupCallback onCheckedChanged;
}S_RadioGroupCallback;
/*TAG:RadioGroupCallbackTab*/
static S_RadioGroupCallback SRadioGroupCallbackTab[] = {
    ID_MAIN_RadioGroup1, onCheckedChanged_RadioGroup1,
};

mainActivity::mainActivity() {
	//todo add init code here
	mVideoLoopIndex = -1;
	mVideoLoopErrorCount = 0;
}

mainActivity::~mainActivity() {
  //todo add init file here
  // 退出应用时需要反注册

	BRIGHTNESSHELPER->screenOff();
    EASYUICONTEXT->unregisterGlobalTouchListener(this);
    unregisterProtocolDataUpdateListener(onProtocolDataUpdate);
    onUI_quit();
    mActivityPtr = NULL;
    mDeviceTipListViewPtr = NULL;
    mWindow8Ptr = NULL;
    mWindow8CompletedGroupTextPtr = NULL;
    mWindow8RunningGroupTextPtr = NULL;
    mWindow8WaitingGroupTextPtr = NULL;
    mWindow8StatusLine1TextPtr = NULL;
    mWindow8StatusLine2TextPtr = NULL;
    mWindow8StatusLine3TextPtr = NULL;
    mPumpLabel5Ptr = NULL;
    mPumpIcon5Ptr = NULL;
    mPumpLabel4Ptr = NULL;
    mPumpIcon4Ptr = NULL;
    mPumpLabel3Ptr = NULL;
    mPumpIcon3Ptr = NULL;
    mPumpLabel2Ptr = NULL;
    mPumpIcon2Ptr = NULL;
    mPumpLabel1Ptr = NULL;
    mPumpIcon1Ptr = NULL;
    mFlowIconPtr = NULL;
    mWaterPressureIconPtr = NULL;
    mRunStatusIconPtr = NULL;
    mFlowValueTextPtr = NULL;
    mFlowLabelPtr = NULL;
    mWaterPressureValueTextPtr = NULL;
    mWaterPressureLabelPtr = NULL;
    mRunStatusValueTextPtr = NULL;
    mRunStatusLabelPtr = NULL;
    mWindow7Ptr = NULL;
    mWindow6Ptr = NULL;
    mTestAdressTipsWindowPtr = NULL;
    mButton41Ptr = NULL;
    mButton40Ptr = NULL;
    mRadioGroup1Ptr = NULL;
    mChangeAdressOkButtonPtr = NULL;
    mTestAdressTipsTextPtr = NULL;
    mTestAdressOkButtonPtr = NULL;
    mTestAdressEditTextPtr = NULL;
    mSrouceAddressEditTextPtr = NULL;
    mCycleTipTextViewPtr = NULL;
    mCycleOKButtonPtr = NULL;
    mButton39Ptr = NULL;
    mIntervalEditTextPtr = NULL;
    mButton38Ptr = NULL;
    mButton37Ptr = NULL;
    mButton36Ptr = NULL;
    mSoakMinEditTextPtr = NULL;
    mButton35Ptr = NULL;
    mSoakHourEditTextPtr = NULL;
    mSoakSecEditTextPtr = NULL;
    mIrrigationSecEditTextPtr = NULL;
    mIrrigationMinEditTextPtr = NULL;
    mButton34Ptr = NULL;
    mButton33Ptr = NULL;
    mButton32Ptr = NULL;
    mIrrigationHourEditTextPtr = NULL;
    mButton31Ptr = NULL;
    mButton24Ptr = NULL;
    mCycle2HourEndEditTextPtr = NULL;
    mTextView16Ptr = NULL;
    mButton30Ptr = NULL;
    mTextView15Ptr = NULL;
    mCycle2MinStartEditTextPtr = NULL;
    mCycle2HourStartEditTextPtr = NULL;
    mButton29Ptr = NULL;
    mCycle2MinEndEditTextPtr = NULL;
    mCycle1MinEndEditTextPtr = NULL;
    mTextView14Ptr = NULL;
    mCycle1HourEndEditTextPtr = NULL;
    mButton28Ptr = NULL;
    mButton25Ptr = NULL;
    mCycle1HourStartEditTextPtr = NULL;
    mTextView13Ptr = NULL;
    mCycle1MinStartEditTextPtr = NULL;
    mButton23Ptr = NULL;
    mCycleWindowPtr = NULL;
    mCycleButtonPtr = NULL;
    mUartValueOffButtonPtr = NULL;
    mUartValueOnButtonPtr = NULL;
    mButton22Ptr = NULL;
    mSenserTestValueListViewPtr = NULL;
    mSenserTestTipsListViewPtr = NULL;
    mButton21Ptr = NULL;
    mGroupTestValueListViewPtr = NULL;
    mGroupTestTipsListViewPtr = NULL;
    mDeviceTestValueListViewPtr = NULL;
    mDeviceTestTipsListViewPtr = NULL;
    mButton20Ptr = NULL;
    mButton15Ptr = NULL;
    mW3TipTextViewPtr = NULL;
    mW3TipWindowPtr = NULL;
    mW2TextViewPtr = NULL;
    mW2TipWindowPtr = NULL;
    mSetRunTimeZeroButtonPtr = NULL;
    mSetRunTimeCencelButtonPtr = NULL;
    mSetRunTimeOKButtonPtr = NULL;
    mSecEditTextPtr = NULL;
    mButton19Ptr = NULL;
    mMinEditTextPtr = NULL;
    mButton18Ptr = NULL;
    mButton17Ptr = NULL;
    mHourEditTextPtr = NULL;
    mButton16Ptr = NULL;
    mSetRunTimeWindowPtr = NULL;
    mRunTimeListViewPtr = NULL;
    mOnOffProgTextButtonPtr = NULL;
    mAfterDayEditTextPtr = NULL;
    mButton27Ptr = NULL;
    mButton26Ptr = NULL;
    mIntervalDayEditTextPtr = NULL;
    mTuesdayButtonPtr = NULL;
    mMondayButtonPtr = NULL;
    mSundayButtonPtr = NULL;
    mEverDayButtonPtr = NULL;
    mSaturdayButtonPtr = NULL;
    mFridayButtonPtr = NULL;
    mThursdayButtonPtr = NULL;
    mWednesdayButtonPtr = NULL;
    mIntervalModeButtonPtr = NULL;
    mWeekModeButtonPtr = NULL;
    mOnOffProgButtonPtr = NULL;
    mNextProgButtonPtr = NULL;
    mPervProgButtonPtr = NULL;
    mShowProgEditTextPtr = NULL;
    mButton14Ptr = NULL;
    mStartTimeMin4EditTextPtr = NULL;
    mButton13Ptr = NULL;
    mTextView11Ptr = NULL;
    mStartTimeHour4EditTextPtr = NULL;
    mStartTimeMin3EditTextPtr = NULL;
    mButton12Ptr = NULL;
    mTextView10Ptr = NULL;
    mStartTimeHour3EditTextPtr = NULL;
    mStartTimeMin2EditTextPtr = NULL;
    mButton11Ptr = NULL;
    mTextView9Ptr = NULL;
    mStartTimeHour2EditTextPtr = NULL;
    mTextView8Ptr = NULL;
    mStartTimeMin1EditTextPtr = NULL;
    mStartTimeHour1EditTextPtr = NULL;
    mButton10Ptr = NULL;
    mGroupBindValueWindowPtr = NULL;
    mSelectSenserListViewPtr = NULL;
    mSelectPumpListViewPtr = NULL;
    mSelectSenserButtonPtr = NULL;
    mSelectPumpButtonPtr = NULL;
    mTextView7Ptr = NULL;
    mTextView6Ptr = NULL;
    mAddSenserEditTextPtr = NULL;
    mAddPumpEditTextPtr = NULL;
    mGroupAddButtonPtr = NULL;
    mGroupDelButtonPtr = NULL;
    mGroupCencelButtonPtr = NULL;
    mTextView5Ptr = NULL;
    mGroupNameEditTextPtr = NULL;
    mTextView4Ptr = NULL;
    mGroupNumEditTextPtr = NULL;
    mIrrNum_TextViewPtr = NULL;
    mGroupBind_ButtonPtr = NULL;
    mClearIrr_ButtonPtr = NULL;
    mChangeIrr_ListViewPtr = NULL;
    mW2_CencelButtonPtr = NULL;
    mW2_DelButtonPtr = NULL;
    mW2_OkButtonPtr = NULL;
    mTextView3Ptr = NULL;
    mW2_NameEditTextPtr = NULL;
    mTextView1Ptr = NULL;
    mW2_AddressEditTextPtr = NULL;
    mw2set_windowPtr = NULL;
    mDeviceListViewPtr = NULL;
    mWindow5Ptr = NULL;
    mWindow4Ptr = NULL;
    mWindow3Ptr = NULL;
    mWindow2Ptr = NULL;
    mTextView2Ptr = NULL;
    mWaterBarPtr = NULL;
    mPumpCheckbox4Ptr = NULL;
    mPumpCheckbox5Ptr = NULL;
    mPumpCheckbox3Ptr = NULL;
    mPumpCheckbox2Ptr = NULL;
    mPumpCheckbox1Ptr = NULL;
    mwifistatusPtr = NULL;
    mDigitalClock1Ptr = NULL;
    mButton9Ptr = NULL;
    mWindow1Ptr = NULL;
    mButton8Ptr = NULL;
    mButton7Ptr = NULL;
    mButton5Ptr = NULL;
    mButton4Ptr = NULL;
    mButton3Ptr = NULL;
    mButton2Ptr = NULL;
    mButton1Ptr = NULL;

}

const char* mainActivity::getAppName() const{
	return "main.ftu";
}

//TAG:onCreate
void mainActivity::onCreate() {
	Activity::onCreate();
    mDeviceTipListViewPtr = (ZKListView*)findControlByID(ID_MAIN_DeviceTipListView);if(mDeviceTipListViewPtr!= NULL){mDeviceTipListViewPtr->setListAdapter(this);mDeviceTipListViewPtr->setItemClickListener(this);}
    mWindow8Ptr = (ZKWindow*)findControlByID(ID_MAIN_Window8);
    mWindow8CompletedGroupTextPtr = (ZKTextView*)findControlByID(ID_MAIN_Window8CompletedGroupText);
    mWindow8RunningGroupTextPtr = (ZKTextView*)findControlByID(ID_MAIN_Window8RunningGroupText);
    mWindow8WaitingGroupTextPtr = (ZKTextView*)findControlByID(ID_MAIN_Window8WaitingGroupText);
    mWindow8StatusLine1TextPtr = (ZKTextView*)findControlByID(ID_MAIN_Window8StatusLine1Text);
    mWindow8StatusLine2TextPtr = (ZKTextView*)findControlByID(ID_MAIN_Window8StatusLine2Text);
    mWindow8StatusLine3TextPtr = (ZKTextView*)findControlByID(ID_MAIN_Window8StatusLine3Text);
    mPumpLabel5Ptr = (ZKTextView*)findControlByID(ID_MAIN_PumpLabel5);
    mPumpIcon5Ptr = (ZKButton*)findControlByID(ID_MAIN_PumpIcon5);
    mPumpLabel4Ptr = (ZKTextView*)findControlByID(ID_MAIN_PumpLabel4);
    mPumpIcon4Ptr = (ZKButton*)findControlByID(ID_MAIN_PumpIcon4);
    mPumpLabel3Ptr = (ZKTextView*)findControlByID(ID_MAIN_PumpLabel3);
    mPumpIcon3Ptr = (ZKButton*)findControlByID(ID_MAIN_PumpIcon3);
    mPumpLabel2Ptr = (ZKTextView*)findControlByID(ID_MAIN_PumpLabel2);
    mPumpIcon2Ptr = (ZKButton*)findControlByID(ID_MAIN_PumpIcon2);
    mPumpLabel1Ptr = (ZKTextView*)findControlByID(ID_MAIN_PumpLabel1);
    mPumpIcon1Ptr = (ZKButton*)findControlByID(ID_MAIN_PumpIcon1);
    mFlowIconPtr = (ZKButton*)findControlByID(ID_MAIN_FlowIcon);
    mWaterPressureIconPtr = (ZKButton*)findControlByID(ID_MAIN_WaterPressureIcon);
    mRunStatusIconPtr = (ZKButton*)findControlByID(ID_MAIN_RunStatusIcon);
    mFlowValueTextPtr = (ZKTextView*)findControlByID(ID_MAIN_FlowValueText);
    mFlowLabelPtr = (ZKTextView*)findControlByID(ID_MAIN_FlowLabel);
    mWaterPressureValueTextPtr = (ZKTextView*)findControlByID(ID_MAIN_WaterPressureValueText);
    mWaterPressureLabelPtr = (ZKTextView*)findControlByID(ID_MAIN_WaterPressureLabel);
    mRunStatusValueTextPtr = (ZKTextView*)findControlByID(ID_MAIN_RunStatusValueText);
    mRunStatusLabelPtr = (ZKTextView*)findControlByID(ID_MAIN_RunStatusLabel);
    mWindow7Ptr = (ZKWindow*)findControlByID(ID_MAIN_Window7);
    mWindow6Ptr = (ZKWindow*)findControlByID(ID_MAIN_Window6);
    mTestAdressTipsWindowPtr = (ZKWindow*)findControlByID(ID_MAIN_TestAdressTipsWindow);
    mButton41Ptr = (ZKButton*)findControlByID(ID_MAIN_Button41);
    mButton40Ptr = (ZKButton*)findControlByID(ID_MAIN_Button40);
    mRadioGroup1Ptr = (ZKRadioGroup*)findControlByID(ID_MAIN_RadioGroup1);if(mRadioGroup1Ptr!= NULL){mRadioGroup1Ptr->setCheckedChangeListener(this);}
    mChangeAdressOkButtonPtr = (ZKButton*)findControlByID(ID_MAIN_ChangeAdressOkButton);
    mTestAdressTipsTextPtr = (ZKTextView*)findControlByID(ID_MAIN_TestAdressTipsText);
    mTestAdressOkButtonPtr = (ZKButton*)findControlByID(ID_MAIN_TestAdressOkButton);
    mTestAdressEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_TestAdressEditText);if(mTestAdressEditTextPtr!= NULL){mTestAdressEditTextPtr->setTextChangeListener(this);}
    mSrouceAddressEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_SrouceAddressEditText);if(mSrouceAddressEditTextPtr!= NULL){mSrouceAddressEditTextPtr->setTextChangeListener(this);}
    mCycleTipTextViewPtr = (ZKTextView*)findControlByID(ID_MAIN_CycleTipTextView);
    mCycleOKButtonPtr = (ZKButton*)findControlByID(ID_MAIN_CycleOKButton);
    mButton39Ptr = (ZKButton*)findControlByID(ID_MAIN_Button39);
    mIntervalEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_IntervalEditText);if(mIntervalEditTextPtr!= NULL){mIntervalEditTextPtr->setTextChangeListener(this);}
    mButton38Ptr = (ZKButton*)findControlByID(ID_MAIN_Button38);
    mButton37Ptr = (ZKButton*)findControlByID(ID_MAIN_Button37);
    mButton36Ptr = (ZKButton*)findControlByID(ID_MAIN_Button36);
    mSoakMinEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_SoakMinEditText);if(mSoakMinEditTextPtr!= NULL){mSoakMinEditTextPtr->setTextChangeListener(this);}
    mButton35Ptr = (ZKButton*)findControlByID(ID_MAIN_Button35);
    mSoakHourEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_SoakHourEditText);if(mSoakHourEditTextPtr!= NULL){mSoakHourEditTextPtr->setTextChangeListener(this);}
    mSoakSecEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_SoakSecEditText);if(mSoakSecEditTextPtr!= NULL){mSoakSecEditTextPtr->setTextChangeListener(this);}
    mIrrigationSecEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_IrrigationSecEditText);if(mIrrigationSecEditTextPtr!= NULL){mIrrigationSecEditTextPtr->setTextChangeListener(this);}
    mIrrigationMinEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_IrrigationMinEditText);if(mIrrigationMinEditTextPtr!= NULL){mIrrigationMinEditTextPtr->setTextChangeListener(this);}
    mButton34Ptr = (ZKButton*)findControlByID(ID_MAIN_Button34);
    mButton33Ptr = (ZKButton*)findControlByID(ID_MAIN_Button33);
    mButton32Ptr = (ZKButton*)findControlByID(ID_MAIN_Button32);
    mIrrigationHourEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_IrrigationHourEditText);if(mIrrigationHourEditTextPtr!= NULL){mIrrigationHourEditTextPtr->setTextChangeListener(this);}
    mButton31Ptr = (ZKButton*)findControlByID(ID_MAIN_Button31);
    mButton24Ptr = (ZKButton*)findControlByID(ID_MAIN_Button24);
    mCycle2HourEndEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_Cycle2HourEndEditText);if(mCycle2HourEndEditTextPtr!= NULL){mCycle2HourEndEditTextPtr->setTextChangeListener(this);}
    mTextView16Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView16);
    mButton30Ptr = (ZKButton*)findControlByID(ID_MAIN_Button30);
    mTextView15Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView15);
    mCycle2MinStartEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_Cycle2MinStartEditText);if(mCycle2MinStartEditTextPtr!= NULL){mCycle2MinStartEditTextPtr->setTextChangeListener(this);}
    mCycle2HourStartEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_Cycle2HourStartEditText);if(mCycle2HourStartEditTextPtr!= NULL){mCycle2HourStartEditTextPtr->setTextChangeListener(this);}
    mButton29Ptr = (ZKButton*)findControlByID(ID_MAIN_Button29);
    mCycle2MinEndEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_Cycle2MinEndEditText);if(mCycle2MinEndEditTextPtr!= NULL){mCycle2MinEndEditTextPtr->setTextChangeListener(this);}
    mCycle1MinEndEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_Cycle1MinEndEditText);if(mCycle1MinEndEditTextPtr!= NULL){mCycle1MinEndEditTextPtr->setTextChangeListener(this);}
    mTextView14Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView14);
    mCycle1HourEndEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_Cycle1HourEndEditText);if(mCycle1HourEndEditTextPtr!= NULL){mCycle1HourEndEditTextPtr->setTextChangeListener(this);}
    mButton28Ptr = (ZKButton*)findControlByID(ID_MAIN_Button28);
    mButton25Ptr = (ZKButton*)findControlByID(ID_MAIN_Button25);
    mCycle1HourStartEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_Cycle1HourStartEditText);if(mCycle1HourStartEditTextPtr!= NULL){mCycle1HourStartEditTextPtr->setTextChangeListener(this);}
    mTextView13Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView13);
    mCycle1MinStartEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_Cycle1MinStartEditText);if(mCycle1MinStartEditTextPtr!= NULL){mCycle1MinStartEditTextPtr->setTextChangeListener(this);}
    mButton23Ptr = (ZKButton*)findControlByID(ID_MAIN_Button23);
    mCycleWindowPtr = (ZKWindow*)findControlByID(ID_MAIN_CycleWindow);
    mCycleButtonPtr = (ZKButton*)findControlByID(ID_MAIN_CycleButton);
    mUartValueOffButtonPtr = (ZKButton*)findControlByID(ID_MAIN_UartValueOffButton);
    mUartValueOnButtonPtr = (ZKButton*)findControlByID(ID_MAIN_UartValueOnButton);
    mButton22Ptr = (ZKButton*)findControlByID(ID_MAIN_Button22);
    mSenserTestValueListViewPtr = (ZKListView*)findControlByID(ID_MAIN_SenserTestValueListView);if(mSenserTestValueListViewPtr!= NULL){mSenserTestValueListViewPtr->setListAdapter(this);mSenserTestValueListViewPtr->setItemClickListener(this);}
    mSenserTestTipsListViewPtr = (ZKListView*)findControlByID(ID_MAIN_SenserTestTipsListView);if(mSenserTestTipsListViewPtr!= NULL){mSenserTestTipsListViewPtr->setListAdapter(this);mSenserTestTipsListViewPtr->setItemClickListener(this);}
    mButton21Ptr = (ZKButton*)findControlByID(ID_MAIN_Button21);
    mGroupTestValueListViewPtr = (ZKListView*)findControlByID(ID_MAIN_GroupTestValueListView);if(mGroupTestValueListViewPtr!= NULL){mGroupTestValueListViewPtr->setListAdapter(this);mGroupTestValueListViewPtr->setItemClickListener(this);}
    mGroupTestTipsListViewPtr = (ZKListView*)findControlByID(ID_MAIN_GroupTestTipsListView);if(mGroupTestTipsListViewPtr!= NULL){mGroupTestTipsListViewPtr->setListAdapter(this);mGroupTestTipsListViewPtr->setItemClickListener(this);}
    mDeviceTestValueListViewPtr = (ZKListView*)findControlByID(ID_MAIN_DeviceTestValueListView);if(mDeviceTestValueListViewPtr!= NULL){mDeviceTestValueListViewPtr->setListAdapter(this);mDeviceTestValueListViewPtr->setItemClickListener(this);}
    mDeviceTestTipsListViewPtr = (ZKListView*)findControlByID(ID_MAIN_DeviceTestTipsListView);if(mDeviceTestTipsListViewPtr!= NULL){mDeviceTestTipsListViewPtr->setListAdapter(this);mDeviceTestTipsListViewPtr->setItemClickListener(this);}
    mButton20Ptr = (ZKButton*)findControlByID(ID_MAIN_Button20);
    mButton15Ptr = (ZKButton*)findControlByID(ID_MAIN_Button15);
    mW3TipTextViewPtr = (ZKTextView*)findControlByID(ID_MAIN_W3TipTextView);
    mW3TipWindowPtr = (ZKWindow*)findControlByID(ID_MAIN_W3TipWindow);
    mW2TextViewPtr = (ZKTextView*)findControlByID(ID_MAIN_W2TextView);
    mW2TipWindowPtr = (ZKWindow*)findControlByID(ID_MAIN_W2TipWindow);
    mSetRunTimeZeroButtonPtr = (ZKButton*)findControlByID(ID_MAIN_SetRunTimeZeroButton);
    mSetRunTimeCencelButtonPtr = (ZKButton*)findControlByID(ID_MAIN_SetRunTimeCencelButton);
    mSetRunTimeOKButtonPtr = (ZKButton*)findControlByID(ID_MAIN_SetRunTimeOKButton);
    mSecEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_SecEditText);if(mSecEditTextPtr!= NULL){mSecEditTextPtr->setTextChangeListener(this);}
    mButton19Ptr = (ZKButton*)findControlByID(ID_MAIN_Button19);
    mMinEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_MinEditText);if(mMinEditTextPtr!= NULL){mMinEditTextPtr->setTextChangeListener(this);}
    mButton18Ptr = (ZKButton*)findControlByID(ID_MAIN_Button18);
    mButton17Ptr = (ZKButton*)findControlByID(ID_MAIN_Button17);
    mHourEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_HourEditText);if(mHourEditTextPtr!= NULL){mHourEditTextPtr->setTextChangeListener(this);}
    mButton16Ptr = (ZKButton*)findControlByID(ID_MAIN_Button16);
    mSetRunTimeWindowPtr = (ZKWindow*)findControlByID(ID_MAIN_SetRunTimeWindow);
    mRunTimeListViewPtr = (ZKListView*)findControlByID(ID_MAIN_RunTimeListView);if(mRunTimeListViewPtr!= NULL){mRunTimeListViewPtr->setListAdapter(this);mRunTimeListViewPtr->setItemClickListener(this);}
    mOnOffProgTextButtonPtr = (ZKButton*)findControlByID(ID_MAIN_OnOffProgTextButton);
    mAfterDayEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_AfterDayEditText);if(mAfterDayEditTextPtr!= NULL){mAfterDayEditTextPtr->setTextChangeListener(this);}
    mButton27Ptr = (ZKButton*)findControlByID(ID_MAIN_Button27);
    mButton26Ptr = (ZKButton*)findControlByID(ID_MAIN_Button26);
    mIntervalDayEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_IntervalDayEditText);if(mIntervalDayEditTextPtr!= NULL){mIntervalDayEditTextPtr->setTextChangeListener(this);}
    mTuesdayButtonPtr = (ZKButton*)findControlByID(ID_MAIN_TuesdayButton);
    mMondayButtonPtr = (ZKButton*)findControlByID(ID_MAIN_MondayButton);
    mSundayButtonPtr = (ZKButton*)findControlByID(ID_MAIN_SundayButton);
    mEverDayButtonPtr = (ZKButton*)findControlByID(ID_MAIN_EverDayButton);
    mSaturdayButtonPtr = (ZKButton*)findControlByID(ID_MAIN_SaturdayButton);
    mFridayButtonPtr = (ZKButton*)findControlByID(ID_MAIN_FridayButton);
    mThursdayButtonPtr = (ZKButton*)findControlByID(ID_MAIN_ThursdayButton);
    mWednesdayButtonPtr = (ZKButton*)findControlByID(ID_MAIN_WednesdayButton);
    mIntervalModeButtonPtr = (ZKButton*)findControlByID(ID_MAIN_IntervalModeButton);
    mWeekModeButtonPtr = (ZKButton*)findControlByID(ID_MAIN_WeekModeButton);
    mOnOffProgButtonPtr = (ZKButton*)findControlByID(ID_MAIN_OnOffProgButton);
    mNextProgButtonPtr = (ZKButton*)findControlByID(ID_MAIN_NextProgButton);
    mPervProgButtonPtr = (ZKButton*)findControlByID(ID_MAIN_PervProgButton);
    mShowProgEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_ShowProgEditText);if(mShowProgEditTextPtr!= NULL){mShowProgEditTextPtr->setTextChangeListener(this);}
    mButton14Ptr = (ZKButton*)findControlByID(ID_MAIN_Button14);
    mStartTimeMin4EditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_StartTimeMin4EditText);if(mStartTimeMin4EditTextPtr!= NULL){mStartTimeMin4EditTextPtr->setTextChangeListener(this);}
    mButton13Ptr = (ZKButton*)findControlByID(ID_MAIN_Button13);
    mTextView11Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView11);
    mStartTimeHour4EditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_StartTimeHour4EditText);if(mStartTimeHour4EditTextPtr!= NULL){mStartTimeHour4EditTextPtr->setTextChangeListener(this);}
    mStartTimeMin3EditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_StartTimeMin3EditText);if(mStartTimeMin3EditTextPtr!= NULL){mStartTimeMin3EditTextPtr->setTextChangeListener(this);}
    mButton12Ptr = (ZKButton*)findControlByID(ID_MAIN_Button12);
    mTextView10Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView10);
    mStartTimeHour3EditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_StartTimeHour3EditText);if(mStartTimeHour3EditTextPtr!= NULL){mStartTimeHour3EditTextPtr->setTextChangeListener(this);}
    mStartTimeMin2EditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_StartTimeMin2EditText);if(mStartTimeMin2EditTextPtr!= NULL){mStartTimeMin2EditTextPtr->setTextChangeListener(this);}
    mButton11Ptr = (ZKButton*)findControlByID(ID_MAIN_Button11);
    mTextView9Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView9);
    mStartTimeHour2EditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_StartTimeHour2EditText);if(mStartTimeHour2EditTextPtr!= NULL){mStartTimeHour2EditTextPtr->setTextChangeListener(this);}
    mTextView8Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView8);
    mStartTimeMin1EditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_StartTimeMin1EditText);if(mStartTimeMin1EditTextPtr!= NULL){mStartTimeMin1EditTextPtr->setTextChangeListener(this);}
    mStartTimeHour1EditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_StartTimeHour1EditText);if(mStartTimeHour1EditTextPtr!= NULL){mStartTimeHour1EditTextPtr->setTextChangeListener(this);}
    if(mAfterDayEditTextPtr!= NULL){mAfterDayEditTextPtr->setClickListener(this);}
    if(mIntervalDayEditTextPtr!= NULL){mIntervalDayEditTextPtr->setClickListener(this);}
    if(mStartTimeMin4EditTextPtr!= NULL){mStartTimeMin4EditTextPtr->setClickListener(this);}
    if(mStartTimeHour4EditTextPtr!= NULL){mStartTimeHour4EditTextPtr->setClickListener(this);}
    if(mStartTimeMin3EditTextPtr!= NULL){mStartTimeMin3EditTextPtr->setClickListener(this);}
    if(mStartTimeHour3EditTextPtr!= NULL){mStartTimeHour3EditTextPtr->setClickListener(this);}
    if(mStartTimeMin2EditTextPtr!= NULL){mStartTimeMin2EditTextPtr->setClickListener(this);}
    if(mStartTimeHour2EditTextPtr!= NULL){mStartTimeHour2EditTextPtr->setClickListener(this);}
    if(mStartTimeMin1EditTextPtr!= NULL){mStartTimeMin1EditTextPtr->setClickListener(this);}
    if(mStartTimeHour1EditTextPtr!= NULL){mStartTimeHour1EditTextPtr->setClickListener(this);}
    mButton10Ptr = (ZKButton*)findControlByID(ID_MAIN_Button10);
    mGroupBindValueWindowPtr = (ZKWindow*)findControlByID(ID_MAIN_GroupBindValueWindow);
    mSelectSenserListViewPtr = (ZKListView*)findControlByID(ID_MAIN_SelectSenserListView);if(mSelectSenserListViewPtr!= NULL){mSelectSenserListViewPtr->setListAdapter(this);mSelectSenserListViewPtr->setItemClickListener(this);}
    mSelectPumpListViewPtr = (ZKListView*)findControlByID(ID_MAIN_SelectPumpListView);if(mSelectPumpListViewPtr!= NULL){mSelectPumpListViewPtr->setListAdapter(this);mSelectPumpListViewPtr->setItemClickListener(this);}
    mSelectSenserButtonPtr = (ZKButton*)findControlByID(ID_MAIN_SelectSenserButton);
    mSelectPumpButtonPtr = (ZKButton*)findControlByID(ID_MAIN_SelectPumpButton);
    mTextView7Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView7);
    mTextView6Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView6);
    mAddSenserEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_AddSenserEditText);if(mAddSenserEditTextPtr!= NULL){mAddSenserEditTextPtr->setTextChangeListener(this);}
    mAddPumpEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_AddPumpEditText);if(mAddPumpEditTextPtr!= NULL){mAddPumpEditTextPtr->setTextChangeListener(this);}
    mGroupAddButtonPtr = (ZKButton*)findControlByID(ID_MAIN_GroupAddButton);
    mGroupDelButtonPtr = (ZKButton*)findControlByID(ID_MAIN_GroupDelButton);
    mGroupCencelButtonPtr = (ZKButton*)findControlByID(ID_MAIN_GroupCencelButton);
    mTextView5Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView5);
    mGroupNameEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_GroupNameEditText);if(mGroupNameEditTextPtr!= NULL){mGroupNameEditTextPtr->setTextChangeListener(this);}
    mTextView4Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView4);
    mGroupNumEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_GroupNumEditText);if(mGroupNumEditTextPtr!= NULL){mGroupNumEditTextPtr->setTextChangeListener(this);}
    mIrrNum_TextViewPtr = (ZKTextView*)findControlByID(ID_MAIN_IrrNum_TextView);
    mGroupBind_ButtonPtr = (ZKButton*)findControlByID(ID_MAIN_GroupBind_Button);
    mClearIrr_ButtonPtr = (ZKButton*)findControlByID(ID_MAIN_ClearIrr_Button);
    mChangeIrr_ListViewPtr = (ZKListView*)findControlByID(ID_MAIN_ChangeIrr_ListView);if(mChangeIrr_ListViewPtr!= NULL){mChangeIrr_ListViewPtr->setListAdapter(this);mChangeIrr_ListViewPtr->setItemClickListener(this);}
    mW2_CencelButtonPtr = (ZKButton*)findControlByID(ID_MAIN_W2_CencelButton);
    mW2_DelButtonPtr = (ZKButton*)findControlByID(ID_MAIN_W2_DelButton);
    mW2_OkButtonPtr = (ZKButton*)findControlByID(ID_MAIN_W2_OkButton);
    mTextView3Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView3);
    mW2_NameEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_W2_NameEditText);if(mW2_NameEditTextPtr!= NULL){mW2_NameEditTextPtr->setTextChangeListener(this);}
    mTextView1Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView1);
    mW2_AddressEditTextPtr = (ZKEditText*)findControlByID(ID_MAIN_W2_AddressEditText);if(mW2_AddressEditTextPtr!= NULL){mW2_AddressEditTextPtr->setTextChangeListener(this);}
    mw2set_windowPtr = (ZKWindow*)findControlByID(ID_MAIN_w2set_window);
    mDeviceListViewPtr = (ZKListView*)findControlByID(ID_MAIN_DeviceListView);if(mDeviceListViewPtr!= NULL){mDeviceListViewPtr->setListAdapter(this);mDeviceListViewPtr->setItemClickListener(this);}
    mWindow5Ptr = (ZKWindow*)findControlByID(ID_MAIN_Window5);
    mWindow4Ptr = (ZKWindow*)findControlByID(ID_MAIN_Window4);
    mWindow3Ptr = (ZKWindow*)findControlByID(ID_MAIN_Window3);
    mWindow2Ptr = (ZKWindow*)findControlByID(ID_MAIN_Window2);
    mTextView2Ptr = (ZKTextView*)findControlByID(ID_MAIN_TextView2);
    mWaterBarPtr = (ZKTextView*)findControlByID(ID_MAIN_WaterBar);
    mPumpCheckbox4Ptr = (ZKCheckBox*)findControlByID(ID_MAIN_PumpCheckbox4);if(mPumpCheckbox4Ptr!= NULL){mPumpCheckbox4Ptr->setCheckedChangeListener(this);}
    mPumpCheckbox5Ptr = (ZKCheckBox*)findControlByID(ID_MAIN_PumpCheckbox5);if(mPumpCheckbox5Ptr!= NULL){mPumpCheckbox5Ptr->setCheckedChangeListener(this);}
    mPumpCheckbox3Ptr = (ZKCheckBox*)findControlByID(ID_MAIN_PumpCheckbox3);if(mPumpCheckbox3Ptr!= NULL){mPumpCheckbox3Ptr->setCheckedChangeListener(this);}
    mPumpCheckbox2Ptr = (ZKCheckBox*)findControlByID(ID_MAIN_PumpCheckbox2);if(mPumpCheckbox2Ptr!= NULL){mPumpCheckbox2Ptr->setCheckedChangeListener(this);}
    mPumpCheckbox1Ptr = (ZKCheckBox*)findControlByID(ID_MAIN_PumpCheckbox1);if(mPumpCheckbox1Ptr!= NULL){mPumpCheckbox1Ptr->setCheckedChangeListener(this);}
    mwifistatusPtr = (ZKButton*)findControlByID(ID_MAIN_wifistatus);
    mDigitalClock1Ptr = (ZKDigitalClock*)findControlByID(ID_MAIN_DigitalClock1);
    mButton9Ptr = (ZKButton*)findControlByID(ID_MAIN_Button9);
    mWindow1Ptr = (ZKWindow*)findControlByID(ID_MAIN_Window1);
    mButton8Ptr = (ZKButton*)findControlByID(ID_MAIN_Button8);
    mButton7Ptr = (ZKButton*)findControlByID(ID_MAIN_Button7);
    mButton5Ptr = (ZKButton*)findControlByID(ID_MAIN_Button5);
    mButton4Ptr = (ZKButton*)findControlByID(ID_MAIN_Button4);
    mButton3Ptr = (ZKButton*)findControlByID(ID_MAIN_Button3);
    mButton2Ptr = (ZKButton*)findControlByID(ID_MAIN_Button2);
    mButton1Ptr = (ZKButton*)findControlByID(ID_MAIN_Button1);
	mActivityPtr = this;
	onUI_init();
  registerProtocolDataUpdateListener(onProtocolDataUpdate);
  rigesterActivityTimer();
}

void mainActivity::onClick(ZKBase *pBase) {
	//TODO: add widget onClik code 
    if (hideCycleTipIfVisible()) {
        return;
    }
    if (hideW2TipWindowIfVisible()) {
        return;
    }

    if (handlePage3EditTextClick(pBase)) {
        return;
    }

    int buttonTablen = sizeof(sButtonCallbackTab) / sizeof(S_ButtonCallback);
    for (int i = 0; i < buttonTablen; ++i) {
        if (sButtonCallbackTab[i].id == pBase->getID()) {
            if (sButtonCallbackTab[i].callback((ZKButton*)pBase)) {
            	return;
            }
            break;
        }
    }


    int len = sizeof(sAppInfoTab) / sizeof(sAppInfoTab[0]);
    for (int i = 0; i < len; ++i) {
        if (sAppInfoTab[i].id == pBase->getID()) {
            EASYUICONTEXT->openActivity(sAppInfoTab[i].pApp);
            return;
        }
    }

	Activity::onClick(pBase);
}

void mainActivity::onResume() {
	Activity::onResume();
	EASYUICONTEXT->registerGlobalTouchListener(this);
	startVideoLoopPlayback();
	onUI_show();

	//BRIGHTNESSHELPER->screenOn();
}

void mainActivity::onPause() {
	Activity::onPause();
	EASYUICONTEXT->unregisterGlobalTouchListener(this);
	stopVideoLoopPlayback();
	onUI_hide();
	//BRIGHTNESSHELPER->screenOff();
}

void mainActivity::onIntent(const Intent *intentPtr) {
	Activity::onIntent(intentPtr);
	onUI_intent(intentPtr);
}

bool mainActivity::onTimer(int id) {
	return onUI_Timer(id);
}

void mainActivity::onProgressChanged(ZKSeekBar *pSeekBar, int progress){

    int seekBarTablen = sizeof(SZKSeekBarCallbackTab) / sizeof(S_ZKSeekBarCallback);
    for (int i = 0; i < seekBarTablen; ++i) {
        if (SZKSeekBarCallbackTab[i].id == pSeekBar->getID()) {
            SZKSeekBarCallbackTab[i].callback(pSeekBar, progress);
            break;
        }
    }
}

int mainActivity::getListItemCount(const ZKListView *pListView) const{
    // Fast path for DeviceListView to avoid table lookup
    if (pListView == mDeviceListViewPtr) {
        return getListItemCount_DeviceListView(pListView);
    }

    int tablen = sizeof(SListViewFunctionsCallbackTab) / sizeof(S_ListViewFunctionsCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SListViewFunctionsCallbackTab[i].id == pListView->getID()) {
            return SListViewFunctionsCallbackTab[i].getListItemCountCallback(pListView);
            break;
        }
    }
    return 0;
}

void mainActivity::obtainListItemData(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index){
    // Fast path for DeviceListView to avoid table lookup
    if (pListView == mDeviceListViewPtr) {
        obtainListItemData_DeviceListView(pListView, pListItem, index);
        return;
    }

    int tablen = sizeof(SListViewFunctionsCallbackTab) / sizeof(S_ListViewFunctionsCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SListViewFunctionsCallbackTab[i].id == pListView->getID()) {
            SListViewFunctionsCallbackTab[i].obtainListItemDataCallback(pListView, pListItem, index);
            break;
        }
    }
}

void mainActivity::onItemClick(ZKListView *pListView, int index, int id){
    int tablen = sizeof(SListViewFunctionsCallbackTab) / sizeof(S_ListViewFunctionsCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SListViewFunctionsCallbackTab[i].id == pListView->getID()) {
            SListViewFunctionsCallbackTab[i].onItemClickCallback(pListView, index, id);
            break;
        }
    }
}

void mainActivity::onSlideItemClick(ZKSlideWindow *pSlideWindow, int index) {
    int tablen = sizeof(SSlideWindowItemClickCallbackTab) / sizeof(S_SlideWindowItemClickCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SSlideWindowItemClickCallbackTab[i].id == pSlideWindow->getID()) {
            SSlideWindowItemClickCallbackTab[i].onSlideItemClickCallback(pSlideWindow, index);
            break;
        }
    }
}

bool mainActivity::onTouchEvent(const MotionEvent &ev) {
    return onmainActivityTouchEvent(ev);
}

void mainActivity::onTextChanged(ZKTextView *pTextView, const std::string &text) {
    int tablen = sizeof(SEditTextInputCallbackTab) / sizeof(S_EditTextInputCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SEditTextInputCallbackTab[i].id == pTextView->getID()) {
            SEditTextInputCallbackTab[i].onEditTextChangedCallback(text);
            break;
        }
    }
}

void mainActivity::rigesterActivityTimer() {
    int tablen = sizeof(REGISTER_ACTIVITY_TIMER_TAB) / sizeof(S_ACTIVITY_TIMEER);
    for (int i = 0; i < tablen; ++i) {
        S_ACTIVITY_TIMEER temp = REGISTER_ACTIVITY_TIMER_TAB[i];
        registerTimer(temp.id, temp.time);
    }
}


void mainActivity::onVideoPlayerMessage(ZKVideoView *pVideoView, int msg) {
    int tablen = sizeof(SVideoViewCallbackTab) / sizeof(S_VideoViewCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SVideoViewCallbackTab[i].id == pVideoView->getID()) {
        	if (SVideoViewCallbackTab[i].loop) {
                //循环播放
        		videoLoopPlayback(pVideoView, msg, i);
        	} else if (SVideoViewCallbackTab[i].onVideoViewCallback != NULL){
        	    SVideoViewCallbackTab[i].onVideoViewCallback(pVideoView, msg);
        	}
            break;
        }
    }
}

void mainActivity::videoLoopPlayback(ZKVideoView *pVideoView, int msg, size_t callbackTabIndex) {
	switch (msg) {
	case ZKVideoView::E_MSGTYPE_VIDEO_PLAY_STARTED:
		LOGD("ZKVideoView::E_MSGTYPE_VIDEO_PLAY_STARTED\n");
    if (callbackTabIndex >= (sizeof(SVideoViewCallbackTab)/sizeof(S_VideoViewCallback))) {
      break;
    }
		pVideoView->setVolume(SVideoViewCallbackTab[callbackTabIndex].defaultvolume / 10.0);
		mVideoLoopErrorCount = 0;
		break;
	case ZKVideoView::E_MSGTYPE_VIDEO_PLAY_ERROR:
		/**错误处理 */
		++mVideoLoopErrorCount;
		if (mVideoLoopErrorCount > 100) {
			LOGD("video loop error counts > 100, quit loop playback !");
            break;
		} //不用break, 继续尝试播放下一个
	case ZKVideoView::E_MSGTYPE_VIDEO_PLAY_COMPLETED:
		LOGD("ZKVideoView::E_MSGTYPE_VIDEO_PLAY_COMPLETED\n");
        std::vector<std::string> videolist;
        std::string fileName(getAppName());
        if (fileName.size() < 4) {
             LOGD("getAppName size < 4, ignore!");
             break;
        }
        fileName = fileName.substr(0, fileName.length() - 4) + "_video_list.txt";
        fileName = "/mnt/extsd/" + fileName;
        if (!parseVideoFileList(fileName.c_str(), videolist)) {
            LOGD("parseVideoFileList failed !");
		    break;
        }
		if (pVideoView && !videolist.empty()) {
			mVideoLoopIndex = (mVideoLoopIndex + 1) % videolist.size();
			pVideoView->play(videolist[mVideoLoopIndex].c_str());
		}
		break;
	}
}

void mainActivity::startVideoLoopPlayback() {
    int tablen = sizeof(SVideoViewCallbackTab) / sizeof(S_VideoViewCallback);
    for (int i = 0; i < tablen; ++i) {
    	if (SVideoViewCallbackTab[i].loop) {
    		ZKVideoView* videoView = (ZKVideoView*)findControlByID(SVideoViewCallbackTab[i].id);
    		if (!videoView) {
    			return;
    		}
    		//循环播放
    		videoLoopPlayback(videoView, ZKVideoView::E_MSGTYPE_VIDEO_PLAY_COMPLETED, i);
    		return;
    	}
    }
}

void mainActivity::stopVideoLoopPlayback() {
    int tablen = sizeof(SVideoViewCallbackTab) / sizeof(S_VideoViewCallback);
    for (int i = 0; i < tablen; ++i) {
    	if (SVideoViewCallbackTab[i].loop) {
    		ZKVideoView* videoView = (ZKVideoView*)findControlByID(SVideoViewCallbackTab[i].id);
    		if (!videoView) {
    			return;
    		}
    		if (videoView->isPlaying()) {
    		    videoView->stop();
    		}
    		return;
    	}
    }
}

bool mainActivity::parseVideoFileList(const char *pFileListPath, std::vector<string>& mediaFileList) {
	mediaFileList.clear();
	if (NULL == pFileListPath || 0 == strlen(pFileListPath)) {
        LOGD("video file list is null!");
		return false;
	}

	ifstream is(pFileListPath, ios_base::in);
	if (!is.is_open()) {
		LOGD("cann't open file %s \n", pFileListPath);
		return false;
	}
	char tmp[1024] = {0};
	while (is.getline(tmp, sizeof(tmp))) {
		string str = tmp;
		removeCharFromString(str, '\"');
		removeCharFromString(str, '\r');
		removeCharFromString(str, '\n');
		if (str.size() > 1) {
     		mediaFileList.push_back(str.c_str());
		}
	}
  LOGD("(f:%s, l:%d) parse fileList[%s], get [%d]files", __FUNCTION__,
      __LINE__, pFileListPath, int(mediaFileList.size()));
  for (std::vector<string>::size_type i = 0; i < mediaFileList.size(); i++) {
    LOGD("file[%d]:[%s]", int(i), mediaFileList[i].c_str());
  }
	is.close();

	return true;
}

int mainActivity::removeCharFromString(string& nString, char c) {
    string::size_type   pos;
    while(1) {
        pos = nString.find(c);
        if(pos != string::npos) {
            nString.erase(pos, 1);
        } else {
            break;
        }
    }
    return (int)nString.size();
}

void mainActivity::registerUserTimer(int id, int time) {
	registerTimer(id, time);
}

void mainActivity::unregisterUserTimer(int id) {
	unregisterTimer(id);
}

void mainActivity::resetUserTimer(int id, int time) {
	resetTimer(id, time);
}
void mainActivity::onCheckedChanged(ZKCheckBox* pCheckBox, bool isChecked) {
    int tablen = sizeof(SCheckboxCallbackTab) / sizeof(S_CheckboxCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SCheckboxCallbackTab[i].id == pCheckBox->getID()) {
        	SCheckboxCallbackTab[i].onCheckedChanged(pCheckBox, isChecked);
            break;
        }
    }
}
void mainActivity::onCheckedChanged(ZKRadioGroup* pRadioGroup, int checkedID) {
    int tablen = sizeof(SRadioGroupCallbackTab) / sizeof(S_RadioGroupCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SRadioGroupCallbackTab[i].id == pRadioGroup->getID()) {
        	SRadioGroupCallbackTab[i].onCheckedChanged(pRadioGroup, checkedID);
            break;
        }
    }
}
