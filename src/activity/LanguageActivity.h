/***********************************************
/gen auto by codex
***********************************************/
#ifndef __LANGUAGEACTIVITY_H__
#define __LANGUAGEACTIVITY_H__

#include "app/Activity.h"
#include "entry/EasyUIContext.h"
#include "utils/Log.h"
#include "control/ZKButton.h"
#include "control/ZKTextView.h"
#include "control/ZKSeekBar.h"
#include "control/ZKEditText.h"
#include "control/ZKListView.h"
#include "control/ZKVideoView.h"
#include "window/ZKWindow.h"
#include "window/ZKSlideWindow.h"

#define ID_LANGUAGE_Button1    20001
#define ID_LANGUAGE_Button2    20002
#define ID_LANGUAGE_Button0    20003
#define ID_LANGUAGE_Button3    20004
#define ID_LANGUAGE_LanBtn     20005
#define ID_LANGUAGE_Button5    20007
#define ID_LANGUAGE_Window1    110001

class LanguageActivity : public Activity,
                         public ZKSeekBar::ISeekBarChangeListener,
                         public ZKListView::IItemClickListener,
                         public ZKListView::AbsListAdapter,
                         public ZKSlideWindow::ISlideItemClickListener,
                         public EasyUIContext::ITouchListener,
                         public ZKEditText::ITextChangeListener,
                         public ZKVideoView::IVideoPlayerMessageListener {
public:
    LanguageActivity();
    virtual ~LanguageActivity();
    void applyCurrentLanguage();

protected:
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
    virtual void onVideoPlayerMessage(ZKVideoView *pVideoView, int msg);

private:
    int mVideoLoopIndex;
    int mVideoLoopErrorCount;
};

#endif
