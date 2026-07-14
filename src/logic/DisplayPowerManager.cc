#include "DisplayPowerManager.h"

#include "entry/EasyUIContext.h"
#include "utils/BrightnessHelper.h"
#include "utils/Log.h"
#include "utils/ScreenHelper.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/time.h>

namespace {

const int kMaxTimeoutSeconds = 3600;

int sTimeoutSeconds = -1;
int sConfiguredTimeoutSeconds = 0;
long long sLastActivityMs = 0;
bool sTimeoutEnabled = false;
bool sScreenOffByTimer = false;
bool sInitialized = false;
unsigned int sScreenshotSequence = 0;

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

void normalizePageTag(const char* pageTag, char* output, size_t outputSize) {
    if (!output || outputSize == 0) {
        return;
    }

    size_t outputIndex = 0;
    if (pageTag) {
        for (size_t i = 0; pageTag[i] != '\0' && outputIndex + 1 < outputSize; ++i) {
            const char ch = pageTag[i];
            const bool isAlphaNumeric = (ch >= 'a' && ch <= 'z')
                    || (ch >= 'A' && ch <= 'Z')
                    || (ch >= '0' && ch <= '9');
            output[outputIndex++] = (isAlphaNumeric || ch == '-' || ch == '_') ? ch : '_';
        }
    }

    if (outputIndex == 0) {
        const char fallback[] = "page";
        const size_t fallbackLength = sizeof(fallback) - 1;
        const size_t copyLength = fallbackLength < outputSize - 1
                ? fallbackLength : outputSize - 1;
        memcpy(output, fallback, copyLength);
        outputIndex = copyLength;
    }
    output[outputIndex] = '\0';
}

bool saveScreenshot(const char* pageTag) {
    struct timespec now;
    memset(&now, 0, sizeof(now));
    clock_gettime(CLOCK_REALTIME, &now);

    struct tm localTime;
    memset(&localTime, 0, sizeof(localTime));
    localtime_r(&now.tv_sec, &localTime);

    char normalizedPageTag[48] = {0};
    normalizePageTag(pageTag, normalizedPageTag, sizeof(normalizedPageTag));

    const unsigned int sequence = ++sScreenshotSequence;
    char fileName[128] = {0};
    snprintf(fileName, sizeof(fileName),
             "screenshot_%04d%02d%02d_%02d%02d%02d_%03ld_%04u_%s.bmp",
             localTime.tm_year + 1900,
             localTime.tm_mon + 1,
             localTime.tm_mday,
             localTime.tm_hour,
             localTime.tm_min,
             localTime.tm_sec,
             now.tv_nsec / 1000000L,
             sequence,
             normalizedPageTag);

    static const char* kScreenshotDirectories[] = {
        "/mnt/extsd",
        "/data",
        "/tmp",
    };
    for (size_t i = 0;
         i < sizeof(kScreenshotDirectories) / sizeof(kScreenshotDirectories[0]);
         ++i) {
        char savePath[192] = {0};
        snprintf(savePath, sizeof(savePath), "%s/%s",
                 kScreenshotDirectories[i], fileName);
        if (ScreenHelper::screenShot(savePath)) {
            LOGD("[Screenshot] saved: %s\n", savePath);
            return true;
        }
        LOGD("[Screenshot] save failed: %s\n", savePath);
    }

    LOGE("[Screenshot] all save paths failed\n");
    return false;
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

bool captureScreenshotNow(const char* pageTag) {
    return saveScreenshot(pageTag);
}

}  // namespace DisplayPowerManager
