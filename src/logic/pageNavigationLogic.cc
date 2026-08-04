static int sCurrentPageIndex = 0;
static bool sCycleWindowOpen = false;
static int sCycleReturnPageIndex = BACK_GROUND_BTN_3;

static bool isMainTabWithoutBack(int pageIndex) {
    return pageIndex == BACK_GROUND_BTN_1
            || pageIndex == BACK_GROUND_BTN_2
            || pageIndex == BACK_GROUND_BTN_3
            || pageIndex == BACK_GROUND_BTN_4;
}

static void setMainSysBackVisible(bool visible) {
    if (mMainSysBackPtr) {
        mMainSysBackPtr->setVisible(visible);
    }
}

static void updateMainSysBackVisibilityForPage(int pageIndex) {
    setMainSysBackVisible(!isMainTabWithoutBack(pageIndex));
}

static void notifyPageHide(int pageIndex) {
    switch (pageIndex) {
    case BACK_GROUND_BTN_1: onPage1Hide(); break;
    case BACK_GROUND_BTN_2: onPage2Hide(); break;
    case BACK_GROUND_BTN_3: onPage3Hide(); break;
    case BACK_GROUND_BTN_4: onPage4Hide(); break;
    case BACK_GROUND_BTN_5: onPage5Hide(); break;
    case BACK_GROUND_BTN_6: onPage6Hide(); break;
    case BACK_GROUND_BTN_7: onPage7Hide(); break;
    case BACK_GROUND_BTN_8: onPage8Hide(); break;
    default: break;
    }
}

static void notifyPageShow(int pageIndex) {
    switch (pageIndex) {
    case BACK_GROUND_BTN_1: onPage1Show(); break;
    case BACK_GROUND_BTN_2: onPage2Show(); break;
    case BACK_GROUND_BTN_3: onPage3Show(); break;
    case BACK_GROUND_BTN_4: onPage4Show(); break;
    case BACK_GROUND_BTN_5: onPage5Show(); break;
    case BACK_GROUND_BTN_6: onPage6Show(); break;
    case BACK_GROUND_BTN_7: onPage7Show(); break;
    case BACK_GROUND_BTN_8: onPage8Show(); break;
    default: break;
    }
}

static void hideCycleWindowOnly(bool restoreReturnPage = true) {
    if (sCycleWindowOpen) {
        onCycleWindowHide();
    }
    if (mCycleWindowPtr) {
        mCycleWindowPtr->hideWnd();
    }
    sCycleWindowOpen = false;

    if (!restoreReturnPage
            || sCycleReturnPageIndex < BACK_GROUND_BTN_1
            || sCycleReturnPageIndex > BACK_GROUND_BTN_8) {
        return;
    }

    ZKButton* buttons[] = {
        NULL,
        mButton1Ptr,
        mButton2Ptr,
        mButton3Ptr,
        mButton4Ptr,
        mButton5Ptr,
        NULL,
        mButton7Ptr,
        mButton8Ptr,
    };
    ZKWindow* windows[] = {
        NULL,
        mWindow1Ptr,
        mWindow2Ptr,
        mWindow3Ptr,
        mWindow4Ptr,
        mWindow5Ptr,
        NULL,
        NULL,
        NULL,
    };

    for (int i = BACK_GROUND_BTN_1; i <= BACK_GROUND_BTN_8; ++i) {
        if (buttons[i]) {
            buttons[i]->setSelected(false);
        }
        if (windows[i]) {
            windows[i]->hideWnd();
        }
    }

    if (buttons[sCycleReturnPageIndex]) {
        buttons[sCycleReturnPageIndex]->setSelected(true);
    }
    if (windows[sCycleReturnPageIndex]) {
        windows[sCycleReturnPageIndex]->showWnd();
    }

    sCurrentPageIndex = sCycleReturnPageIndex;
    updateMainSysBackVisibilityForPage(sCurrentPageIndex);
    notifyPageShow(sCurrentPageIndex);
}

