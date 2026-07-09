#include "DisplayPowerManager.h"

#include "entry/EasyUIContext.h"
#include "utils/BrightnessHelper.h"

#include <sys/time.h>

namespace {

const int kMaxTimeoutSeconds = 3600;

int sTimeoutSeconds = -1;
int sConfiguredTimeoutSeconds = 0;
long long sLastActivityMs = 0;
bool sTimeoutEnabled = false;
bool sScreenOffByTimer = false;
bool sInitialized = false;

int normalizeTimeout(int seconds) {
    if (seconds <= 0) {
        return -1;
    }
    return seconds > kMaxTimeoutSeconds ? kMaxTimeoutSeconds : seconds;
}

long long nowMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<long long>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

void resetIdleCounter() {
    sLastActivityMs = nowMs();
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
        sConfiguredTimeoutSeconds = 0;
        sTimeoutEnabled = false;
        sTimeoutSeconds = -1;
        disableContextScreensaver();
        sInitialized = true;
    }
    disableContextScreensaver();
    resetIdleCounter();
    sScreenOffByTimer = !BRIGHTNESSHELPER->isScreenOn();
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

bool onOneSecondTimer() {
    if (!sTimeoutEnabled || sTimeoutSeconds <= 0) {
        return true;
    }

    if (!BRIGHTNESSHELPER->isScreenOn()) {
        sScreenOffByTimer = true;
        return true;
    }

    const long long elapsedMs = nowMs() - sLastActivityMs;
    if (elapsedMs >= static_cast<long long>(sTimeoutSeconds) * 1000) {
        BRIGHTNESSHELPER->screenOff();
        sScreenOffByTimer = true;
        resetIdleCounter();
    }

    return true;
}

bool handleTouchEvent() {
    const bool needWake = sScreenOffByTimer || !BRIGHTNESSHELPER->isScreenOn();
    if (needWake) {
        BRIGHTNESSHELPER->screenOn();
        sScreenOffByTimer = false;
        resetIdleCounter();
        return true;
    }

    resetIdleCounter();
    return false;
}

}  // namespace DisplayPowerManager
