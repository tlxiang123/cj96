/***********************************************
/gen auto by codex
***********************************************/
#include "LanguageActivity.h"
#include "logic/Cj96I18n.h"

static ZKButton* mLanBtnPtr;
static ZKButton* mButton0Ptr;
static ZKButton* mButton1Ptr;
static ZKButton* mButton2Ptr;
static ZKButton* mButton3Ptr;
static ZKButton* mButton5Ptr;
static ZKWindow* mWindow1Ptr;
static LanguageActivity* mActivityPtr;

REGISTER_ACTIVITY(LanguageActivity);

typedef bool (*ButtonCallback)(ZKButton *pButton);

typedef struct {
    int id;
    ButtonCallback callback;
} S_ButtonCallback;

static void refreshLanguageButtons();
static bool onButtonClick_LanBtn(ZKButton *pButton);
static bool onButtonClick_Button0(ZKButton *pButton);
static bool onButtonClick_Button1(ZKButton *pButton);
static bool onButtonClick_Button2(ZKButton *pButton);
static bool onButtonClick_Button3(ZKButton *pButton);
static bool onButtonClick_Button5(ZKButton *pButton);

static S_ButtonCallback sButtonCallbackTab[] = {
    {ID_LANGUAGE_LanBtn, onButtonClick_LanBtn},
    {ID_LANGUAGE_Button0, onButtonClick_Button0},
    {ID_LANGUAGE_Button1, onButtonClick_Button1},
    {ID_LANGUAGE_Button2, onButtonClick_Button2},
    {ID_LANGUAGE_Button3, onButtonClick_Button3},
    {ID_LANGUAGE_Button5, onButtonClick_Button5},
};

LanguageActivity::LanguageActivity() {
    mVideoLoopIndex = -1;
    mVideoLoopErrorCount = 0;
}

LanguageActivity::~LanguageActivity() {
    EASYUICONTEXT->unregisterGlobalTouchListener(this);
    mLanBtnPtr = NULL;
    mButton0Ptr = NULL;
    mButton1Ptr = NULL;
    mButton2Ptr = NULL;
    mButton3Ptr = NULL;
    mButton5Ptr = NULL;
    mWindow1Ptr = NULL;
}

const char* LanguageActivity::getAppName() const {
    return "Language.ftu";
}

void LanguageActivity::onCreate() {
    Activity::onCreate();
    mLanBtnPtr = (ZKButton*)findControlByID(ID_LANGUAGE_LanBtn);
    mButton0Ptr = (ZKButton*)findControlByID(ID_LANGUAGE_Button0);
    mButton1Ptr = (ZKButton*)findControlByID(ID_LANGUAGE_Button1);
    mButton2Ptr = (ZKButton*)findControlByID(ID_LANGUAGE_Button2);
    mButton3Ptr = (ZKButton*)findControlByID(ID_LANGUAGE_Button3);
    mButton5Ptr = (ZKButton*)findControlByID(ID_LANGUAGE_Button5);
    mWindow1Ptr = (ZKWindow*)findControlByID(ID_LANGUAGE_Window1);
    mActivityPtr = this;
    CJ96_I18N_APPLY("Language.ftu");
    refreshLanguageButtons();
}

void LanguageActivity::onClick(ZKBase *pBase) {
    int buttonCount = sizeof(sButtonCallbackTab) / sizeof(S_ButtonCallback);
    for (int i = 0; i < buttonCount; ++i) {
        if (sButtonCallbackTab[i].id == pBase->getID()) {
            if (sButtonCallbackTab[i].callback((ZKButton*)pBase)) {
                return;
            }
            break;
        }
    }
    Activity::onClick(pBase);
}

void LanguageActivity::onResume() {
    Activity::onResume();
    EASYUICONTEXT->registerGlobalTouchListener(this);
    CJ96_I18N_APPLY("Language.ftu");
    refreshLanguageButtons();
}

void LanguageActivity::onPause() {
    Activity::onPause();
    EASYUICONTEXT->unregisterGlobalTouchListener(this);
}

void LanguageActivity::onIntent(const Intent *intentPtr) {
    Activity::onIntent(intentPtr);
}

bool LanguageActivity::onTimer(int id) {
    return false;
}

void LanguageActivity::onProgressChanged(ZKSeekBar *pSeekBar, int progress) {
}

int LanguageActivity::getListItemCount(const ZKListView *pListView) const {
    return 0;
}

void LanguageActivity::obtainListItemData(ZKListView *pListView, ZKListView::ZKListItem *pListItem, int index) {
}

void LanguageActivity::onItemClick(ZKListView *pListView, int index, int subItemIndex) {
}

void LanguageActivity::onSlideItemClick(ZKSlideWindow *pSlideWindow, int index) {
}

bool LanguageActivity::onTouchEvent(const MotionEvent &ev) {
    return false;
}

void LanguageActivity::onTextChanged(ZKTextView *pTextView, const string &text) {
}

void LanguageActivity::onVideoPlayerMessage(ZKVideoView *pVideoView, int msg) {
}

void LanguageActivity::applyCurrentLanguage() {
    CJ96_I18N_APPLY("Language.ftu");
}

static void refreshLanguageButtons() {
    const int language = Cj96I18n::getLanguage();
    if (mButton0Ptr) mButton0Ptr->setSelected(language == Cj96I18n::LANG_ZH);
    if (mButton1Ptr) mButton1Ptr->setSelected(language == Cj96I18n::LANG_EN);
    if (mButton3Ptr) mButton3Ptr->setSelected(language == Cj96I18n::LANG_DE);
    if (mButton2Ptr) mButton2Ptr->setSelected(language == Cj96I18n::LANG_FR);
    if (mButton5Ptr) mButton5Ptr->setSelected(language == Cj96I18n::LANG_JA);
}

static void selectLanguage(int language) {
    Cj96I18n::setLanguage(language);
    if (mActivityPtr) {
        mActivityPtr->applyCurrentLanguage();
    }
    refreshLanguageButtons();
}

static bool onButtonClick_LanBtn(ZKButton *pButton) {
    EASYUICONTEXT->goBack();
    return true;
}

static bool onButtonClick_Button0(ZKButton *pButton) {
    selectLanguage(Cj96I18n::LANG_ZH);
    return true;
}

static bool onButtonClick_Button1(ZKButton *pButton) {
    selectLanguage(Cj96I18n::LANG_EN);
    return true;
}

static bool onButtonClick_Button2(ZKButton *pButton) {
    selectLanguage(Cj96I18n::LANG_FR);
    return true;
}

static bool onButtonClick_Button3(ZKButton *pButton) {
    selectLanguage(Cj96I18n::LANG_DE);
    return true;
}

static bool onButtonClick_Button5(ZKButton *pButton) {
    selectLanguage(Cj96I18n::LANG_JA);
    return true;
}
