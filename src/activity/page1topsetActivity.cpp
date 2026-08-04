/***********************************************
/gen auto by zuitools
***********************************************/
#include "page1topsetActivity.h"

/*TAG:GlobalVariable全局变量*/
static ZKTextView* mTextView7Ptr;
static ZKButton* msys_backPtr;
static ZKWindow* mnettopwindPtr;
static ZKButton* mButton3Ptr;
static ZKButton* mButton2Ptr;
static ZKButton* mLanBtnPtr;
static ZKButton* mDisplayBtnPtr;
static ZKButton* mSetSysTimeBtnPtr;
static ZKButton* mOpen4GButtonPtr;
static ZKButton* mOpenNetButtonPtr;
static ZKButton* mOpenWifiButtonPtr;
static ZKWindow* mDebugPasswordWindowPtr;
static ZKTextView* mDebugPasswordTipTextPtr;
static ZKEditText* mDebugPasswordEditTextPtr;
static ZKButton* mDebugPasswordOkButtonPtr;
static ZKButton* mDebugPasswordCancelButtonPtr;
static ZKButton* mDebugBtnPtr;
static page1topsetActivity* mActivityPtr;

/*register activity*/
REGISTER_ACTIVITY(page1topsetActivity);



typedef struct {
	int id; // 定时器ID ， 不能重复
	int time; // 定时器  时间间隔  单位 毫秒
}S_ACTIVITY_TIMEER;

#include "logic/page1topsetLogic.cc"
#include "logic/globalScreenshotLogic.cc"

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
    20120, onButtonClick_GlobalScreenshotButton,
    ID_PAGE1TOPSET_DebugBtn, onButtonClick_DebugBtn,
    ID_PAGE1TOPSET_DebugPasswordOkButton, onButtonClick_DebugPasswordOkButton,
    ID_PAGE1TOPSET_DebugPasswordCancelButton, onButtonClick_DebugPasswordCancelButton,
    ID_PAGE1TOPSET_sys_back, onButtonClick_sys_back,
    ID_PAGE1TOPSET_Button3, onButtonClick_Button3,
    ID_PAGE1TOPSET_Button2, onButtonClick_Button2,
    ID_PAGE1TOPSET_LanBtn, onButtonClick_LanBtn,
    ID_PAGE1TOPSET_DisplayBtn, onButtonClick_DisplayBtn,
    ID_PAGE1TOPSET_SetSysTimeBtn, onButtonClick_SetSysTimeBtn,
    ID_PAGE1TOPSET_Open4GButton, onButtonClick_Open4GButton,
    ID_PAGE1TOPSET_OpenNetButton, onButtonClick_OpenNetButton,
    ID_PAGE1TOPSET_OpenWifiButton, onButtonClick_OpenWifiButton,
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


page1topsetActivity::page1topsetActivity() {
	//todo add init code here
	mVideoLoopIndex = -1;
	mVideoLoopErrorCount = 0;
}

page1topsetActivity::~page1topsetActivity() {
  //todo add init file here
  // 退出应用时需要反注册
    EASYUICONTEXT->unregisterGlobalTouchListener(this);
    unregisterProtocolDataUpdateListener(onProtocolDataUpdate);
    onUI_quit();
    mActivityPtr = NULL;
    mTextView7Ptr = NULL;
    msys_backPtr = NULL;
    mnettopwindPtr = NULL;
    mButton3Ptr = NULL;
    mButton2Ptr = NULL;
    mLanBtnPtr = NULL;
    mDisplayBtnPtr = NULL;
    mSetSysTimeBtnPtr = NULL;
    mOpen4GButtonPtr = NULL;
    mOpenNetButtonPtr = NULL;
    mOpenWifiButtonPtr = NULL;
    mDebugPasswordWindowPtr = NULL;
    mDebugPasswordTipTextPtr = NULL;
    mDebugPasswordEditTextPtr = NULL;
    mDebugPasswordOkButtonPtr = NULL;
    mDebugPasswordCancelButtonPtr = NULL;
    mDebugBtnPtr = NULL;
}

const char* page1topsetActivity::getAppName() const{
	return "page1topset.ftu";
}

//TAG:onCreate
void page1topsetActivity::onCreate() {
	Activity::onCreate();
    mTextView7Ptr = (ZKTextView*)findControlByID(ID_PAGE1TOPSET_TextView7);
    msys_backPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_sys_back);
    mnettopwindPtr = (ZKWindow*)findControlByID(ID_PAGE1TOPSET_nettopwind);
    mButton3Ptr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_Button3);
    mButton2Ptr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_Button2);
    mLanBtnPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_LanBtn);
    mDisplayBtnPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_DisplayBtn);
    mSetSysTimeBtnPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_SetSysTimeBtn);
    mOpen4GButtonPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_Open4GButton);
    mOpenNetButtonPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_OpenNetButton);
    mOpenWifiButtonPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_OpenWifiButton);
    mDebugPasswordWindowPtr = (ZKWindow*)findControlByID(ID_PAGE1TOPSET_DebugPasswordWindow);
    mDebugPasswordTipTextPtr = (ZKTextView*)findControlByID(ID_PAGE1TOPSET_DebugPasswordTipText);
    mDebugPasswordEditTextPtr = (ZKEditText*)findControlByID(ID_PAGE1TOPSET_DebugPasswordEditText);
    mDebugPasswordOkButtonPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_DebugPasswordOkButton);
    mDebugPasswordCancelButtonPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_DebugPasswordCancelButton);
    mDebugBtnPtr = (ZKButton*)findControlByID(ID_PAGE1TOPSET_DebugBtn);
	mActivityPtr = this;
	onUI_init();
  registerProtocolDataUpdateListener(onProtocolDataUpdate);
  rigesterActivityTimer();
}

