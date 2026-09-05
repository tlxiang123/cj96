#include "DisplayPowerManager.h"

#include "entry/EasyUIContext.h"
#include "utils/BrightnessHelper.h"
#include <cstdio>
#include <cstdarg>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace {

const int kMaxTimeoutSeconds = 3600;
const int kDefaultTimeoutSeconds = 300;
const char* kTuyaCommandDirectory = "/tmp/cj96_tuya_demo";
const char* kTuyaScreenPowerStatePath = "/tmp/cj96_tuya_demo/screen_power_state";
struct PersistentLogTarget {
    const char* directory;
    const char* path;
    const char* backupPath;
};

// Keep diagnostic logs on the external storage. The internal /data partition
// is only 384 KiB and must remain reserved for system/runtime configuration.
const PersistentLogTarget kPersistentLogTargets[] = {
    {"/mnt/extsd/cj96_logs", "/mnt/extsd/cj96_logs/display_power.log",
     "/mnt/extsd/cj96_logs/display_power.log.1"},
};
const int kPersistentLogTargetCount =
    static_cast<int>(sizeof(kPersistentLogTargets) / sizeof(kPersistentLogTargets[0]));
const long long kPersistentLogMaxBytes = 256 * 1024;

int sTimeoutSeconds = -1;
int sConfiguredTimeoutSeconds = 0;
long long sLastActivityMs = 0;
bool sTimeoutEnabled = false;
bool sScreenOffByTimer = false;
bool sInitialized = false;
int sLastWrittenScreenState = -1;
unsigned int sTimerTickCount = 0;

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

void appendPersistentLog(const char* format, ...) {
    struct timespec wallTs;
    clock_gettime(CLOCK_REALTIME, &wallTs);
    struct tm localTm;
    localtime_r(&wallTs.tv_sec, &localTm);
    char wallText[32] = {0};
    strftime(wallText, sizeof(wallText), "%Y-%m-%d %H:%M:%S", &localTm);

    char message[384] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // Write the same record to every available persistent target. A missing
    // or unavailable external-storage mirror must not prevent /data logging.
    for (int i = 0; i < kPersistentLogTargetCount; ++i) {
        const PersistentLogTarget& target = kPersistentLogTargets[i];
        (void)mkdir(target.directory, 0755);

        struct stat logStat;
        if (stat(target.path, &logStat) == 0 &&
                logStat.st_size >= kPersistentLogMaxBytes) {
            (void)rename(target.path, target.backupPath);
        }

        FILE* fp = fopen(target.path, "ab");
        if (!fp) {
            continue;
        }

        fprintf(fp, "[%s.%03ld][mono=%lld] %s\n",
                wallText,
                wallTs.tv_nsec / 1000000L,
                nowMs(),
                message);
        (void)fflush(fp);
        (void)fsync(fileno(fp));
        (void)fclose(fp);
    }
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
    const bool firstInitialization = !sInitialized;
    if (firstInitialization) {
        sConfiguredTimeoutSeconds = kDefaultTimeoutSeconds;
        sTimeoutEnabled = true;
        sTimeoutSeconds = kDefaultTimeoutSeconds;
        disableContextScreensaver();
        sInitialized = true;
        // The initial process state is treated as screen-on without querying
        // the LCD driver. Later page visits must preserve the tracked state.
        sScreenOffByTimer = false;
        writeScreenPowerState(false);
        appendPersistentLog("display power manager initialized timeout=%d", sTimeoutSeconds);
    }
    disableContextScreensaver();
    resetIdleCounter();
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
    if (sScreenOffByTimer) {
        resetIdleCounter();
        return true;
    }

    const long long startMs = nowMs();
    appendPersistentLog("sleepScreen begin");
    fprintf(stderr, " DisplayPowerManager sleepScreen begin\n");
    BRIGHTNESSHELPER->screenOff();
    sScreenOffByTimer = true;
    writeScreenPowerState(true);
    resetIdleCounter();
    const long long elapsedMs = nowMs() - startMs;
    appendPersistentLog("sleepScreen completed elapsed_ms=%lld", elapsedMs);
    fprintf(stderr, " DisplayPowerManager sleepScreen completed in %lld ms\n",
         elapsedMs);
    return true;
}

bool wakeScreen() {
    if (!sScreenOffByTimer) {
        resetIdleCounter();
        return true;
    }

    const long long startMs = nowMs();
    appendPersistentLog("wakeScreen begin");
    fprintf(stderr, " DisplayPowerManager wakeScreen begin\n");
    BRIGHTNESSHELPER->screenOn();
    sScreenOffByTimer = false;
    writeScreenPowerState(false);
    resetIdleCounter();
    const long long elapsedMs = nowMs() - startMs;
    appendPersistentLog("wakeScreen completed elapsed_ms=%lld", elapsedMs);
    fprintf(stderr, " DisplayPowerManager wakeScreen completed in %lld ms\n",
         elapsedMs);
    return true;
}

bool onOneSecondTimer() {
    ++sTimerTickCount;
    if ((sTimerTickCount % 10U) == 0U) {
        appendPersistentLog("heartbeat tick=%u timeout_enabled=%d screen_off=%d idle_ms=%lld",
                            sTimerTickCount,
                            sTimeoutEnabled ? 1 : 0,
                            sScreenOffByTimer ? 1 : 0,
                            sLastActivityMs > 0 ? nowMs() - sLastActivityMs : -1LL);
    }

    if (!sTimeoutEnabled || sTimeoutSeconds <= 0 || sScreenOffByTimer) {
        return true;
    }

    const long long elapsedMs = nowMs() - sLastActivityMs;
    if (elapsedMs >= static_cast<long long>(sTimeoutSeconds) * 1000) {
        sleepScreen();
    }

    return true;
}

bool handleTouchEvent() {
    if (sScreenOffByTimer) {
        appendPersistentLog("touch while screen_off: wake requested");
        wakeScreen();
        return true;
    }

    resetIdleCounter();
    return false;
}
}  // namespace DisplayPowerManager
