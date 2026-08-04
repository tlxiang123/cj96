#include "utils/ScreenHelper.h"
#include "utils/Log.h"

static const char* kGlobalScreenshotSavePath = "/mnt/extsd/cj96_screenshot.bmp";

static bool onButtonClick_GlobalScreenshotButton(ZKButton *pButton) {
    LOGD(" GlobalScreenshotButton save to %s !!!\n", kGlobalScreenshotSavePath);
    const bool ok = ScreenHelper::screenShot(kGlobalScreenshotSavePath);
    LOGD(" GlobalScreenshotButton result = %d !!!\n", ok ? 1 : 0);
    return false;
}