void page1topsetActivity::onClick(ZKBase *pBase) {
	//TODO: add widget onClik code 
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

void page1topsetActivity::onResume() {
	Activity::onResume();
	EASYUICONTEXT->registerGlobalTouchListener(this);
	startVideoLoopPlayback();
	onUI_show();
}

void page1topsetActivity::onPause() {
	Activity::onPause();
	EASYUICONTEXT->unregisterGlobalTouchListener(this);
	stopVideoLoopPlayback();
	onUI_hide();
}

void page1topsetActivity::onIntent(const Intent *intentPtr) {
	Activity::onIntent(intentPtr);
	onUI_intent(intentPtr);
}

bool page1topsetActivity::onTimer(int id) {
	return onUI_Timer(id);
}

void page1topsetActivity::onProgressChanged(ZKSeekBar *pSeekBar, int progress){

    int seekBarTablen = sizeof(SZKSeekBarCallbackTab) / sizeof(S_ZKSeekBarCallback);
    for (int i = 0; i < seekBarTablen; ++i) {
        if (SZKSeekBarCallbackTab[i].id == pSeekBar->getID()) {
            SZKSeekBarCallbackTab[i].callback(pSeekBar, progress);
            break;
        }
    }
}

int page1topsetActivity::getListItemCount(const ZKListView *pListView) const{
    int tablen = sizeof(SListViewFunctionsCallbackTab) / sizeof(S_ListViewFunctionsCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SListViewFunctionsCallbackTab[i].id == pListView->getID()) {
            return SListViewFunctionsCallbackTab[i].getListItemCountCallback(pListView);
            break;
        }
    }
    return 0;
}

void page1topsetActivity::obtainListItemData(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index){
    int tablen = sizeof(SListViewFunctionsCallbackTab) / sizeof(S_ListViewFunctionsCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SListViewFunctionsCallbackTab[i].id == pListView->getID()) {
            SListViewFunctionsCallbackTab[i].obtainListItemDataCallback(pListView, pListItem, index);
            break;
        }
    }
}

void page1topsetActivity::onItemClick(ZKListView *pListView, int index, int id){
    int tablen = sizeof(SListViewFunctionsCallbackTab) / sizeof(S_ListViewFunctionsCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SListViewFunctionsCallbackTab[i].id == pListView->getID()) {
            SListViewFunctionsCallbackTab[i].onItemClickCallback(pListView, index, id);
            break;
        }
    }
}

void page1topsetActivity::onSlideItemClick(ZKSlideWindow *pSlideWindow, int index) {
    int tablen = sizeof(SSlideWindowItemClickCallbackTab) / sizeof(S_SlideWindowItemClickCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SSlideWindowItemClickCallbackTab[i].id == pSlideWindow->getID()) {
            SSlideWindowItemClickCallbackTab[i].onSlideItemClickCallback(pSlideWindow, index);
            break;
        }
    }
}

bool page1topsetActivity::onTouchEvent(const MotionEvent &ev) {
    return onpage1topsetActivityTouchEvent(ev);
}

void page1topsetActivity::onTextChanged(ZKTextView *pTextView, const std::string &text) {
    int tablen = sizeof(SEditTextInputCallbackTab) / sizeof(S_EditTextInputCallback);
    for (int i = 0; i < tablen; ++i) {
        if (SEditTextInputCallbackTab[i].id == pTextView->getID()) {
            SEditTextInputCallbackTab[i].onEditTextChangedCallback(text);
            break;
        }
    }
}

void page1topsetActivity::rigesterActivityTimer() {
    int tablen = sizeof(REGISTER_ACTIVITY_TIMER_TAB) / sizeof(S_ACTIVITY_TIMEER);
    for (int i = 0; i < tablen; ++i) {
        S_ACTIVITY_TIMEER temp = REGISTER_ACTIVITY_TIMER_TAB[i];
        registerTimer(temp.id, temp.time);
    }
}


void page1topsetActivity::onVideoPlayerMessage(ZKVideoView *pVideoView, int msg) {
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

void page1topsetActivity::videoLoopPlayback(ZKVideoView *pVideoView, int msg, size_t callbackTabIndex) {

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

void page1topsetActivity::startVideoLoopPlayback() {
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

void page1topsetActivity::stopVideoLoopPlayback() {
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

bool page1topsetActivity::parseVideoFileList(const char *pFileListPath, std::vector<string>& mediaFileList) {
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

int page1topsetActivity::removeCharFromString(string& nString, char c) {
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

void page1topsetActivity::registerUserTimer(int id, int time) {
	registerTimer(id, time);
}

void page1topsetActivity::unregisterUserTimer(int id) {
	unregisterTimer(id);
}

void page1topsetActivity::resetUserTimer(int id, int time) {
	resetTimer(id, time);
}
