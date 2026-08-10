#include "DisplayPowerManager.h"

#include "entry/EasyUIContext.h"
#include "utils/BrightnessHelper.h"
#include <cstdio>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace {

const int kMaxTimeoutSeconds = 3600;
const int kDefaultTimeoutSeconds = 300;
const char* kTuyaCommandDirectory = "/mnt/extsd/tuya_demo";
const char* kTuyaScreenPowerStatePath = "/mnt/extsd/tuya_demo/screen_power_state";

int sTimeoutSeconds = -1;
int sConfiguredTimeoutSeconds = 0;
long long sLastActivityMs = 0;
bool sTimeoutEnabled = false;
bool sScreenOffByTimer = false;
bool sInitialized = false;
int sLastWrittenScreenState = -1;

int normalizeTimeout(int seconds) {
    if (seconds <= 0) {
        return -1;
    }
    return seconds > kMaxTimeoutSeconds ? kMaxTimeoutSeconds : seconds;
}

long long nowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<long long>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

void resetIdleCounter() {
    sLastActivityMs = nowMs();
}

void writeScreenPowerState(bool screenOff) {
    const int state = screenOff ? 1 : 0;
    if (sLastWrittenScreenState == state && access(kTuyaScreenPowerStatePath, F_OK) == 0) {
        return;
    }

    (void)mkdir(kTuyaCommandDirectory, 0755);
    FILE* fp = fopen(kTuyaScreenPowerStatePath, "wb");
    if (!fp) {
        return;
    }
    fprintf(fp, "%s\n", screenOff ? "sleep" : "wake");
    fclose(fp);
    sLastWrittenScreenState = state;
}

void disableContextScreensaver() {
    EASYUICONTEXT->setScreensaverEnable(false);
}

void applyTimeoutToContext() {
    if (sTimeoutEnabled && sConfiguredTimeoutSeconds > 0) {
        sTimeoutSeconds = sConfiguredTimeoutSeconds;
    } else {
        sTimeoutSeconds = -1;
    }
    disableContextScreensaver();
    resetIdleCounter();
}

}  // namespace

namespace DisplayPowerManager {

void syncFromContext() {
    if (!sInitialized) {
        sConfiguredTimeoutSeconds = kDefaultTimeoutSeconds;
        sTimeoutEnabled = true;
        sTimeoutSeconds = kDefaultTimeoutSeconds;
        disableContextScreensaver();
        sInitialized = true;
    }
    disableContextScreensaver();
    resetIdleCounter();
    sScreenOffByTimer = !BRIGHTNESSHELPER->isScreenOn();
    writeScreenPowerState(sScreenOffByTimer);
}

void setTimeoutSeconds(int seconds) {
    setConfiguredTimeoutSeconds(seconds);
    setTimeoutEnabled(sConfiguredTimeoutSeconds > 0);
}

int getTimeoutSeconds() {
    return sTimeoutSeconds > 0 ? sTimeoutSeconds : 0;
}

void setConfiguredTimeoutSeconds(int seconds) {
    const int normalized = normalizeTimeout(seconds);
    sConfiguredTimeoutSeconds = normalized > 0 ? normalized : 0;
    if (sTimeoutEnabled && sConfiguredTimeoutSeconds <= 0) {
        sTimeoutEnabled = false;
    }
    applyTimeoutToContext();
}

int getConfiguredTimeoutSeconds() {
    return sConfiguredTimeoutSeconds;
}

void setTimeoutEnabled(bool enabled) {
    sTimeoutEnabled = enabled && sConfiguredTimeoutSeconds > 0;
    applyTimeoutToContext();
}

bool isTimeoutEnabled() {
    return sTimeoutEnabled;
}

bool sleepScreen() {
    BRIGHTNESSHELPER->screenOff();
    sScreenOffByTimer = !BRIGHTNESSHELPER->isScreenOn();
    writeScreenPowerState(sScreenOffByTimer);
    resetIdleCounter();
    return sScreenOffByTimer;
}

bool wakeScreen() {
    BRIGHTNESSHELPER->screenOn();
    sScreenOffByTimer = !BRIGHTNESSHELPER->isScreenOn();
    writeScreenPowerState(sScreenOffByTimer);
    resetIdleCounter();
    return !sScreenOffByTimer;
}

bool onOneSecondTimer() {
    const bool screenOff = !BRIGHTNESSHELPER->isScreenOn();
    if (sScreenOffByTimer != screenOff) {
        sScreenOffByTimer = screenOff;
    }
    writeScreenPowerState(sScreenOffByTimer);

    if (!sTimeoutEnabled || sTimeoutSeconds <= 0) {
        return true;
    }

    if (!BRIGHTNESSHELPER->isScreenOn()) {
        sScreenOffByTimer = true;
        return true;
    }

    const long long elapsedMs = nowMs() - sLastActivityMs;
    if (elapsedMs >= static_cast<long long>(sTimeoutSeconds) * 1000) {
        sleepScreen();
    }

    return true;
}

bool handleTouchEvent() {
    const bool needWake = sScreenOffByTimer || !BRIGHTNESSHELPER->isScreenOn();
    if (needWake) {
        wakeScreen();
        return true;
    }

    resetIdleCounter();
    return false;
}

}  // namespace DisplayPowerManager