static void showMainPage(int pageIndex) {
    ZKButton* buttons[] = {
        NULL,
        mButton1Ptr,
        mButton2Ptr,
        mButton3Ptr,
        mButton4Ptr,
        mButton5Ptr,
        NULL,
        mButton7Ptr,
        mButton8Ptr,
    };
    ZKWindow* windows[] = {
        NULL,
        mWindow1Ptr,
        mWindow2Ptr,
        mWindow3Ptr,
        mWindow4Ptr,
        mWindow5Ptr,
        NULL,
        NULL,
        NULL,
    };

    if (pageIndex < BACK_GROUND_BTN_1 || pageIndex > BACK_GROUND_BTN_8) {
        return;
    }

    hideCycleWindowOnly(false);

    if (sCurrentPageIndex == BACK_GROUND_BTN_2
            && sCurrentPageIndex != pageIndex
            && !handlePage2BeforeMainPageSwitch(pageIndex)) {
        return;
    }

    if (sCurrentPageIndex != 0 && sCurrentPageIndex != pageIndex) {
        notifyPageHide(sCurrentPageIndex);
    }

    for (int i = BACK_GROUND_BTN_1; i <= BACK_GROUND_BTN_8; ++i) {
        if (buttons[i]) {
            buttons[i]->setSelected(false);
        }
        if (windows[i]) {
            windows[i]->hideWnd();
        }
    }

    if (buttons[pageIndex]) {
        buttons[pageIndex]->setSelected(true);
    }
    if (windows[pageIndex]) {
        windows[pageIndex]->showWnd();
    }

    sCurrentPageIndex = pageIndex;
    updateMainSysBackVisibilityForPage(pageIndex);
    notifyPageShow(pageIndex);
}

static void showCycleWindow() {
    ZKButton* buttons[] = {
        NULL,
        mButton1Ptr,
        mButton2Ptr,
        mButton3Ptr,
        mButton4Ptr,
        mButton5Ptr,
        NULL,
        mButton7Ptr,
        mButton8Ptr,
    };
    ZKWindow* windows[] = {
        NULL,
        mWindow1Ptr,
        mWindow2Ptr,
        mWindow3Ptr,
        mWindow4Ptr,
        mWindow5Ptr,
        NULL,
        NULL,
        NULL,
    };

    if (sCurrentPageIndex >= BACK_GROUND_BTN_1 && sCurrentPageIndex <= BACK_GROUND_BTN_8) {
        notifyPageHide(sCurrentPageIndex);
    }
    sCycleReturnPageIndex = BACK_GROUND_BTN_3;

    for (int i = BACK_GROUND_BTN_1; i <= BACK_GROUND_BTN_8; ++i) {
        if (buttons[i]) {
            buttons[i]->setSelected(false);
        }
        if (windows[i]) {
            windows[i]->hideWnd();
        }
    }

    if (mCycleWindowPtr) {
        mCycleWindowPtr->showWnd();
    }
    sCurrentPageIndex = 0;
    sCycleWindowOpen = true;
    setMainSysBackVisible(true);
    onCycleWindowShow();
}

static void initMainPageNavigation() {
    sCurrentPageIndex = 0;
    showMainPage(BACK_GROUND_BTN_1);
}

static bool handleButtonClick_Button1(ZKButton *pButton) {
    showMainPage(BACK_GROUND_BTN_1);
    return false;
}

static bool handleButtonClick_Button2(ZKButton *pButton) {
    showMainPage(BACK_GROUND_BTN_2);
    return false;
}

static bool handleButtonClick_Button3(ZKButton *pButton) {
    showMainPage(BACK_GROUND_BTN_3);
    return false;
}

static bool handleButtonClick_Button4(ZKButton *pButton) {
    showMainPage(BACK_GROUND_BTN_4);
    return false;
}

static bool handleButtonClick_Button5(ZKButton *pButton) {
    showMainPage(BACK_GROUND_BTN_5);
    return false;
}

static bool handleButtonClick_Button7(ZKButton *pButton) {
    showMainPage(BACK_GROUND_BTN_7);
    return false;
}

static bool handleButtonClick_Button8(ZKButton *pButton) {
    if (sCurrentPageIndex == BACK_GROUND_BTN_2) {
        handlePage2BeforeMainPageSwitch(BACK_GROUND_BTN_8);
    }
    return false;
}

static bool handleButtonClick_Button9(ZKButton *pButton) {
    if (sCurrentPageIndex == BACK_GROUND_BTN_2
            && !handlePage2BeforeMainPageSwitch(BACK_GROUND_BTN_1)) {
        return false;
    }
    EASYUICONTEXT->openActivity("page1topsetActivity");
    return false;
}
