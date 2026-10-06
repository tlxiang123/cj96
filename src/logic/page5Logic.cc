// Page5 logic.
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <deque>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <termio.h>
#include <unistd.h>
#include <vector>
#include "Cj96I18n.h"
#include "DeviceDiscoveryTiming.h"

static const BYTE WINDOW5_RSP_ACK = 0x80U;
static const BYTE WINDOW5_RSP_NACK = 0x7FU;
static const BYTE WINDOW5_CMD_SET_ADDRESS = 0x40U;
static const BYTE WINDOW5_CMD_SET_CONFIG = 0x42U;
static const BYTE WINDOW5_CMD_GET_CONFIG = 0x43U;
static const BYTE WINDOW5_CMD_GET_DEVICE_STATE = 0x44U;
static const BYTE WINDOW5_CMD_SET_VALVE_STATE = 0x45U;
static const BYTE WINDOW5_CMD_DISCOVER_DEVICES = 0x46U;
static const BYTE WINDOW5_DECODER_TYPE_VALUE = 1U;
static const BYTE WINDOW5_DECODER_TYPE_SENSER = 2U;
static const BYTE WINDOW5_DEVICE_ADDRESS_MIN = 1U;
static const int WINDOW5_DEVICE_ADDRESS_MAX = 255;
static const BYTE WINDOW5_CONFIG_ADDRESS_MIN = cj96_discovery::ADDRESS_MIN;
static const int WINDOW5_CONFIG_ADDRESS_MAX = cj96_discovery::ADDRESS_MAX;
static const int WINDOW5_DEFAULT_UNCONFIGURED_ADDRESS = 8888;
static const BYTE WINDOW5_PROTOCOL_MAX_DATA_LEN = 32U;
static const int WINDOW5_RSP_WAIT_MS = 300;
static const int WINDOW5_VALVE_RSP_WAIT_MS = 4000;
static const int WINDOW5_VALVE_RSP_WAIT_LOOPS = WINDOW5_VALVE_RSP_WAIT_MS / 10;
// Throttle every 0x45 command, including individual AC open/close and
// group valve operations; AC boards acknowledge before actuation settles.
static const int WINDOW5_VALVE_COMMAND_GAP_MS = 2500;
static const size_t WINDOW5_QUEUE_MAX = 255U;
static const size_t WINDOW5_DEVICE_NAME_MAX = 32U;
// Query each possible address independently so one slow/noisy node cannot
// collide with replies from other boards. Missed addresses get one retry pass.
static const int WINDOW5_DISCOVERY_PROBE_WAIT_MS = cj96_discovery::PROBE_WAIT_MS;
static const int WINDOW5_DISCOVERY_RETRY_PASSES = cj96_discovery::RETRY_PASSES;
static const int WINDOW5_DISCOVERY_RETRY_WAIT_MS = cj96_discovery::RETRY_WAIT_MS;

struct SWindow5Rs485Result {
    int replyType;
    BYTE status;
    int returnedAddress;
    BYTE returnedDecoderType;
    bool hasReturnedDecoderType;
    BYTE returnedDeviceState;
    bool hasReturnedDeviceState;
    bool hasSensorValue;
    int sensorRawValue;
    BYTE sensorDecimals;
    BYTE sensorUnit;
    bool sendOk;
};

static long long getWindow5MonotonicMs() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return -1; // Never mix wall-clock time with a monotonic deadline.
    }
    return (static_cast<long long>(ts.tv_sec) * 1000LL) +
           (static_cast<long long>(ts.tv_nsec) / 1000000LL);
}

struct SWindow5Rs485Request {
    BYTE cmd;
    BYTE dataLen;
    BYTE data[WINDOW5_PROTOCOL_MAX_DATA_LEN];
    char frameName[16];
    bool trackDeviceState;
    int targetAddress;
    bool discoverDevices;
    bool valveCommand;
    bool urgentCommand;
    bool groupValveCommand;
    int groupNo;
};

struct SWindow5DeviceStateUpdate {
    int targetAddress;
    SWindow5Rs485Result result;
    bool valveCommand;
};

struct SWindow5DiscoveredDevice {
    int address;
    BYTE decoderType;
    BYTE deviceState;
    bool addressConflict;
};

static pthread_mutex_t sWindow5QueueMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sWindow5QueueCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t sWindow5RouteMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sWindow5IoMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sWindow5StateUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static std::deque<SWindow5Rs485Request> sWindow5RequestQueue;
static std::deque<SWindow5DeviceStateUpdate> sWindow5StateUpdateQueue;
static std::vector<SWindow5DiscoveredDevice> sWindow5DiscoveryResults;
static pthread_t sWindow5WorkerThread;
static bool sWindow5WorkerStarted = false;
static char sWindow5KnownDevice[WINDOW5_DEVICE_NAME_MAX] = {0};
static int sWindow5NextDevicePollIndex = 0;
// Runtime sensor values change slowly and each 0x44 query occupies the
// shared bus, so a sensor is polled at most once per interval.  Valves keep
// their original per-slot polling because their state can change at any
// moment.
static const long long WINDOW5_SENSOR_POLL_INTERVAL_MS = 5000LL;
static std::vector<long long> sWindow5SensorLastPollMs;
// A node that stops answering must not be allowed to slow the whole shared
// bus down.  After this many consecutive unanswered polls the address enters
// a long backoff: it is skipped (and shown disconnected) until the deadline
// passes, so one broken decoder cannot starve the healthy ones.
static const int WINDOW5_FAULT_FAIL_LIMIT = 3;
static const long long WINDOW5_FAULT_BACKOFF_MS = 60000LL;
static int sWindow5AddressFailCount[256] = {0};
static long long sWindow5AddressBackoffUntilMs[256] = {0};
static bool sWindow5DiscoveryRunning = false;
static bool sWindow5DiscoveryCompleted = false;
static bool sWindow5DiscoveryScanCompleted = false;
static unsigned int sWindow5LastDiscoveryBusDeviceCount = 0U;
static bool sWindow5LastDiscoveryBusDeviceCountValid = false;
static long long sWindow5LastDiscoveryScanId = 0LL;
static std::vector<int> sWindow5LastDiscoveryUnaddedAddresses;
// A Window2 add-device address check is a short exclusive RS485 transaction.
// While it is active, normal 0x44 state polling must not enqueue or consume
// traffic, otherwise an old sensor reply can be mistaken for the add check.
static bool sWindow5ManualConfigActive = false;
static volatile bool sWindow5UrgentNoReplyPending = false;
static bool sWindow5ValveCommandBusy = false;
static int sWindow5ValveCommandPendingCount = 0;
static bool sWindow5ValveCommandHadFailure = false;
static BYTE sWindow5ValveCommandFinalState = 0xFFU;
static BYTE sWindow5ValveCommandTargetState = 0xFFU;
static bool sWindow5ValveCommandIsGroup = false;
static int sWindow5ValveCommandGroupNo = 0;
static int sWindow5ValveCommandTargetAddress = -1;
static unsigned int sWindow5ValveCommandCompletionSerial = 0;
static bool sWindow5ValveCommandLastSucceeded = true;
static long long sWindow5ValveSuccessTipHideDeadlineMs = 0;
static bool sWindow5TypePopupVisible = false;
static BYTE sWindow5SelectedDecoderLabelType = WINDOW5_DECODER_TYPE_VALUE;
static const char *sWindow5SelectedDecoderLabel = "电磁阀";

static SWindow5Rs485Result sendWindow5Rs485CommandDetailedSync(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName);
static void* window5Rs485Worker(void *arg);
static bool ensureWindow5Rs485Worker();
static bool isWindow5DeviceDiscoveryActive();
static bool isWindow5ManualConfigActive();
static bool beginWindow5ManualConfigTransaction();
static void endWindow5ManualConfigTransaction();
static bool enqueueWindow5Rs485Command(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName);
static void setWindow5KnownDevice(const char *pFileName);
static void getWindow5KnownDevice(char *pOut, size_t outSize);
static bool getWindow5SelectedDecoderType(BYTE *pDecoderType);
static bool parseWindow5TestAddressEditText(int *pAddress);
static bool parseWindow5ValveAddressEditText(int *pAddress);
static void setWindow5TestAddressTip(const char *pText);
static void setWindow5TestAddressSuccessTip(const char *pText);
static void setWindow5TestAddressFailureTip(const char *pText);
static void updateWindow5TestAddressTipAutoHide();
static bool isWindow5ValveCommandBusy();
static unsigned int getWindow5ValveCommandCompletionSerial();
static bool wasLastWindow5ValveCommandSuccessful();
static void showWindow5ValveWaitTip();
static void addWindow5ValveCommandPending(BYTE targetState,
                                          bool groupCommand,
                                          int groupNo,
                                          int targetAddress);
static void finishWindow5ValveCommandWait(const SWindow5Rs485Result &result,
                                          int expectedAddress);
static bool checkWindow5ValveAddressReady(int address, BYTE decoderType);
static bool sendWindow5ManualValveStateCommand(bool open);
static bool requestWindow5DeviceStateByAddress(int address);
static long long getWindow5NowMs();
static bool requestWindow5GroupDevicesState(int groupNo, bool open,
                                             bool includeValves,
                                             bool includePumps);
static bool sendWindow5ForceSetAddressCommand();
static bool checkWindow5AddressOccupied(int address, SWindow5Rs485Result *pResult);
static void hideWindow5TypePopupOnly();
static void updateWindow5DecoderTypeTitle();
static bool isWindow5ManagedDecoderDevice(const SDATA *data);
static void window5MarkAddressFault(int address, bool failed);
static bool window5IsAddressInBackoff(int address);
static void window5ClearAllAddressFaults();

static SWindow5Rs485Result makeWindow5Rs485Result() {
    SWindow5Rs485Result result;
    result.replyType = 0;
    result.status = 0xFFU;
    result.returnedAddress = -1;
    result.returnedDecoderType = 0xFFU;
    result.hasReturnedDecoderType = false;
    result.returnedDeviceState = 0xFFU;
    result.hasReturnedDeviceState = false;
    result.hasSensorValue = false;
    result.sensorRawValue = 0;
    result.sensorDecimals = 0xFFU;
    result.sensorUnit = 0xFFU;
    result.sendOk = false;
    return result;
}

static void putWindow5Address(BYTE *pData, int address) {
    if (pData == NULL) {
        return;
    }
    pData[0] = static_cast<BYTE>((address >> 8) & 0xFF);
    pData[1] = static_cast<BYTE>(address & 0xFF);
}

static int getWindow5Address(const BYTE *pData) {
    if (pData == NULL) {
        return -1;
    }
    return (static_cast<int>(pData[0]) << 8) | static_cast<int>(pData[1]);
}

static void dumpWindow5Hex(const BYTE *pData, UINT len) {
    char buf[256];
    UINT pos = 0;

    for (UINT i = 0; (i < len) && (pos + 4U < sizeof(buf)); ++i) {
        const int n = snprintf(buf + pos, sizeof(buf) - pos, "%s%02X",
                               (i == 0U) ? "" : " ", pData[i]);
        if (n <= 0) {
            break;
        }
        pos += (UINT)n;
    }
    buf[(pos < sizeof(buf)) ? pos : (sizeof(buf) - 1U)] = '\0';
    LOGD("[Window5Rs485] rx %u bytes: %s\r\n", len, buf);
}

static bool configureWindow5Rs485Port(int fd) {
    struct termios newtio = { 0 };

    newtio.c_cflag = B2400 | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = 0;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 1;

    tcflush(fd, TCIOFLUSH);
    return tcsetattr(fd, TCSANOW, &newtio) == 0;
}

static bool writeWindow5Rs485Frame(int fd, const BYTE *pData, UINT len) {
    UINT writtenLen = 0;
    int retryCount = 0;

    while (writtenLen < len) {
        const int ret = write(fd, pData + writtenLen, len - writtenLen);
        if (ret > 0) {
            writtenLen += ret;
            retryCount = 0;
            continue;
        }

        if ((ret < 0) && ((errno == EINTR) || (errno == EAGAIN))) {
            if (++retryCount > 20) {
                LOGD("[Window5Rs485] write retry timeout, written=%u/%u\r\n", writtenLen, len);
                return false;
            }
            usleep(10000);
            continue;
        }

        LOGD("[Window5Rs485] write failed errno=%d(%s), written=%u/%u\r\n",
             errno, strerror(errno), writtenLen, len);
        return false;
    }

    return tcdrain(fd) == 0;
}

static void setWindow5KnownDevice(const char *pFileName) {
    if (pFileName == NULL) {
        return;
    }

    pthread_mutex_lock(&sWindow5RouteMutex);
    memset(sWindow5KnownDevice, 0, sizeof(sWindow5KnownDevice));
    strncpy(sWindow5KnownDevice, pFileName, sizeof(sWindow5KnownDevice) - 1U);
    pthread_mutex_unlock(&sWindow5RouteMutex);
}

static void getWindow5KnownDevice(char *pOut, size_t outSize) {
    if ((pOut == NULL) || (outSize == 0U)) {
        return;
    }

    pthread_mutex_lock(&sWindow5RouteMutex);
    strncpy(pOut, sWindow5KnownDevice, outSize - 1U);
    pOut[outSize - 1U] = '\0';
    pthread_mutex_unlock(&sWindow5RouteMutex);
}

static int parseWindow5Rs485Reply(const BYTE *pData,
                                  UINT len,
                                  BYTE expectedCmd,
                                  int expectedAddress,
                                  BYTE *pStatus,
                                  int *pReturnedAddress,
                                  BYTE *pReturnedDecoderType,
                                  bool *pHasReturnedDecoderType,
                                  BYTE *pReturnedDeviceState,
                                  bool *pHasReturnedDeviceState,
                                  bool *pHasSensorValue,
                                  int *pSensorRawValue,
                                  BYTE *pSensorDecimals,
                                  BYTE *pSensorUnit) {
    if (pStatus != NULL) {
        *pStatus = 0xFFU;
    }
    if (pReturnedAddress != NULL) {
        *pReturnedAddress = -1;
    }
    if (pReturnedDecoderType != NULL) {
        *pReturnedDecoderType = 0xFFU;
    }
    if (pHasReturnedDecoderType != NULL) {
        *pHasReturnedDecoderType = false;
    }
    if (pReturnedDeviceState != NULL) {
        *pReturnedDeviceState = 0xFFU;
    }
    if (pHasReturnedDeviceState != NULL) {
        *pHasReturnedDeviceState = false;
    }
    if (pHasSensorValue != NULL) {
        *pHasSensorValue = false;
    }
    if (pSensorRawValue != NULL) {
        *pSensorRawValue = 0;
    }
    if (pSensorDecimals != NULL) {
        *pSensorDecimals = 0xFFU;
    }
    if (pSensorUnit != NULL) {
        *pSensorUnit = 0xFFU;
    }

    if ((pData == NULL) || (len < 7U)) {
        return 0;
    }

    for (UINT i = 0; (i + 7U) <= len; ++i) {
        if ((pData[i] != 0x55U) || (pData[i + 1U] != 0xAAU)) {
            continue;
        }

        const BYTE cmd = pData[i + 2U];
        const BYTE dataLen = pData[i + 3U];
        const UINT frameLen = 5U + dataLen;
        if (dataLen < 2U || dataLen > WINDOW5_PROTOCOL_MAX_DATA_LEN ||
            (cmd != WINDOW5_RSP_ACK && cmd != WINDOW5_RSP_NACK)) {
            continue;
        }
        if (frameLen > (len - i)) {
            // A damaged length/header must not hide a later complete frame.
            continue;
        }

        BYTE checksum = cmd + dataLen;
        for (UINT j = 0; j < dataLen; ++j) {
            checksum += pData[i + 4U + j];
        }

        if (checksum != pData[i + 4U + dataLen]) {
            continue;
        }

        if (pData[i + 4U] != expectedCmd) {
            continue;
        }
        if (expectedAddress >= 0 && dataLen >= 4U &&
            getWindow5Address(&pData[i + 6U]) != expectedAddress) {
            // New firmware returns the target address. Older valve boards
            // may return a short ACK without address/state; keep those ACKs
            // compatible, but never accept a reply carrying another address.
            continue;
        }

        if (pStatus != NULL) {
            *pStatus = pData[i + 5U];
        }
        if ((pReturnedAddress != NULL) && (dataLen >= 4U)) {
            *pReturnedAddress = getWindow5Address(&pData[i + 6U]);
        }
        if (dataLen >= 5U) {
            if (pReturnedDecoderType != NULL) {
                *pReturnedDecoderType = pData[i + 8U];
            }
            if (pHasReturnedDecoderType != NULL) {
                *pHasReturnedDecoderType = true;
            }
        }
        if (dataLen >= 6U) {
            if (pReturnedDeviceState != NULL) {
                *pReturnedDeviceState = pData[i + 9U];
            }
            if (pHasReturnedDeviceState != NULL) {
                *pHasReturnedDeviceState = true;
            }
        }
        if (dataLen >= 10U) {
            if (pSensorRawValue != NULL) {
                int raw = (static_cast<int>(pData[i + 10U]) << 8) |
                          static_cast<int>(pData[i + 11U]);
                if ((raw & 0x8000) != 0) {
                    raw -= 0x10000;
                }
                *pSensorRawValue = raw;
            }
            if (pSensorDecimals != NULL) {
                *pSensorDecimals = pData[i + 12U];
            }
            if (pSensorUnit != NULL) {
                *pSensorUnit = pData[i + 13U];
            }
            if (pHasSensorValue != NULL) {
                *pHasSensorValue = (pData[i + 12U] <= 3U) && (pData[i + 13U] <= 8U);
            }
        }

        if (cmd == WINDOW5_RSP_ACK) {
            return 1;
        }
        if (cmd == WINDOW5_RSP_NACK) {
            return 2;
        }
    }

    return 0;
}

static int waitWindow5Rs485Reply(int fd,
                                  BYTE expectedCmd,
                                  int waitMs,
                                  int expectedAddress,
                                  BYTE *pStatus,
                                  int *pReturnedAddress,
                                  BYTE *pReturnedDecoderType,
                                  bool *pHasReturnedDecoderType,
                                  BYTE *pReturnedDeviceState,
                                  bool *pHasReturnedDeviceState,
                                  bool *pHasSensorValue,
                                  int *pSensorRawValue,
                                  BYTE *pSensorDecimals,
                                  BYTE *pSensorUnit,
                                  bool *pIoError = NULL) {
    if (pIoError != NULL) *pIoError = false;
    BYTE rxBuf[256] = {0};
    UINT rxLen = 0;
    const long long startedMs = getWindow5MonotonicMs();
    if (startedMs < 0) {
        if (pIoError != NULL) *pIoError = true;
        return 0;
    }
    const long long deadlineMs = startedMs + ((waitMs < 0) ? 0 : waitMs);

    while (true) {
        if (sWindow5UrgentNoReplyPending) {
            LOGD("[Window5Rs485] abort reply wait for urgent no-reply command\r\n");
            return 0;
        }
        const long long nowMs = getWindow5MonotonicMs();
        if (nowMs < 0) {
            if (pIoError != NULL) *pIoError = true;
            return 0;
        }
        const long long remainingMs = deadlineMs - nowMs;
        if (remainingMs <= 0) {
            break;
        }

        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        const int pollMs = (remainingMs > 10LL) ? 10 : static_cast<int>(remainingMs);
        const int pollRet = poll(&pfd, 1, pollMs);
        if (pollRet < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (pIoError != NULL) *pIoError = true;
            LOGD("[Window5Rs485] ack poll failed errno=%d(%s)\r\n", errno, strerror(errno));
            return 0;
        }
        if (pollRet == 0) {
            continue;
        }
        if ((pfd.revents & (POLLNVAL | POLLERR | POLLHUP)) != 0) {
            if (pIoError != NULL) *pIoError = true;
            LOGD("[Window5Rs485] ack poll invalid fd\r\n");
            return 0;
        }
        if ((pfd.revents & POLLIN) == 0) {
            if ((pfd.revents & (POLLERR | POLLHUP)) != 0) {
                return 0;
            }
            continue;
        }

        while (rxLen < sizeof(rxBuf)) {
            const int ret = read(fd, rxBuf + rxLen, sizeof(rxBuf) - rxLen);
            if (ret > 0) {
                rxLen += static_cast<UINT>(ret);
                continue;
            }
            if ((ret < 0) &&
                ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))) {
                break;
            }
            if (ret < 0) {
                if (pIoError != NULL) *pIoError = true;
                return 0;
            }
            break;
        }

        const int replyType = parseWindow5Rs485Reply(rxBuf, rxLen, expectedCmd,
                                                      expectedAddress, pStatus,
                                                      pReturnedAddress,
                                                      pReturnedDecoderType,
                                                      pHasReturnedDecoderType,
                                                      pReturnedDeviceState,
                                                      pHasReturnedDeviceState,
                                                      pHasSensorValue,
                                                      pSensorRawValue,
                                                      pSensorDecimals,
                                                      pSensorUnit);
        if (replyType != 0) {
            if (expectedAddress < 0) dumpWindow5Hex(rxBuf, rxLen);
            return replyType;
        }
        if (rxLen == sizeof(rxBuf)) {
            // Retain a possible fragmented frame at the buffer boundary.
            const UINT keep = WINDOW5_PROTOCOL_MAX_DATA_LEN + 4U;
            memmove(rxBuf, rxBuf + rxLen - keep, keep);
            rxLen = keep;
        }
    }

    return 0;
}

static bool sendWindow5Rs485Device(const char *pFileName,
                                   const BYTE *pFrame, UINT frameLen,
                                   const char *pFrameName,
                                   bool updateKnownRoute,
                                   SWindow5Rs485Result *pResult) {
    LOGD("[Window5Rs485] try %s baud=2400, frame=%s\r\n", pFileName, pFrameName);

    const int fd = open(pFileName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        LOGD("[Window5Rs485] open %s failed errno=%d(%s)\r\n", pFileName, errno, strerror(errno));
        return false;
    }

    if (!configureWindow5Rs485Port(fd)) {
        LOGD("[Window5Rs485] config %s failed errno=%d(%s)\r\n", pFileName, errno, strerror(errno));
        close(fd);
        return false;
    }

    const bool sendOk = writeWindow5Rs485Frame(fd, pFrame, frameLen);
    if (pResult != NULL) {
        pResult->sendOk = sendOk;
    }
    if (!sendOk) {
        LOGD("[Window5Rs485] send %s %s FAIL\r\n", pFileName, pFrameName);
        close(fd);
        return false;
    }

    BYTE replyStatus = 0xFFU;
    int returnedAddress = -1;
    BYTE returnedDecoderType = 0xFFU;
    bool hasReturnedDecoderType = false;
    BYTE returnedDeviceState = 0xFFU;
    bool hasReturnedDeviceState = false;
    bool hasSensorValue = false;
    int sensorRawValue = 0;
    BYTE sensorDecimals = 0xFFU;
    BYTE sensorUnit = 0xFFU;
    const int replyWaitMs = (pFrame[2] == WINDOW5_CMD_SET_VALVE_STATE) ?
        WINDOW5_VALVE_RSP_WAIT_MS : WINDOW5_RSP_WAIT_MS;
    const int expectedReplyAddress =
        (pFrame[2] == WINDOW5_CMD_SET_VALVE_STATE && pFrame[3] >= 2U) ?
        getWindow5Address(&pFrame[4]) : -1;
    const int replyType = waitWindow5Rs485Reply(fd, pFrame[2], replyWaitMs,
                                                expectedReplyAddress,
                                                &replyStatus,
                                                &returnedAddress,
                                                &returnedDecoderType,
                                                &hasReturnedDecoderType,
                                                &returnedDeviceState,
                                                &hasReturnedDeviceState,
                                                &hasSensorValue,
                                                &sensorRawValue,
                                                &sensorDecimals,
                                                &sensorUnit);
    if (pResult != NULL) {
        pResult->replyType = replyType;
        pResult->status = replyStatus;
        pResult->returnedAddress = returnedAddress;
        pResult->returnedDecoderType = returnedDecoderType;
        pResult->hasReturnedDecoderType = hasReturnedDecoderType;
        pResult->returnedDeviceState = returnedDeviceState;
        pResult->hasReturnedDeviceState = hasReturnedDeviceState;
        pResult->hasSensorValue = hasSensorValue;
        pResult->sensorRawValue = sensorRawValue;
        pResult->sensorDecimals = sensorDecimals;
        pResult->sensorUnit = sensorUnit;
    }
    if (replyType == 1) {
        if (hasReturnedDeviceState) {
            LOGD("[Window5Rs485] send %s %s OK, reply=ACK, status=%u, address=%u, decoderType=%u, state=%u\r\n",
                 pFileName, pFrameName, replyStatus, returnedAddress,
                 returnedDecoderType, returnedDeviceState);
        } else if (hasReturnedDecoderType) {
            LOGD("[Window5Rs485] send %s %s OK, reply=ACK, status=%u, address=%u, decoderType=%u\r\n",
                 pFileName, pFrameName, replyStatus, returnedAddress, returnedDecoderType);
        } else if (returnedAddress >= 0) {
            LOGD("[Window5Rs485] send %s %s OK, reply=ACK, status=%u, address=%u\r\n",
                 pFileName, pFrameName, replyStatus, returnedAddress);
        } else {
            LOGD("[Window5Rs485] send %s %s OK, reply=ACK, status=%u\r\n", pFileName, pFrameName, replyStatus);
        }
        if (updateKnownRoute) {
            setWindow5KnownDevice(pFileName);
        }
        close(fd);
        return true;
    }

    if (replyType == 2) {
        if (hasReturnedDeviceState) {
            LOGD("[Window5Rs485] send %s %s OK, reply=NACK, status=%u, address=%u, decoderType=%u, state=%u\r\n",
                 pFileName, pFrameName, replyStatus, returnedAddress,
                 returnedDecoderType, returnedDeviceState);
        } else if (hasReturnedDecoderType) {
            LOGD("[Window5Rs485] send %s %s OK, reply=NACK, status=%u, address=%u, decoderType=%u\r\n",
                 pFileName, pFrameName, replyStatus, returnedAddress, returnedDecoderType);
        } else if (returnedAddress >= 0) {
            LOGD("[Window5Rs485] send %s %s OK, reply=NACK, status=%u, address=%u\r\n",
                 pFileName, pFrameName, replyStatus, returnedAddress);
        } else {
            LOGD("[Window5Rs485] send %s %s OK, reply=NACK, status=%u\r\n", pFileName, pFrameName, replyStatus);
        }
        if (updateKnownRoute) {
            setWindow5KnownDevice(pFileName);
        }
        close(fd);
        return true;
    }

    LOGD("[Window5Rs485] send %s %s OK, reply timeout\r\n", pFileName, pFrameName);
    close(fd);
    return false;
}

static SWindow5Rs485Result sendWindow5Rs485CommandDetailedUnlocked(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName) {
    static const char* kUartDeviceList[] = {
        "/dev/ttyS2",
        "/dev/ttyS1",
        "/dev/ttyS3",
    };
    SWindow5Rs485Result result = makeWindow5Rs485Result();
    BYTE frame[WINDOW5_PROTOCOL_MAX_DATA_LEN + 5U] = {0};
    BYTE checksum = cmd + dataLen;

    if (dataLen > WINDOW5_PROTOCOL_MAX_DATA_LEN) {
        LOGD("[Window5Rs485] data too long, frame=%s len=%u\r\n", pFrameName, dataLen);
        return result;
    }
    if ((dataLen > 0U) && (pData == NULL)) {
        LOGD("[Window5Rs485] data null, frame=%s len=%u\r\n", pFrameName, dataLen);
        return result;
    }

    frame[0] = 0x55U;
    frame[1] = 0xAAU;
    frame[2] = cmd;
    frame[3] = dataLen;
    for (BYTE i = 0; i < dataLen; ++i) {
        frame[4U + i] = pData[i];
        checksum += pData[i];
    }
    frame[4U + dataLen] = checksum;
    const UINT frameLen = 5U + dataLen;

    const int total = sizeof(kUartDeviceList) / sizeof(kUartDeviceList[0]);
    LOGD("[Window5Rs485] scan send start, candidate=%d, baud=2400, frame=%s cmd=0x%02X len=%u\r\n",
         total, pFrameName, cmd, dataLen);

    char knownDevice[WINDOW5_DEVICE_NAME_MAX] = {0};
    getWindow5KnownDevice(knownDevice, sizeof(knownDevice));
    if (knownDevice[0] != '\0') {
        LOGD("[Window5Rs485] try cached route -> %s\r\n", knownDevice);
        SWindow5Rs485Result routeResult = makeWindow5Rs485Result();
        if (sendWindow5Rs485Device(knownDevice, frame, frameLen, pFrameName, false, &routeResult)) {
            LOGD("[Window5Rs485] scan matched cached route=%s frame=%s\r\n", knownDevice, pFrameName);
            return routeResult;
        }
        if (routeResult.sendOk) {
            result = routeResult;
        }
        LOGD("[Window5Rs485] cached route failed, fall back to scan\r\n");
    }

    for (int i = 0; i < total; ++i) {
        if ((knownDevice[0] != '\0') &&
            (strcmp(knownDevice, kUartDeviceList[i]) == 0)) {
            continue;
        }
        LOGD("[Window5Rs485] scan send %d/%d -> %s\r\n", i + 1, total, kUartDeviceList[i]);
        SWindow5Rs485Result routeResult = makeWindow5Rs485Result();
        if (sendWindow5Rs485Device(kUartDeviceList[i], frame, frameLen, pFrameName, true, &routeResult)) {
            LOGD("[Window5Rs485] scan matched route=%s frame=%s\r\n", kUartDeviceList[i], pFrameName);
            return routeResult;
        }
        if (routeResult.sendOk) {
            result = routeResult;
        }
        usleep(20000);
    }

    LOGD("[Window5Rs485] scan send finish, frame=%s, matched=0, candidate=%d\r\n",
         pFrameName, total);
    return result;
}

static SWindow5Rs485Result sendWindow5Rs485CommandDetailedSync(BYTE cmd,
                                                               const BYTE *pData,
                                                               BYTE dataLen,
                                                               const char *pFrameName) {
    pthread_mutex_lock(&sWindow5IoMutex);
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedUnlocked(
        cmd, pData, dataLen, pFrameName);
    pthread_mutex_unlock(&sWindow5IoMutex);
    return result;
}


static const char* getWindow5SensorUnitText(BYTE unit) {
    switch (unit) {
    case 0U: return "MPa";
    case 1U: return "KPa";
    case 2U: return "Pa";
    case 3U: return "Bar";
    case 4U: return "mBar";
    case 5U: return "kg/cm2";
    case 6U: return "psi";
    case 7U: return "mH2O";
    case 8U: return "mmH2O";
    default: return "";
    }
}

static void formatWindow5SensorStatus(const SWindow5Rs485Result &result,
                                      char *pOut,
                                      size_t outSize) {
    if ((pOut == NULL) || (outSize == 0U)) {
        return;
    }
    pOut[0] = '\0';
    if (!result.hasSensorValue || result.sensorDecimals > 3U) {
        return;
    }

    const bool negative = result.sensorRawValue < 0;
    int absValue = negative ? -result.sensorRawValue : result.sensorRawValue;
    int scale = 1;
    for (BYTE i = 0; i < result.sensorDecimals; ++i) {
        scale *= 10;
    }

    const int integerPart = absValue / scale;
    const int fractionPart = absValue % scale;
    const char *unitText = getWindow5SensorUnitText(result.sensorUnit);
    if (result.sensorDecimals == 0U) {
        snprintf(pOut, outSize, "%s%d%s", negative ? "-" : "", integerPart, unitText);
    } else if (result.sensorDecimals == 1U) {
        snprintf(pOut, outSize, "%s%d.%01d%s", negative ? "-" : "", integerPart, fractionPart, unitText);
    } else if (result.sensorDecimals == 2U) {
        snprintf(pOut, outSize, "%s%d.%02d%s", negative ? "-" : "", integerPart, fractionPart, unitText);
    } else {
        snprintf(pOut, outSize, "%s%d.%03d%s", negative ? "-" : "", integerPart, fractionPart, unitText);
    }
}

static void pushWindow5DeviceStateUpdate(int targetAddress,
                                         const SWindow5Rs485Result &result,
                                         bool valveCommand) {
    SWindow5DeviceStateUpdate update;
    update.targetAddress = targetAddress;
    update.result = result;
    update.valveCommand = valveCommand;

    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    if (sWindow5StateUpdateQueue.size() >= WINDOW5_QUEUE_MAX) {
        sWindow5StateUpdateQueue.pop_front();
    }
    sWindow5StateUpdateQueue.push_back(update);
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
}

static void addWindow5DiscoveredDevice(std::vector<SWindow5DiscoveredDevice> &devices,
                                       int address,
                                       BYTE decoderType,
                                       BYTE deviceState) {
    for (size_t i = 0; i < devices.size(); ++i) {
        if (devices[i].address != address) {
            continue;
        }
        if (devices[i].decoderType != decoderType) {
            devices[i].addressConflict = true;
            LOGD("[Window5Rs485] discovery address conflict address=%u type=%u/%u\r\n",
                 address, devices[i].decoderType, decoderType);
            return;
        }
        devices[i].deviceState = deviceState;
        return;
    }

    SWindow5DiscoveredDevice device;
    device.address = address;
    device.decoderType = decoderType;
    device.deviceState = deviceState;
    device.addressConflict = false;
    devices.push_back(device);
}

// Per-address replies are serialized; track the two-pass recovery counts.
static unsigned int sWindow5DiscoveryDirectProbes = 0U;
static unsigned int sWindow5DiscoveryDirectAccepted = 0U;
static long long sWindow5DiscoveryStartedMs = 0LL;
static unsigned int sWindow5DiscoveryExpectedProbes = 0U;

long long getWindow5DiscoveryElapsedMs() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const bool running = sWindow5DiscoveryRunning;
    const long long started = sWindow5DiscoveryStartedMs;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    if (!running || started <= 0) return 0LL;
    const long long now = getWindow5MonotonicMs();
    return now > started ? now - started : 0LL;
}

unsigned int getWindow5DiscoveryProbeCount() {
    return sWindow5DiscoveryDirectProbes;
}

unsigned int getWindow5DiscoveryExpectedProbeCount() {
    return sWindow5DiscoveryExpectedProbes;
}

static void finishWindow5DeviceDiscovery(const std::vector<SWindow5DiscoveredDevice> &devices,
                                         bool scanCompleted) {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    sWindow5DiscoveryResults = devices;
    sWindow5DiscoveryRunning = false;
    sWindow5DiscoveryCompleted = true;
    sWindow5DiscoveryScanCompleted = scanCompleted;
    sWindow5DiscoveryStartedMs = 0LL;
    // Any normal state reply that completed while the discovery transaction
    // was taking the bus belongs to the old device table.  Do not apply it
    // after the new table has been committed.  Keep valve-command replies so
    // an unrelated manual command can still finish its UI wait state.
    for (std::deque<SWindow5DeviceStateUpdate>::iterator it = sWindow5StateUpdateQueue.begin();
         it != sWindow5StateUpdateQueue.end();) {
        if (!it->valveCommand) {
            it = sWindow5StateUpdateQueue.erase(it);
        } else {
            ++it;
        }
    }
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
}

// A single-address 0x44 state request avoids simultaneous replies entirely.
// The full sweep below retries only addresses that did not return a valid ACK.
static int probeWindow5DeviceStateOnFd(int fd, int address, int waitMs,
                                        std::vector<SWindow5DiscoveredDevice> &devices) {
    BYTE requestData[2] = {0};
    putWindow5Address(requestData, address);

    BYTE frame[7] = {
        0x55U,
        0xAAU,
        WINDOW5_CMD_GET_DEVICE_STATE,
        sizeof(requestData),
        requestData[0],
        requestData[1],
        0U,
    };
    frame[6] = WINDOW5_CMD_GET_DEVICE_STATE + sizeof(requestData) +
               requestData[0] + requestData[1];

    if (sWindow5UrgentNoReplyPending) return -1;
    if (tcflush(fd, TCIFLUSH) != 0) return -1;
    if (!writeWindow5Rs485Frame(fd, frame, sizeof(frame))) {
        return -1;
    }

    BYTE replyStatus = 0xFFU;
    int returnedAddress = -1;
    BYTE returnedDecoderType = 0xFFU;
    bool hasReturnedDecoderType = false;
    BYTE returnedDeviceState = 0xFFU;
    bool hasReturnedDeviceState = false;
    bool hasSensorValue = false;
    int sensorRawValue = 0;
    BYTE sensorDecimals = 0xFFU;
    BYTE sensorUnit = 0xFFU;
    bool ioError = false;
    const int replyType = waitWindow5Rs485Reply(
        fd, WINDOW5_CMD_GET_DEVICE_STATE, waitMs, address,
        &replyStatus, &returnedAddress, &returnedDecoderType,
        &hasReturnedDecoderType, &returnedDeviceState, &hasReturnedDeviceState,
        &hasSensorValue, &sensorRawValue, &sensorDecimals, &sensorUnit, &ioError);
    if (ioError || sWindow5UrgentNoReplyPending) return -1;
    if ((replyType != 1) || (replyStatus != 0U) ||
        (returnedAddress != address) || !hasReturnedDecoderType ||
        !hasReturnedDeviceState) {
        LOGD("[Window5Rs485] direct probe rejected address=%d replyType=%d "
             "status=%u returnedAddress=%d hasType=%d type=%u "
             "hasState=%d state=%u\r\n",
             address, replyType, replyStatus, returnedAddress,
             hasReturnedDecoderType ? 1 : 0, returnedDecoderType,
             hasReturnedDeviceState ? 1 : 0, returnedDeviceState);
        return false;
    }
    const bool validType = (returnedDecoderType == WINDOW5_DECODER_TYPE_VALUE) ||
                           (returnedDecoderType == WINDOW5_DECODER_TYPE_SENSER);
    const bool validState = (returnedDeviceState <= 1U) ||
                            (returnedDeviceState == 0xFFU);
    if (!validType || !validState) {
        LOGD("[Window5Rs485] direct probe invalid data address=%d type=%u state=%u\r\n",
             address, returnedDecoderType, returnedDeviceState);
        return false;
    }

    addWindow5DiscoveredDevice(devices, address, returnedDecoderType,
                               returnedDeviceState);
    return true;
}

// Return false only for an aborted/failed transaction, not a missing decoder.
static bool sweepWindow5UnansweredAddresses(int fd,
        std::vector<SWindow5DiscoveredDevice> &devices, std::string &phaseReport) {
    std::vector<bool> discovered(WINDOW5_CONFIG_ADDRESS_MAX + 1U, false);
    for (size_t i = 0; i < devices.size(); ++i) {
        if (devices[i].address >= WINDOW5_CONFIG_ADDRESS_MIN &&
            devices[i].address <= WINDOW5_CONFIG_ADDRESS_MAX) {
            discovered[devices[i].address] = true;
        }
    }
    for (int pass = 0; pass <= WINDOW5_DISCOVERY_RETRY_PASSES; ++pass) {
        const long long startedMs = getWindow5MonotonicMs();
        if (startedMs < 0) return false;
        const unsigned int before = static_cast<UINT>(devices.size());
        unsigned int attempted = 0U;
        for (int address = WINDOW5_CONFIG_ADDRESS_MIN;
             address <= WINDOW5_CONFIG_ADDRESS_MAX; ++address) {
            if (sWindow5UrgentNoReplyPending) return false;
            if (discovered[address]) continue;
            ++attempted;
            ++sWindow5DiscoveryDirectProbes;
            const int result = probeWindow5DeviceStateOnFd(fd, address,
                pass == 0 ? WINDOW5_DISCOVERY_PROBE_WAIT_MS : WINDOW5_DISCOVERY_RETRY_WAIT_MS,
                devices);
            if (result < 0) {
                LOGD("[Window5Rs485] discovery aborted pass=%d address=%d\r\n", pass, address);
                return false;
            }
            if (result > 0) {
                discovered[address] = true;
                ++sWindow5DiscoveryDirectAccepted;
                LOGD("[Window5Rs485] discovery recovered pass=%d address=%d\r\n", pass, address);
            }
        }
        char line[192];
        snprintf(line, sizeof(line), "pass=%d probes=%u recovered=%u elapsedMs=%lld\r\n",
                 pass, attempted, static_cast<UINT>(devices.size()) - before,
                 getWindow5MonotonicMs() - startedMs);
        phaseReport += line;
        if (attempted == 0U) break;
    }
    // Unanswered addresses include unused addresses; these are not a count of
    // physically present missing devices (the master cannot know that count).
    phaseReport += "unansweredAddresses=";
    bool first = true;
    for (int address = WINDOW5_CONFIG_ADDRESS_MIN;
         address <= WINDOW5_CONFIG_ADDRESS_MAX; ++address) {
        if (discovered[address]) continue;
        char text[16];
        snprintf(text, sizeof(text), "%s%d", first ? "" : ",", address);
        phaseReport += text;
        first = false;
    }
    phaseReport += "\r\n";
    return !sWindow5UrgentNoReplyPending;
}

static void runWindow5DeviceDiscovery() {
    const long long scanStartedMs = getWindow5MonotonicMs();
    struct timeval wallClockStart;
    memset(&wallClockStart, 0, sizeof(wallClockStart));
    (void)gettimeofday(&wallClockStart, NULL);
    const long long scanId =
        (static_cast<long long>(wallClockStart.tv_sec) * 1000000LL) +
        static_cast<long long>(wallClockStart.tv_usec);
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    sWindow5LastDiscoveryScanId = scanId;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    std::string phaseReport;
    std::vector<SWindow5DiscoveredDevice> devices;
    const char* pDeviceName = "/dev/ttyS2";
    const int fd = open(pDeviceName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        finishWindow5DeviceDiscovery(devices, false);
        return;
    }
    if (!configureWindow5Rs485Port(fd)) {
        close(fd);
        finishWindow5DeviceDiscovery(devices, false);
        return;
    }

    sWindow5DiscoveryDirectProbes = 0U;
    sWindow5DiscoveryDirectAccepted = 0U;
    sWindow5DiscoveryExpectedProbes = cj96_discovery::ADDRESS_COUNT *
                                      (cj96_discovery::RETRY_PASSES + 1);
    sWindow5DiscoveryStartedMs = scanStartedMs;

    // Any stale abort flag from a previous transaction must not kill this scan.
    sWindow5UrgentNoReplyPending = false;
    bool scanCompleted = (scanStartedMs >= 0);
    phaseReport += "scanMode=sequential_unicast\r\n";
    // Query all configured addresses independently. A broken node cannot
    // collide with another node's reply or consume another address's slot.
    if (scanCompleted) {
        scanCompleted = sweepWindow5UnansweredAddresses(fd, devices, phaseReport);
    }
    if (sWindow5UrgentNoReplyPending) scanCompleted = false;

    for (size_t i = 0; i < devices.size(); ++i) {
        for (size_t j = i + 1U; j < devices.size(); ++j) {
            if (devices[j].address < devices[i].address) {
                const SWindow5DiscoveredDevice temp = devices[i];
                devices[i] = devices[j];
                devices[j] = temp;
            }
        }
    }

    LOGD("[Window5Rs485] discovery summary mode=sequential_unicast devices=%u "
         "probes=%u accepted=%u retryPasses=%d\r\n",
         static_cast<UINT>(devices.size()), sWindow5DiscoveryDirectProbes,
         sWindow5DiscoveryDirectAccepted, WINDOW5_DISCOVERY_RETRY_PASSES);

    // The board redirects stdout to /dev/null, so mirror the discovery result
    // into a file that can be pulled over adb while validating the scan.
    {
        std::string report = phaseReport;
        char line[192] = {0};
        snprintf(line, sizeof(line), "scanId=%lld\r\n", scanId);
        report += line;
        snprintf(line, sizeof(line), "totalElapsedMs=%lld\r\n", getWindow5MonotonicMs() - scanStartedMs);
        report += line;
        snprintf(line, sizeof(line), "mode=sequential_unicast\r\n");
        report += line;
        snprintf(line, sizeof(line), "devices=%u\r\n", static_cast<UINT>(devices.size()));
        report += line;
        snprintf(line, sizeof(line), "directProbes=%u\r\n", sWindow5DiscoveryDirectProbes);
        report += line;
        snprintf(line, sizeof(line), "directAccepted=%u\r\n", sWindow5DiscoveryDirectAccepted);
        report += line;
        snprintf(line, sizeof(line), "sweepPasses=%d\r\n", WINDOW5_DISCOVERY_RETRY_PASSES + 1);
        report += line;
        snprintf(line, sizeof(line), "retryPasses=%d\r\n", WINDOW5_DISCOVERY_RETRY_PASSES);
        report += line;
        snprintf(line, sizeof(line), "scanCompleted=%d\r\n", scanCompleted ? 1 : 0);
        report += line;
        report += "addresses=";
        for (size_t i = 0; i < devices.size(); ++i) {
            snprintf(line, sizeof(line), "%s%u", (i == 0U) ? "" : ",",
                     static_cast<unsigned>(devices[i].address));
            report += line;
        }
        report += "\r\n";
        char archiveName[64] = {0};
        snprintf(archiveName, sizeof(archiveName), "discovery_scan_%lld.log", scanId);
        (void)cj96_persist::writeTextAtomic(
            cj96_persist::logPath(archiveName), report);
        (void)cj96_persist::writeTextAtomic(
            cj96_persist::logPath("discovery_scan.log"), report);
    }

    close(fd);
    if (!devices.empty()) {
        setWindow5KnownDevice(pDeviceName);
    }

    finishWindow5DeviceDiscovery(devices, scanCompleted);
}

static void* window5Rs485Worker(void *arg) {
    (void)arg;
    long long lastValveCommandCompletedMs = 0LL;

    while (true) {
        SWindow5Rs485Request req;

        pthread_mutex_lock(&sWindow5QueueMutex);
        while (sWindow5RequestQueue.empty()) {
            pthread_cond_wait(&sWindow5QueueCond, &sWindow5QueueMutex);
        }
        req = sWindow5RequestQueue.front();
        sWindow5RequestQueue.pop_front();
        pthread_mutex_unlock(&sWindow5QueueMutex);

        if (req.valveCommand && lastValveCommandCompletedMs > 0LL) {
            const long long nowMs = getWindow5MonotonicMs();
            const long long elapsedMs = nowMs - lastValveCommandCompletedMs;
            const long long remainingMs = WINDOW5_VALVE_COMMAND_GAP_MS - elapsedMs;
            if (remainingMs > 0LL) {
                LOGD("[Window5Rs485] valve quiet gap %lld ms before next valve "
                     "address=%d group=%d\r\n",
                     remainingMs, req.targetAddress,
                     req.groupValveCommand ? req.groupNo : 0);
                struct timespec delay;
                delay.tv_sec = static_cast<time_t>(remainingMs / 1000LL);
                delay.tv_nsec = static_cast<long>((remainingMs % 1000LL) * 1000000LL);
                while (nanosleep(&delay, &delay) != 0 && errno == EINTR) {
                }
            }
        }

        LOGD("[Window5Rs485] worker start frame=%s cmd=0x%02X\r\n", req.frameName, req.cmd);
        const bool interactiveCommand = req.urgentCommand ||
                                        (req.cmd == WINDOW5_CMD_SET_VALVE_STATE) ||
                                        req.discoverDevices;
        if (interactiveCommand) {
            sWindow5UrgentNoReplyPending = false;
        }
        if (req.discoverDevices) {
            pthread_mutex_lock(&sWindow5IoMutex);
            runWindow5DeviceDiscovery();
            pthread_mutex_unlock(&sWindow5IoMutex);
            continue;
        }
        const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
            req.cmd, req.data, req.dataLen, req.frameName);
        if (req.valveCommand) {
            lastValveCommandCompletedMs = getWindow5MonotonicMs();
        }
        if (req.trackDeviceState) {
            pushWindow5DeviceStateUpdate(req.targetAddress, result, req.valveCommand);
        }
    }

    return NULL;
}

static bool ensureWindow5Rs485Worker() {
    pthread_mutex_lock(&sWindow5QueueMutex);
    if (!sWindow5WorkerStarted) {
        const int ret = pthread_create(&sWindow5WorkerThread, NULL, window5Rs485Worker, NULL);
        if (ret != 0) {
            pthread_mutex_unlock(&sWindow5QueueMutex);
            LOGD("[Window5Rs485] create worker failed ret=%d\r\n", ret);
            return false;
        }
        pthread_detach(sWindow5WorkerThread);
        sWindow5WorkerStarted = true;
    }
    pthread_mutex_unlock(&sWindow5QueueMutex);
    return true;
}

static bool enqueueWindow5Rs485CommandInternal(BYTE cmd,
                                               const BYTE *pData,
                                               BYTE dataLen,
                                               const char *pFrameName,
                                               bool trackDeviceState,
                                               int targetAddress,
                                               bool discoverDevices,
                                               bool urgentCommand,
                                               bool groupValveCommand,
                                               int groupNo) {
    // Discovery owns the RS485 bus.  A state query accepted here can finish
    // after discovery starts and make the result table look intermittent.
    if ((cmd == WINDOW5_CMD_GET_DEVICE_STATE) &&
        (isWindow5DeviceDiscoveryActive() || isWindow5ManualConfigActive())) {
        LOGD("[Window5Rs485] state query blocked during exclusive transaction "
             "frame=%s address=%d\r\n", pFrameName, targetAddress);
        return false;
    }
    if (!ensureWindow5Rs485Worker()) {
        return false;
    }
    if (dataLen > WINDOW5_PROTOCOL_MAX_DATA_LEN) {
        LOGD("[Window5Rs485] queue data too long, frame=%s len=%u\r\n", pFrameName, dataLen);
        return false;
    }
    if ((dataLen > 0U) && (pData == NULL)) {
        LOGD("[Window5Rs485] queue data null, frame=%s len=%u\r\n", pFrameName, dataLen);
        return false;
    }

    pthread_mutex_lock(&sWindow5QueueMutex);
    if ((cmd == WINDOW5_CMD_GET_DEVICE_STATE) && !urgentCommand) {
        for (std::deque<SWindow5Rs485Request>::const_iterator it = sWindow5RequestQueue.begin();
             it != sWindow5RequestQueue.end(); ++it) {
            if ((it->cmd == cmd) && (it->targetAddress == targetAddress)) {
                pthread_mutex_unlock(&sWindow5QueueMutex);
                return true;
            }
        }
    }
    if (sWindow5RequestQueue.size() >= WINDOW5_QUEUE_MAX) {
        pthread_mutex_unlock(&sWindow5QueueMutex);
        LOGD("[Window5Rs485] queue full, drop frame=%s cmd=0x%02X\r\n", pFrameName, cmd);
        return false;
    }

    SWindow5Rs485Request req;
    req.cmd = cmd;
    req.dataLen = dataLen;
    memset(req.data, 0, sizeof(req.data));
    if (dataLen > 0U) {
        memcpy(req.data, pData, dataLen);
    }
    memset(req.frameName, 0, sizeof(req.frameName));
    strncpy(req.frameName, pFrameName, sizeof(req.frameName) - 1U);
    req.trackDeviceState = trackDeviceState;
    req.targetAddress = targetAddress;
    req.discoverDevices = discoverDevices;
    req.valveCommand = (cmd == WINDOW5_CMD_SET_VALVE_STATE);
    req.urgentCommand = urgentCommand;
    req.groupValveCommand = req.valveCommand && groupValveCommand;
    req.groupNo = req.groupValveCommand ? groupNo : 0;
    // While discovery owns the bus, unrelated interactive commands (valve
    // writes, pressure requests) must wait instead of aborting the scan.  They
    // are queued as ordinary requests and executed after the scan commits.
    const bool discoveryOwnsBus = isWindow5DeviceDiscoveryActive();
    const bool interactiveCommand = discoverDevices ||
                                    (!discoveryOwnsBus &&
                                     (req.urgentCommand ||
                                      (cmd == WINDOW5_CMD_SET_VALVE_STATE)));
    if (interactiveCommand) {
        for (std::deque<SWindow5Rs485Request>::iterator it = sWindow5RequestQueue.begin();
             it != sWindow5RequestQueue.end();) {
            if (it->cmd == WINDOW5_CMD_GET_DEVICE_STATE) {
                it = sWindow5RequestQueue.erase(it);
            } else {
                ++it;
            }
        }
        const bool valveAlreadyWaiting = req.valveCommand && sWindow5ValveCommandBusy;
        if (!valveAlreadyWaiting) {
            sWindow5UrgentNoReplyPending = true;
        }
        if (valveAlreadyWaiting) {
            sWindow5RequestQueue.push_back(req);
        } else {
            sWindow5RequestQueue.push_front(req);
        }
    } else {
        sWindow5RequestQueue.push_back(req);
    }
    pthread_cond_signal(&sWindow5QueueCond);
    pthread_mutex_unlock(&sWindow5QueueMutex);

    LOGD("[Window5Rs485] queue push frame=%s cmd=0x%02X\r\n", pFrameName, cmd);
    if (req.valveCommand) {
        addWindow5ValveCommandPending(
            (req.dataLen >= 4U) ? req.data[3] : 0xFFU,
            req.groupValveCommand, req.groupNo, req.targetAddress);
    }
    return true;
}

static bool enqueueWindow5Rs485Command(BYTE cmd,
                                       const BYTE *pData,
                                       BYTE dataLen,
                                       const char *pFrameName) {
    return enqueueWindow5Rs485CommandInternal(cmd, pData, dataLen, pFrameName,
                                              false, 0, false, false, false, 0);
}

static bool enqueueWindow5TrackedRs485Command(BYTE cmd,
                                              const BYTE *pData,
                                              BYTE dataLen,
                                              const char *pFrameName,
                                              int targetAddress) {
    return enqueueWindow5Rs485CommandInternal(cmd, pData, dataLen, pFrameName,
                                              true, targetAddress, false, false,
                                              false, 0);
}

static bool enqueueWindow5GroupValveRs485Command(const BYTE *pData,
                                                  BYTE dataLen,
                                                  const char *pFrameName,
                                                  int targetAddress,
                                                  int groupNo) {
    return enqueueWindow5Rs485CommandInternal(
        WINDOW5_CMD_SET_VALVE_STATE, pData, dataLen, pFrameName,
        true, targetAddress, false, false, true, groupNo);
}

static bool enqueueWindow5UrgentTrackedRs485Command(BYTE cmd,
                                                    const BYTE *pData,
                                                    BYTE dataLen,
                                                    const char *pFrameName,
                                                    int targetAddress) {
    return enqueueWindow5Rs485CommandInternal(cmd, pData, dataLen, pFrameName,
                                              true, targetAddress, false, true,
                                              false, 0);
}

bool isWindow5DeviceDiscoveryRunning() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const bool running = sWindow5DiscoveryRunning;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return running;
}

// Last completed scan: how many decoders answered on the RS485 bus.  Window2
// reports this in the list footer so the operator can see the real bus size
// without counting rows.
bool hasWindow5LastDiscoveryDeviceCount() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const bool valid = sWindow5LastDiscoveryBusDeviceCountValid;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return valid;
}

unsigned int getWindow5LastDiscoveryDeviceCount() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const unsigned int count = sWindow5LastDiscoveryBusDeviceCount;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return count;
}

std::string getWindow5LastDiscoveryUnaddedAddressesText() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const std::vector<int> addresses = sWindow5LastDiscoveryUnaddedAddresses;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);

    std::string text;
    char addressText[16] = {0};
    for (size_t i = 0; i < addresses.size(); ++i) {
        if (i > 0U) {
            text += ((i % 8U) == 0U) ? "\r\n" : ", ";
        }
        snprintf(addressText, sizeof(addressText), "%d", addresses[i]);
        text += addressText;
    }
    return text;
}

static bool isWindow5DeviceDiscoveryActive() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const bool active = sWindow5DiscoveryRunning || sWindow5DiscoveryCompleted;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return active;
}

static bool isWindow5ManualConfigActive() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const bool active = sWindow5ManualConfigActive;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return active;
}

static bool beginWindow5ManualConfigTransaction() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    if (sWindow5DiscoveryRunning || sWindow5DiscoveryCompleted ||
        sWindow5ManualConfigActive) {
        pthread_mutex_unlock(&sWindow5StateUpdateMutex);
        return false;
    }
    sWindow5ManualConfigActive = true;
    for (std::deque<SWindow5DeviceStateUpdate>::iterator it = sWindow5StateUpdateQueue.begin();
         it != sWindow5StateUpdateQueue.end();) {
        if (!it->valveCommand) {
            it = sWindow5StateUpdateQueue.erase(it);
        } else {
            ++it;
        }
    }
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);

    pthread_mutex_lock(&sWindow5QueueMutex);
    for (std::deque<SWindow5Rs485Request>::iterator it = sWindow5RequestQueue.begin();
         it != sWindow5RequestQueue.end();) {
        if (it->cmd == WINDOW5_CMD_GET_DEVICE_STATE) {
            it = sWindow5RequestQueue.erase(it);
        } else {
            ++it;
        }
    }
    pthread_mutex_unlock(&sWindow5QueueMutex);
    return true;
}

static void endWindow5ManualConfigTransaction() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    sWindow5ManualConfigActive = false;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
}

bool requestWindow5DeviceDiscovery() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    if (sWindow5DiscoveryRunning || sWindow5DiscoveryCompleted ||
        sWindow5ManualConfigActive) {
        pthread_mutex_unlock(&sWindow5StateUpdateMutex);
        return false;
    }
    sWindow5DiscoveryRunning = true;
    sWindow5DiscoveryCompleted = false;
    sWindow5DiscoveryScanCompleted = false;
    sWindow5DiscoveryResults.clear();
    sWindow5LastDiscoveryUnaddedAddresses.clear();
    // Drop results from polling cycles that were already in flight.  They are
    // not part of this discovery transaction and must not be applied to the
    // freshly discovered device table.
    for (std::deque<SWindow5DeviceStateUpdate>::iterator it = sWindow5StateUpdateQueue.begin();
         it != sWindow5StateUpdateQueue.end();) {
        if (!it->valveCommand) {
            it = sWindow5StateUpdateQueue.erase(it);
        } else {
            ++it;
        }
    }
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);

    pthread_mutex_lock(&sWindow5QueueMutex);
    // Also remove normal state requests that have not reached the worker yet.
    for (std::deque<SWindow5Rs485Request>::iterator it = sWindow5RequestQueue.begin();
         it != sWindow5RequestQueue.end();) {
        if (it->cmd == WINDOW5_CMD_GET_DEVICE_STATE) {
            it = sWindow5RequestQueue.erase(it);
        } else {
            ++it;
        }
    }
    pthread_mutex_unlock(&sWindow5QueueMutex);

    const bool queued = enqueueWindow5Rs485CommandInternal(
        WINDOW5_CMD_DISCOVER_DEVICES, NULL, 0U, "DISCOVER",
        false, 0, true, false, false, 0);
    if (!queued) {
        pthread_mutex_lock(&sWindow5StateUpdateMutex);
        sWindow5DiscoveryRunning = false;
        pthread_mutex_unlock(&sWindow5StateUpdateMutex);
        return false;
    }

    refreshDeviceListViews();
    return true;
}

static bool takeWindow5DiscoveryResults(std::vector<SWindow5DiscoveredDevice> *pDevices,
                                        bool *pScanCompleted) {
    if (pDevices == NULL || pScanCompleted == NULL) {
        return false;
    }

    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    if (!sWindow5DiscoveryCompleted) {
        pthread_mutex_unlock(&sWindow5StateUpdateMutex);
        return false;
    }
    *pDevices = sWindow5DiscoveryResults;
    *pScanCompleted = sWindow5DiscoveryScanCompleted;
    sWindow5DiscoveryResults.clear();
    sWindow5DiscoveryCompleted = false;
    sWindow5DiscoveryScanCompleted = false;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return true;
}

static bool applyWindow5DiscoveryResults() {
    std::vector<SWindow5DiscoveredDevice> devices;
    bool scanCompleted = false;
    if (!takeWindow5DiscoveryResults(&devices, &scanCompleted)) {
        return false;
    }
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const long long scanId = sWindow5LastDiscoveryScanId;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);

    int conflictCount = 0;
    int addedValveCount = 0;
    int addedSensorCount = 0;
    std::vector<int> discoveredAddresses;
    std::vector<int> responsiveAddresses;
    std::vector<int> unaddedAddresses;
    for (size_t i = 0; i < devices.size(); ++i) {
        const SWindow5DiscoveredDevice &device = devices[i];
        bool responsiveAlready = false;
        for (size_t j = 0; j < responsiveAddresses.size(); ++j) {
            if (responsiveAddresses[j] == device.address) {
                responsiveAlready = true;
                break;
            }
        }
        if (!responsiveAlready) {
            responsiveAddresses.push_back(device.address);
        }
        if (device.addressConflict) {
            ++conflictCount;
            scanCompleted = false;
            bool listed = false;
            for (size_t j = 0; j < unaddedAddresses.size(); ++j) {
                if (unaddedAddresses[j] == device.address) {
                    listed = true;
                    break;
                }
            }
            if (!listed) {
                unaddedAddresses.push_back(device.address);
            }
            continue;
        }
        bool discoveredAlready = false;
        for (size_t j = 0; j < discoveredAddresses.size(); ++j) {
            if (discoveredAddresses[j] == device.address) {
                discoveredAlready = true;
                break;
            }
        }
        if (!discoveredAlready) {
            discoveredAddresses.push_back(device.address);
        }

        bool added = false;
        (void)DeviceDataStore::syncDiscoveredDevice(
            device.address,
            device.decoderType,
            device.deviceState <= 1U,
            device.deviceState == 1U,
            &added);
        if (added) {
            if (device.decoderType == WINDOW5_DECODER_TYPE_VALUE) {
                ++addedValveCount;
            } else if (device.decoderType == WINDOW5_DECODER_TYPE_SENSER) {
                ++addedSensorCount;
            }
        }

        bool presentInTable = false;
        for (int index = 0; index < DeviceDataStore::getDeviceCount(); ++index) {
            const SDATA *data = DeviceDataStore::getDevice(index);
            if (data && data->address == device.address) {
                presentInTable = true;
                break;
            }
        }
        if (!presentInTable) {
            bool listed = false;
            for (size_t j = 0; j < unaddedAddresses.size(); ++j) {
                if (unaddedAddresses[j] == device.address) {
                    listed = true;
                    break;
                }
            }
            if (!listed) {
                unaddedAddresses.push_back(device.address);
            }
        }
    }

    // Keep a snapshot of transport completion before marking per-device misses.
    // A completed address sweep removes nonresponding rows when the bus has
    // at least one valid responder; a totally silent bus is treated as a bus
    // fault because absence and a broken return path cannot be distinguished.
    const bool scanTransactionCompleted = scanCompleted;
    std::vector<int> missedKnownAddresses;
    for (int index = 0; index < DeviceDataStore::getDeviceCount(); ++index) {
        const SDATA* data = DeviceDataStore::getDevice(index);
        if (!isWindow5ManagedDecoderDevice(data)) continue;
        const int address = data->address;
        if (address < WINDOW5_CONFIG_ADDRESS_MIN ||
            address > WINDOW5_CONFIG_ADDRESS_MAX) {
            continue;
        }
        bool answered = false;
        for (size_t j = 0; j < responsiveAddresses.size(); ++j) {
            if (responsiveAddresses[j] == address) { answered = true; break; }
        }
        if (!answered) missedKnownAddresses.push_back(address);
    }
    const int missedKnownCount = static_cast<int>(missedKnownAddresses.size());
    const bool busSilent = devices.empty() &&
                           (DeviceDataStore::getDeviceCount() > 0);
    if (missedKnownCount > 0 || busSilent) {
        scanCompleted = false;
    }

    int removedCustomCount = 0;
    if (scanTransactionCompleted && !busSilent && (conflictCount == 0)) {
        removedCustomCount =
            DeviceDataStore::removeCustomDevicesNotInDiscovery(responsiveAddresses);
    }
    // Keep the visible custom-device table in address order after every sync.
    DeviceDataStore::sortCustomDevicesByAddress();

    window5ClearAllAddressFaults();
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    sWindow5LastDiscoveryBusDeviceCount =
            static_cast<unsigned int>(discoveredAddresses.size());
    sWindow5LastDiscoveryBusDeviceCountValid = true;
    sWindow5LastDiscoveryUnaddedAddresses = unaddedAddresses;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    LOGD("[Window5Rs485] discovery complete devices=%u conflicts=%d "
         "unadded=%u missedKnown=%d removed=%d busSilent=%d\\r\\n",
         static_cast<UINT>(devices.size()), conflictCount,
         static_cast<UINT>(unaddedAddresses.size()), missedKnownCount,
         removedCustomCount, busSilent ? 1 : 0);
    LOGD("[Window5Rs485] discovery scanComplete=%d\\r\\n",
         scanCompleted ? 1 : 0);
    {
        char report[512] = {0};
        snprintf(report, sizeof(report),
                 "scanId=%lld\\r\\nsyncComplete=%d\\r\\nrespondingDevices=%u\\r\\n"
                 "missedKnown=%d\\r\\nconflicts=%d\\r\\nremoved=%d\\r\\n"
                 "busSilent=%d\\r\\nvisibleDeviceCount=%d\\r\\n",
                 scanId, scanCompleted ? 1 : 0,
                 static_cast<UINT>(responsiveAddresses.size()), missedKnownCount,
                 conflictCount, removedCustomCount, busSilent ? 1 : 0,
                 DeviceDataStore::getDeviceCount());
        char archiveName[64] = {0};
        snprintf(archiveName, sizeof(archiveName), "discovery_sync_%lld.log", scanId);
        (void)cj96_persist::writeTextAtomic(
            cj96_persist::logPath(archiveName), report);
        (void)cj96_persist::writeTextAtomic(
            cj96_persist::logPath("discovery_sync.log"), report);
    }
    char tipText[256] = {0};
    char fmtBuf[192] = {0};
    if (!scanCompleted) {
        char missedText[128] = {0};
        size_t used = 0U;
        for (size_t i = 0; i < missedKnownAddresses.size() && used + 8U < sizeof(missedText); ++i) {
            const int written = snprintf(missedText + used, sizeof(missedText) - used,
                                         (i == 0U) ? "%d" : ",%d", missedKnownAddresses[i]);
            if (written <= 0) break;
            used += static_cast<size_t>(written);
        }
        if (busSilent) {
            snprintf(tipText, sizeof(tipText), "%s",
                     Cj96I18n::translateRuntimeText("同步失败：总线异常，无设备应答\r\n已保留原设备表", Cj96I18n::getLanguage()));
        } else if (scanTransactionCompleted && missedKnownCount > 0) {
            snprintf(fmtBuf, sizeof(fmtBuf), "%s",
                     Cj96I18n::translateRuntimeText(
                         "同步未完整：确认应答%d台\\r\\n未应答设备已移除%d台",
                         Cj96I18n::getLanguage()));
            snprintf(tipText, sizeof(tipText), fmtBuf,
                     static_cast<int>(responsiveAddresses.size()), removedCustomCount);
        } else if (missedKnownCount > 0) {
            snprintf(fmtBuf, sizeof(fmtBuf), "%s",
                     Cj96I18n::translateRuntimeText("同步失败：%d 个设备未应答\r\n未应答地址：%s\r\n已保留原设备表", Cj96I18n::getLanguage()));
            snprintf(tipText, sizeof(tipText), fmtBuf, missedKnownCount, missedText);
        } else {
            snprintf(tipText, sizeof(tipText), "%s",
                     Cj96I18n::translateRuntimeText("同步失败：未完成扫描\r\n已保留原设备表", Cj96I18n::getLanguage()));
        }
        LOGD("[Window5Rs485] discovery failed missedKnown=%d busSilent=%d\\r\\n",
             missedKnownCount, busSilent ? 1 : 0);
    } else {
        snprintf(fmtBuf, sizeof(fmtBuf), "%s",
                 Cj96I18n::translateRuntimeText("此次共添加\r\n电磁阀 数量%d\r\n传感器 数量%d", Cj96I18n::getLanguage()));
        snprintf(tipText, sizeof(tipText), fmtBuf, addedValveCount, addedSensorCount);
    }
    (void)showW2TipText(tipText);
    completePage2DeviceDiscoveryForTuya(addedValveCount, addedSensorCount);
    return true;
}

static bool sendWindow5ManualValveStateCommand(bool open) {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    BYTE decoderType = WINDOW5_DECODER_TYPE_VALUE;
    if (!getWindow5SelectedDecoderType(&decoderType)) {
        return false;
    }
    if (decoderType != WINDOW5_DECODER_TYPE_VALUE) {
        setWindow5TestAddressFailureTip("解码器类型错误");
        return false;
    }

    int address = 0;
    if (!parseWindow5ValveAddressEditText(&address)) {
        return false;
    }
    if (!checkWindow5ValveAddressReady(address, decoderType)) {
        return false;
    }

    BYTE requestData[4] = {0};
    putWindow5Address(requestData, address);
    requestData[2] = decoderType;
    requestData[3] = static_cast<BYTE>(open ? 1U : 0U);
    const bool queued = enqueueWindow5TrackedRs485Command(WINDOW5_CMD_SET_VALVE_STATE,
                                              requestData, sizeof(requestData),
                                              open ? "VALVE_ON" : "VALVE_OFF",
                                              address);
    // This complete decoder page is the technical debug/configuration page.
    // Its direct open/close actions must never enter the irrigation log.
    return queued;
}

static bool sendWindow5ValveOnCommand() {
    return sendWindow5ManualValveStateCommand(true);
}

static bool sendWindow5ValveOffCommand() {
    return sendWindow5ManualValveStateCommand(false);
}

static bool isWindow5ManagedDecoderDevice(const SDATA *data) {
    return data && ((strcmp(data->type, "电磁阀") == 0) ||
                    (strcmp(data->type, "传感器") == 0));
}

// A node that stops answering must not be allowed to slow the shared bus
// down.  After several consecutive unanswered polls the address enters a
// long backoff, so one broken decoder cannot starve the healthy ones.
static void window5MarkAddressFault(int address, bool failed) {
    if (address < 0 || address > 255) return;
    if (!failed) {
        sWindow5AddressFailCount[address] = 0;
        sWindow5AddressBackoffUntilMs[address] = 0LL;
        return;
    }
    if (sWindow5AddressFailCount[address] < WINDOW5_FAULT_FAIL_LIMIT) {
        ++sWindow5AddressFailCount[address];
    }
    if (sWindow5AddressFailCount[address] >= WINDOW5_FAULT_FAIL_LIMIT) {
        sWindow5AddressBackoffUntilMs[address] =
                getWindow5NowMs() + WINDOW5_FAULT_BACKOFF_MS;
        LOGD("[Window5Rs485] address %d silent, backoff %lld ms\r\n",
             address, WINDOW5_FAULT_BACKOFF_MS);
        (void)DeviceDataStore::updateRuntimeStateByAddress(
                address, false, DEVICE_DECODER_TYPE_UNKNOWN, false, false);
    }
}

static bool window5IsAddressInBackoff(int address) {
    if (address < 0 || address > 255) return false;
    const long long untilMs = sWindow5AddressBackoffUntilMs[address];
    if (untilMs <= 0LL) return false;
    if (getWindow5NowMs() >= untilMs) {
        sWindow5AddressBackoffUntilMs[address] = 0LL;
        sWindow5AddressFailCount[address] = 0;
        return false;
    }
    return true;
}

static void window5ClearAllAddressFaults() {
    memset(sWindow5AddressFailCount, 0, sizeof(sWindow5AddressFailCount));
    memset(sWindow5AddressBackoffUntilMs, 0, sizeof(sWindow5AddressBackoffUntilMs));
}

static bool requestWindow5DeviceStateByAddress(int address) {
    if (isWindow5DeviceDiscoveryActive()) {
        return false;
    }
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    if ((address < WINDOW5_DEVICE_ADDRESS_MIN) ||
        (address > WINDOW5_DEVICE_ADDRESS_MAX)) {
        return false;
    }

    BYTE requestData[2] = {0};
    putWindow5Address(requestData, address);
    return enqueueWindow5TrackedRs485Command(WINDOW5_CMD_GET_DEVICE_STATE,
                                              requestData, sizeof(requestData),
                                              "GET_STATE", address);
}

static bool requestWindow5PressureStateByAddress(int address) {
    if (isWindow5DeviceDiscoveryActive()) {
        return false;
    }
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    if ((address < WINDOW5_DEVICE_ADDRESS_MIN) ||
        (address > WINDOW5_DEVICE_ADDRESS_MAX)) {
        return false;
    }

    BYTE requestData[2] = {0};
    putWindow5Address(requestData, address);
    return enqueueWindow5UrgentTrackedRs485Command(
        WINDOW5_CMD_GET_DEVICE_STATE, requestData, sizeof(requestData),
        "PRESSURE", address);
}

bool requestWindow5DeviceState(int deviceIndex) {
    const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
    if (!isWindow5ManagedDecoderDevice(data)) {
        return false;
    }
    return requestWindow5DeviceStateByAddress(data->address);
}

static bool requestWindow5ValveStateInternal(int deviceIndex, bool open,
                                            bool groupCommand = false,
                                            int groupNo = 0) {
    const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
    if (!isPumpDevice(data) ||
        (data->address < WINDOW5_DEVICE_ADDRESS_MIN) ||
        (data->address > WINDOW5_DEVICE_ADDRESS_MAX)) {
        return false;
    }

    BYTE requestData[4] = {0};
    putWindow5Address(requestData, data->address);
    requestData[2] = WINDOW5_DECODER_TYPE_VALUE;
    requestData[3] = static_cast<BYTE>(open ? 1U : 0U);
    if (groupCommand) {
        return enqueueWindow5GroupValveRs485Command(
            requestData, sizeof(requestData),
            open ? "GROUP_ON" : "GROUP_OFF", data->address, groupNo);
    }
    return enqueueWindow5TrackedRs485Command(
        WINDOW5_CMD_SET_VALVE_STATE, requestData, sizeof(requestData),
        open ? "VALVE_ON" : "VALVE_OFF", data->address);
}

static bool requestWindow5ValveState(int deviceIndex, bool open) {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    return requestWindow5ValveStateInternal(deviceIndex, open);
}

static bool requestWindow5GroupValveState(int groupNo, bool open) {
    return requestWindow5GroupDevicesState(groupNo, open, true, false);
}

static bool requestWindow5GroupDevicesState(int groupNo, bool open,
                                            bool includeValves,
                                            bool includePumps) {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    bool requested = false;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || !DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            continue;
        }
        // Do not let a board already proven silent monopolise a group command.
        // Keep newly discovered/unknown devices eligible for their first real
        // command; only explicit offline state or communication backoff is
        // filtered here.
        if ((!data->connected && data->stateKnown) ||
            window5IsAddressInBackoff(data->address)) {
            LOGD("[Window5Rs485] skip offline/backoff group=%d address=%d connected=%d stateKnown=%d\r\n",
                 groupNo, data->address, data->connected ? 1 : 0,
                 data->stateKnown ? 1 : 0);
            continue;
        }
        const bool valve = std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0;
        const bool pump = std::strcmp(data->type, "水泵") == 0;
        if ((!includeValves || !valve) && (!includePumps || !pump)) {
            continue;
        }
        requested = requestWindow5ValveStateInternal(
                        i, open, true, groupNo) || requested;
    }
    return requested;
}

static bool requestWindow5GroupPumpState(int groupNo, bool open) {
    return requestWindow5GroupDevicesState(groupNo, open, false, true);
}

static bool requestWindow5GroupIrrigationState(int groupNo, bool open) {
    return requestWindow5GroupDevicesState(groupNo, open, true, true);
}

static bool requestWindow5AllRunningIrrigationOff() {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    bool requested = false;
    bool groupRequested[129] = {false};
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || !data->connected || !data->stateKnown || !data->state) {
            continue;
        }
        const bool valve = std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0;
        const bool pump = std::strcmp(data->type, "水泵") == 0;
        if (!valve && !pump) {
            continue;
        }
        bool hasGroup = false;
        for (int groupNo = 1; groupNo <= 128; ++groupNo) {
            if (!DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
                continue;
            }
            hasGroup = true;
            if (!groupRequested[groupNo]) {
                requested = requestWindow5GroupIrrigationState(groupNo, false) || requested;
                groupRequested[groupNo] = true;
            }
        }
        // Preserve the previous behavior for a running custom device that has
        // no group binding. It is still a single-valve operation in that case.
        if (!hasGroup) {
            requested = requestWindow5ValveStateInternal(i, false) || requested;
        }
    }
    return requested;
}

static bool popWindow5DeviceStateUpdate(SWindow5DeviceStateUpdate *pUpdate) {
    if (pUpdate == NULL) {
        return false;
    }

    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    if (sWindow5StateUpdateQueue.empty()) {
        pthread_mutex_unlock(&sWindow5StateUpdateMutex);
        return false;
    }
    *pUpdate = sWindow5StateUpdateQueue.front();
    sWindow5StateUpdateQueue.pop_front();
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return true;
}

static void applyWindow5DeviceStateUpdates() {
    const bool discoveryApplied = applyWindow5DiscoveryResults();
    bool deviceStateApplied = false;
    SWindow5DeviceStateUpdate update;

    while (popWindow5DeviceStateUpdate(&update)) {
        const SWindow5Rs485Result &result = update.result;
        const bool validType = result.returnedDecoderType == WINDOW5_DECODER_TYPE_VALUE ||
                               result.returnedDecoderType == WINDOW5_DECODER_TYPE_SENSER;
        const bool acceptedValveReply = update.valveCommand &&
                                        (result.replyType == 1) &&
                                        (result.status == 0U) &&
                                        ((result.returnedAddress < 0) ||
                                         (result.returnedAddress == update.targetAddress));
        const bool identified = (result.replyType == 1) &&
                                (result.status == 0U) &&
                                (result.returnedAddress == update.targetAddress) &&
                                result.hasReturnedDecoderType &&
                                result.hasReturnedDeviceState &&
                                validType &&
                                (result.returnedDeviceState <= 1U);
        if (identified && result.hasSensorValue &&
            result.returnedDecoderType == WINDOW5_DECODER_TYPE_SENSER) {
            char sensorStatus[20] = {0};
            formatWindow5SensorStatus(result, sensorStatus, sizeof(sensorStatus));
            deviceStateApplied = DeviceDataStore::updateRuntimeSensorStatusByAddress(
                                     update.targetAddress, true, sensorStatus) ||
                                 deviceStateApplied;
        } else {
            if (acceptedValveReply && !identified) {
                // The command was accepted, but this board returned a legacy
                // short ACK without type/state. Do not turn a healthy board
                // into an offline board merely because its reply is shorter.
                deviceStateApplied = DeviceDataStore::updateRuntimeStateByAddress(
                                         update.targetAddress, true,
                                         DEVICE_DECODER_TYPE_UNKNOWN, false, false) ||
                                     deviceStateApplied;
            } else {
                deviceStateApplied = DeviceDataStore::updateRuntimeStateByAddress(
                                         update.targetAddress,
                                         identified,
                                         identified ? result.returnedDecoderType : DEVICE_DECODER_TYPE_UNKNOWN,
                                         identified,
                                         identified && (result.returnedDeviceState != 0U)) ||
                                     deviceStateApplied;
            }
        }
        // Healthy complete replies and accepted legacy valve ACKs clear the
        // fault counter; a device that really keeps failing is backed off.
        window5MarkAddressFault(update.targetAddress, !(identified || acceptedValveReply));
        if (update.valveCommand) {
            finishWindow5ValveCommandWait(result, update.targetAddress);
        }
    }

    if (discoveryApplied) {
        // Discovery changes the table shape, so rebuild the visible lists.
        // Routine state polling must not rebuild 200-row lists on the UI thread.
        refreshDeviceListViews();
        refreshWindow4ListViews();
        showDeviceListEmptyRow();
    }
    if (deviceStateApplied) {
        // Keep routine status updates on the lightweight dashboard path.
        refreshWindow8IrrigationState();
        refreshRunStatusValueText();
        refreshHomeSensorStatus();
    }
}

static void requestWindow5NextDeviceState() {
    if (isWindow5DeviceDiscoveryRunning() || isWindow5ManualConfigActive()) {
        return;
    }
    if (isWindow5ValveCommandBusy()) {
        return;
    }

    const int total = DeviceDataStore::getDeviceCount();
    if (total <= 0) {
        sWindow5NextDevicePollIndex = 0;
        return;
    }

    if ((sWindow5NextDevicePollIndex < 0) ||
        (sWindow5NextDevicePollIndex >= total)) {
        sWindow5NextDevicePollIndex = 0;
    }

    if (static_cast<int>(sWindow5SensorLastPollMs.size()) < total) {
        sWindow5SensorLastPollMs.resize(static_cast<size_t>(total), 0LL);
    }
    const long long nowMs = getWindow5NowMs();
    for (int checked = 0; checked < total; ++checked) {
        const int index = sWindow5NextDevicePollIndex;
        sWindow5NextDevicePollIndex = (sWindow5NextDevicePollIndex + 1) % total;
        const SDATA* data = DeviceDataStore::getDevice(index);
        if (!isWindow5ManagedDecoderDevice(data)) {
            continue;
        }
        if (window5IsAddressInBackoff(data->address)) {
            continue;
        }
        const bool isSensor = (data->type != NULL) &&
                              (strcmp(data->type, "传感器") == 0);
        if (isSensor) {
            const long long lastMs = sWindow5SensorLastPollMs[index];
            if ((lastMs > 0LL) &&
                ((nowMs - lastMs) < WINDOW5_SENSOR_POLL_INTERVAL_MS)) {
                continue;
            }
            sWindow5SensorLastPollMs[index] = nowMs;
        }
        (void)requestWindow5DeviceState(index);
        return;
    }
}

void updateWindow5DeviceStatePolling() {
    // Do not consume old replies or enqueue new sensor queries while a full
    // discovery transaction owns the bus.  A completed result is deliberately
    // allowed through once so applyWindow5DiscoveryResults() can commit it.
    if (isWindow5DeviceDiscoveryRunning() || isWindow5ManualConfigActive()) {
        return;
    }
    applyWindow5DeviceStateUpdates();
    if (isWindow5DeviceDiscoveryActive() || isWindow5ManualConfigActive()) {
        return;
    }
    requestWindow5NextDeviceState();
}

static bool sWindow5TestAddressTipVisible = false;
static bool sWindow5AddressTextUpdating = false;
static bool sWindow5SourceAddressTextUpdating = false;
static bool sWindow5ValveAddressTextUpdating = false;
static const int WINDOW5_CONFIG_TIP_COLOR_NEUTRAL = static_cast<int>(0xFF1D1D1FU);
static const int WINDOW5_CONFIG_TIP_COLOR_WAIT = static_cast<int>(0xFF005BBBU);
static const int WINDOW5_CONFIG_TIP_COLOR_SUCCESS = static_cast<int>(0xFF248A3DU);
static const int WINDOW5_CONFIG_TIP_COLOR_FAILURE = static_cast<int>(0xFFD92D20U);

static void setWindow5TestAddressTipWithColor(const char *pText, int textColor) {
    const bool visible = (pText != NULL) && (pText[0] != '\0');
    sWindow5ValveSuccessTipHideDeadlineMs = 0;

    LOGD("[Window5Rs485] address tip: %s\r\n", pText ? pText : "");
    if (mTestAdressTipsTextPtr) {
        mTestAdressTipsTextPtr->setTextColor(textColor);
        mTestAdressTipsTextPtr->setText(pText ?
                Cj96I18n::translateRuntimeText(pText, Cj96I18n::getLanguage()) : "");
        mTestAdressTipsTextPtr->setVisible(visible);
    }
    if (mTestAdressTipsWindowPtr) {
        mTestAdressTipsWindowPtr->setVisible(visible);
    }
    sWindow5TestAddressTipVisible = visible;
}

static void setWindow5TestAddressTip(const char *pText) {
    setWindow5TestAddressTipWithColor(pText, WINDOW5_CONFIG_TIP_COLOR_NEUTRAL);
}

static void setWindow5TestAddressSuccessTip(const char *pText) {
    setWindow5TestAddressTipWithColor(pText, WINDOW5_CONFIG_TIP_COLOR_SUCCESS);
}

static void setWindow5TestAddressFailureTip(const char *pText) {
    setWindow5TestAddressTipWithColor(pText, WINDOW5_CONFIG_TIP_COLOR_FAILURE);
}

static long long getWindow5NowMs() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    return (static_cast<long long>(tv.tv_sec) * 1000LL) +
           (static_cast<long long>(tv.tv_usec) / 1000LL);
}

static void scheduleWindow5ValveSuccessTipAutoHide() {
    sWindow5ValveSuccessTipHideDeadlineMs = getWindow5NowMs() + 800LL;
}

static void hideWindow5TestAddressTipOnly() {
    sWindow5ValveSuccessTipHideDeadlineMs = 0;
    if (mTestAdressTipsTextPtr) {
        mTestAdressTipsTextPtr->setText("");
        mTestAdressTipsTextPtr->setVisible(false);
    }
    if (mTestAdressTipsWindowPtr) {
        mTestAdressTipsWindowPtr->setVisible(false);
    }
    sWindow5TestAddressTipVisible = false;
}

static void updateWindow5TestAddressTipAutoHide() {
    if (sWindow5ValveSuccessTipHideDeadlineMs <= 0) {
        return;
    }
    if (isWindow5ValveCommandBusy()) {
        return;
    }
    if (getWindow5NowMs() >= sWindow5ValveSuccessTipHideDeadlineMs) {
        hideWindow5TestAddressTipOnly();
    }
}

static bool isWindow5ValveCommandBusy() {
    return sWindow5ValveCommandBusy;
}

static unsigned int getWindow5ValveCommandCompletionSerial() {
    return sWindow5ValveCommandCompletionSerial;
}

static bool wasLastWindow5ValveCommandSuccessful() {
    return sWindow5ValveCommandLastSucceeded;
}

static void showWindow5ValveWaitTip() {
    char tip[96] = {0};
    const char *action = (sWindow5ValveCommandTargetState == 1U) ? "开" :
                         (sWindow5ValveCommandTargetState == 0U) ? "关" : "操作";
    if (sWindow5ValveCommandIsGroup && sWindow5ValveCommandGroupNo > 0) {
        snprintf(tip, sizeof(tip), "正在%s 阀组[%d]", action,
                 sWindow5ValveCommandGroupNo);
    } else if (sWindow5ValveCommandTargetAddress >= 0) {
        snprintf(tip, sizeof(tip), "正在%s 阀门[%d]", action,
                 sWindow5ValveCommandTargetAddress);
    } else {
        snprintf(tip, sizeof(tip), "正在%s阀门，请等待", action);
    }
    setWindow5TestAddressTipWithColor(tip, WINDOW5_CONFIG_TIP_COLOR_WAIT);
}

static void addWindow5ValveCommandPending(BYTE targetState,
                                          bool groupCommand,
                                          int groupNo,
                                          int targetAddress) {
    if (sWindow5ValveCommandPendingCount <= 0) {
        sWindow5ValveCommandPendingCount = 0;
        sWindow5ValveCommandHadFailure = false;
        sWindow5ValveCommandFinalState = 0xFFU;
        sWindow5ValveCommandTargetState = targetState;
        sWindow5ValveCommandIsGroup = groupCommand && groupNo > 0;
        sWindow5ValveCommandGroupNo = sWindow5ValveCommandIsGroup ? groupNo : 0;
        sWindow5ValveCommandTargetAddress = targetAddress;
    } else {
        if (sWindow5ValveCommandTargetState != targetState) {
            sWindow5ValveCommandTargetState = 0xFFU;
        }
        if (!groupCommand || !sWindow5ValveCommandIsGroup ||
            (sWindow5ValveCommandGroupNo != groupNo)) {
            sWindow5ValveCommandIsGroup = false;
            sWindow5ValveCommandGroupNo = 0;
        }
    }
    ++sWindow5ValveCommandPendingCount;
    sWindow5ValveCommandBusy = true;
    showWindow5ValveWaitTip();
}

static bool isWindow5ValveResultOk(const SWindow5Rs485Result &result,
                                    int expectedAddress) {
    if ((result.replyType != 1) || (result.status != 0U)) {
        return false;
    }
    if ((expectedAddress >= 0) && (result.returnedAddress >= 0) &&
        (result.returnedAddress != expectedAddress)) {
        return false;
    }
    // A complete state is preferred, but a valid ACK without state is still
    // a successful command for older AC/DC board firmware.
    return !result.hasReturnedDeviceState ||
           (result.returnedDeviceState <= 1U);
}

static void finishWindow5ValveCommandWait(const SWindow5Rs485Result &result,
                                          int expectedAddress) {
    const bool commandOk = isWindow5ValveResultOk(result, expectedAddress);
    LOGD("[Window5Rs485] valve result target=%d replyType=%d status=%u "
         "returnedAddress=%d hasType=%d type=%u hasState=%d state=%u accepted=%d\r\n",
         expectedAddress, result.replyType, result.status,
         result.returnedAddress, result.hasReturnedDecoderType ? 1 : 0,
         result.returnedDecoderType, result.hasReturnedDeviceState ? 1 : 0,
         result.returnedDeviceState, commandOk ? 1 : 0);
    if (commandOk) {
        if (result.hasReturnedDeviceState &&
            (result.returnedDeviceState <= 1U)) {
            sWindow5ValveCommandFinalState = result.returnedDeviceState;
        }
    } else {
        sWindow5ValveCommandHadFailure = true;
    }

    if (sWindow5ValveCommandPendingCount > 0) {
        --sWindow5ValveCommandPendingCount;
    }
    if (sWindow5ValveCommandPendingCount > 0) {
        showWindow5ValveWaitTip();
        return;
    }

    sWindow5ValveCommandPendingCount = 0;
    sWindow5ValveCommandBusy = false;
    sWindow5ValveCommandLastSucceeded = !sWindow5ValveCommandHadFailure;
    ++sWindow5ValveCommandCompletionSerial;
    char tip[96] = {0};
    const bool groupCommand = sWindow5ValveCommandIsGroup &&
                              sWindow5ValveCommandGroupNo > 0;
    const int groupNo = sWindow5ValveCommandGroupNo;
    const int address = sWindow5ValveCommandTargetAddress;
    const char *subject = groupCommand ? "阀组" : "阀门";
    const int subjectNo = groupCommand ? groupNo : address;
    if (sWindow5ValveCommandHadFailure) {
        sWindow5ValveCommandHadFailure = false;
        if (subjectNo >= 0) {
            snprintf(tip, sizeof(tip), "%s[%d]操作超时", subject, subjectNo);
        } else {
            snprintf(tip, sizeof(tip), "阀门操作超时");
        }
        setWindow5TestAddressFailureTip(tip);
        // Failure notices should also be transient so a timeout cannot leave
        // a blocking-looking popup on screen indefinitely.
        scheduleWindow5ValveSuccessTipAutoHide();
    } else {
        if (subjectNo >= 0 && sWindow5ValveCommandFinalState <= 1U) {
            snprintf(tip, sizeof(tip), "%s[%d]已%s", subject, subjectNo,
                     sWindow5ValveCommandFinalState == 1U ? "开启" : "关闭");
        } else if (groupCommand) {
            snprintf(tip, sizeof(tip), "阀组[%d]动作完成", groupNo);
        } else if (address >= 0) {
            snprintf(tip, sizeof(tip), "阀门[%d]动作完成", address);
        } else {
            snprintf(tip, sizeof(tip), "阀门动作完成");
        }
        setWindow5TestAddressSuccessTip(tip);
        scheduleWindow5ValveSuccessTipAutoHide();
    }
    sWindow5ValveCommandTargetState = 0xFFU;
    sWindow5ValveCommandIsGroup = false;
    sWindow5ValveCommandGroupNo = 0;
    sWindow5ValveCommandTargetAddress = -1;
}

static bool hideWindow5TestAddressTipIfVisible() {
    if (isWindow5ValveCommandBusy()) {
        // Keep the wait prompt visible, but do not swallow global touch events.
        // Navigation buttons must remain usable while a valve worker waits for a reply.
        showWindow5ValveWaitTip();
        return false;
    }

    if (!sWindow5TestAddressTipVisible) {
        return false;
    }

    hideWindow5TestAddressTipOnly();
    return true;
}

static bool getWindow5SelectedDecoderType(BYTE *pDecoderType) {
    if (pDecoderType == NULL) {
        return false;
    }

    const int checkedID = mRadioGroup1Ptr ? mRadioGroup1Ptr->getCheckedID() : 0;
    if (checkedID == ID_MAIN_ValueRadioButton) {
        *pDecoderType = WINDOW5_DECODER_TYPE_VALUE;
        return true;
    }
    if (checkedID == ID_MAIN_SenserRadioButton) {
        *pDecoderType = WINDOW5_DECODER_TYPE_SENSER;
        return true;
    }

    // The FTU has ValueRadioButton visually checked by default, but the
    // generated RadioGroup can report no checkedID until the first change
    // event. Keep the runtime default consistent with the visible UI.
    LOGD("[Window5Rs485] decoder type not initialized, default to value, checkedID=%d\r\n",
         checkedID);
    *pDecoderType = WINDOW5_DECODER_TYPE_VALUE;
    return true;
}

static const char* getWindow5DecoderTypeText(BYTE decoderType) {
    switch (decoderType) {
    case WINDOW5_DECODER_TYPE_VALUE:
        return Cj96I18n::translateRuntimeText("电磁阀", Cj96I18n::getLanguage());
    case WINDOW5_DECODER_TYPE_SENSER:
        return Cj96I18n::translateRuntimeText("传感器", Cj96I18n::getLanguage());
    default:
        return Cj96I18n::translateRuntimeText("未知", Cj96I18n::getLanguage());
    }
}

static const char* getWindow5DecoderDisplayText(BYTE decoderType) {
    if ((sWindow5SelectedDecoderLabel != NULL) &&
        (sWindow5SelectedDecoderLabel[0] != '\0') &&
        (sWindow5SelectedDecoderLabelType == decoderType)) {
        const char* label = sWindow5SelectedDecoderLabel;
        return Cj96I18n::translateRuntimeText(label, Cj96I18n::getLanguage());
    }
    return getWindow5DecoderTypeText(decoderType);
}

static void setWindow5TypePopupButtonsVisible(bool sensorMode) {
    ZKButton* sensorButtons[] = {
        mWindow5TypeRainButtonPtr,
        mWindow5TypeHumidityButtonPtr,
        mWindow5TypePressureButtonPtr,
        mWindow5TypeFlowButtonPtr,
    };
    for (size_t i = 0; i < sizeof(sensorButtons) / sizeof(sensorButtons[0]); ++i) {
        if (sensorButtons[i]) {
            sensorButtons[i]->setVisible(sensorMode);
            sensorButtons[i]->setTouchable(sensorMode);
        }
    }

    ZKButton* valveButtons[] = {
        mWindow5TypeACValveButtonPtr,
        mWindow5TypeDCValveButtonPtr,
    };
    for (size_t i = 0; i < sizeof(valveButtons) / sizeof(valveButtons[0]); ++i) {
        if (valveButtons[i]) {
            valveButtons[i]->setVisible(!sensorMode);
            valveButtons[i]->setTouchable(!sensorMode);
        }
    }
}

static void showWindow5TypePopup(bool sensorMode) {
    if (mWindow5TypePopupTitleTextPtr) {
        mWindow5TypePopupTitleTextPtr->setText(sensorMode ?
            Cj96I18n::translateRuntimeText("选择传感器类型", Cj96I18n::getLanguage()) :
            Cj96I18n::translateRuntimeText("选择电磁阀类型", Cj96I18n::getLanguage()));
        mWindow5TypePopupTitleTextPtr->setVisible(true);
    }
    setWindow5TypePopupButtonsVisible(sensorMode);
    if (mWindow10Ptr) {
        mWindow10Ptr->setVisible(true);
    }
    sWindow5TypePopupVisible = true;
}

static void hideWindow5TypePopupOnly() {
    if (mWindow5TypePopupTitleTextPtr) {
        mWindow5TypePopupTitleTextPtr->setVisible(false);
    }
    ZKButton* allButtons[] = {
        mWindow5TypeRainButtonPtr,
        mWindow5TypeHumidityButtonPtr,
        mWindow5TypePressureButtonPtr,
        mWindow5TypeFlowButtonPtr,
        mWindow5TypeACValveButtonPtr,
        mWindow5TypeDCValveButtonPtr,
    };
    for (size_t i = 0; i < sizeof(allButtons) / sizeof(allButtons[0]); ++i) {
        if (allButtons[i]) {
            allButtons[i]->setVisible(false);
            allButtons[i]->setTouchable(false);
        }
    }
    if (mWindow10Ptr) {
        mWindow10Ptr->setVisible(false);
    }
    sWindow5TypePopupVisible = false;
}

static void selectWindow5DecoderSubtype(BYTE decoderType, const char *pLabel) {
    sWindow5SelectedDecoderLabelType = decoderType;
    sWindow5SelectedDecoderLabel = pLabel ? pLabel : getWindow5DecoderTypeText(decoderType);
    hideWindow5TypePopupOnly();
    updateWindow5DecoderTypeTitle();
}

static void updateWindow5DecoderTypeTitle() {
    if (!mButton40Ptr) {
        return;
    }

    BYTE decoderType = WINDOW5_DECODER_TYPE_VALUE;
    const char *pText = "电磁阀";
    const int checkedID = mRadioGroup1Ptr ? mRadioGroup1Ptr->getCheckedID() : 0;
    if (checkedID == ID_MAIN_SenserRadioButton) {
        decoderType = WINDOW5_DECODER_TYPE_SENSER;
    }
    pText = getWindow5DecoderDisplayText(decoderType);

    char title[64] = {0};
    snprintf(title, sizeof(title), Cj96I18n::translateRuntimeText(
             "解码器类型：%s", Cj96I18n::getLanguage()), pText);
    mButton40Ptr->setText(title);
}

static void setWindow5ConfigTip(int address, BYTE decoderType, const char *pStatusText) {
    char tip[160] = {0};
    int textColor = WINDOW5_CONFIG_TIP_COLOR_NEUTRAL;
    if ((pStatusText != NULL) && (strcmp(pStatusText, Cj96I18n::translateRuntimeText("成功", Cj96I18n::getLanguage())) == 0)) {
        textColor = WINDOW5_CONFIG_TIP_COLOR_SUCCESS;
    } else if ((pStatusText != NULL) &&
               (strncmp(pStatusText, Cj96I18n::translateRuntimeText("失败", Cj96I18n::getLanguage()), strlen(Cj96I18n::translateRuntimeText("失败", Cj96I18n::getLanguage()))) == 0)) {
        textColor = WINDOW5_CONFIG_TIP_COLOR_FAILURE;
    }
    char fmtBuf[64] = {0};
    snprintf(fmtBuf, sizeof(fmtBuf), "%s",
             Cj96I18n::translateRuntimeText("地址：%d\r\n类型：%s\r\n%s", Cj96I18n::getLanguage()));
    snprintf(tip, sizeof(tip), fmtBuf,
             address, getWindow5DecoderDisplayText(decoderType),
             pStatusText ?
             Cj96I18n::translateRuntimeText(pStatusText, Cj96I18n::getLanguage()) : "");
    setWindow5TestAddressTipWithColor(tip, textColor);
}

static bool isWindow5AsciiSpace(char c) {
    return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

static bool normalizeWindow5AddressText(const std::string &text, long *pValue) {
    if (pValue == NULL) {
        return false;
    }

    const char *pStart = text.c_str();
    while (isWindow5AsciiSpace(*pStart)) {
        ++pStart;
    }
    if (*pStart == '\0') {
        return false;
    }

    errno = 0;
    char *pEnd = NULL;
    const long value = strtol(pStart, &pEnd, 10);
    while ((pEnd != NULL) && isWindow5AsciiSpace(*pEnd)) {
        ++pEnd;
    }
    if (((errno != 0) && (errno != ERANGE)) ||
        (pEnd == pStart) || ((pEnd != NULL) && (*pEnd != '\0'))) {
        return false;
    }

    if ((errno == ERANGE) || (value > WINDOW5_CONFIG_ADDRESS_MAX)) {
        *pValue = (value < 0) ? WINDOW5_CONFIG_ADDRESS_MIN :
                                WINDOW5_CONFIG_ADDRESS_MAX;
    } else if (value < WINDOW5_CONFIG_ADDRESS_MIN) {
        *pValue = WINDOW5_CONFIG_ADDRESS_MIN;
    } else {
        *pValue = value;
    }
    return true;
}

static bool normalizeWindow5SourceAddressText(const std::string &text, long *pValue) {
    if (pValue == NULL) {
        return false;
    }

    const char *pStart = text.c_str();
    while (isWindow5AsciiSpace(*pStart)) {
        ++pStart;
    }
    if (*pStart == '\0') {
        return false;
    }

    errno = 0;
    char *pEnd = NULL;
    const long value = strtol(pStart, &pEnd, 10);
    while ((pEnd != NULL) && isWindow5AsciiSpace(*pEnd)) {
        ++pEnd;
    }
    if (((errno != 0) && (errno != ERANGE)) ||
        (pEnd == pStart) || ((pEnd != NULL) && (*pEnd != '\0'))) {
        return false;
    }

    if (value == WINDOW5_DEFAULT_UNCONFIGURED_ADDRESS) {
        *pValue = WINDOW5_DEFAULT_UNCONFIGURED_ADDRESS;
    } else if ((errno == ERANGE) || (value > WINDOW5_CONFIG_ADDRESS_MAX)) {
        *pValue = (value < 0) ? WINDOW5_CONFIG_ADDRESS_MIN :
                                WINDOW5_CONFIG_ADDRESS_MAX;
    } else if (value < WINDOW5_CONFIG_ADDRESS_MIN) {
        *pValue = WINDOW5_CONFIG_ADDRESS_MIN;
    } else {
        *pValue = value;
    }
    return true;
}

static bool normalizeWindow5SourceAddressTextForEdit(const std::string &text, long *pValue) {
    if (pValue == NULL) {
        return false;
    }

    const char *pStart = text.c_str();
    while (isWindow5AsciiSpace(*pStart)) {
        ++pStart;
    }
    if (*pStart == '\0') {
        return false;
    }

    const char *pEnd = pStart;
    while (*pEnd >= '0' && *pEnd <= '9') {
        ++pEnd;
    }
    const size_t digitCount = static_cast<size_t>(pEnd - pStart);
    const char *pAfterDigits = pEnd;
    while (isWindow5AsciiSpace(*pAfterDigits)) {
        ++pAfterDigits;
    }
    const bool onlySpacesAfterDigits = (*pAfterDigits == '\0');
    if (onlySpacesAfterDigits && (digitCount > 0U) && (digitCount <= 4U)) {
        bool prefixOfDefault = true;
        for (size_t i = 0; i < digitCount; ++i) {
            if (pStart[i] != '8') {
                prefixOfDefault = false;
                break;
            }
        }
        if (prefixOfDefault) {
            char temp[8] = {0};
            memcpy(temp, pStart, digitCount);
            *pValue = strtol(temp, NULL, 10);
            return true;
        }
    }

    return normalizeWindow5SourceAddressText(text, pValue);
}

static void setWindow5NormalizedAddressText(long value) {
    if (!mTestAdressEditTextPtr) {
        return;
    }

    char normalizedText[16] = {0};
    snprintf(normalizedText, sizeof(normalizedText), "%ld", value);
    if (mTestAdressEditTextPtr->getText() == normalizedText) {
        return;
    }

    sWindow5AddressTextUpdating = true;
    mTestAdressEditTextPtr->setText(normalizedText);
	sWindow5AddressTextUpdating = false;
}

static void setWindow5NormalizedSourceAddressText(long value) {
    if (!mSrouceAddressEditTextPtr) {
        return;
    }

    char normalizedText[16] = {0};
    snprintf(normalizedText, sizeof(normalizedText), "%ld", value);
    if (mSrouceAddressEditTextPtr->getText() == normalizedText) {
        return;
    }

    sWindow5SourceAddressTextUpdating = true;
    mSrouceAddressEditTextPtr->setText(normalizedText);
    sWindow5SourceAddressTextUpdating = false;
}

static void setWindow5NormalizedValveAddressText(long value) {
    if (!mValveAddressEditTextPtr) {
        return;
    }

    char normalizedText[16] = {0};
    snprintf(normalizedText, sizeof(normalizedText), "%ld", value);
    if (mValveAddressEditTextPtr->getText() == normalizedText) {
        return;
    }

    sWindow5ValveAddressTextUpdating = true;
    mValveAddressEditTextPtr->setText(normalizedText);
    sWindow5ValveAddressTextUpdating = false;
}

static bool parseWindow5ValveAddressEditText(int *pAddress) {
    if (pAddress == NULL) {
        return false;
    }

    if (!mValveAddressEditTextPtr) {
        setWindow5TestAddressFailureTip("阀地址输入框无效");
        return false;
    }

    const std::string text = mValveAddressEditTextPtr->getText();
    const char *pStart = text.c_str();
    while (isWindow5AsciiSpace(*pStart)) {
        ++pStart;
    }

    if (*pStart == '\0') {
        setWindow5TestAddressFailureTip("请输入阀地址\r\n范围20-255");
        return false;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    if (!normalizeWindow5AddressText(text, &normalizedValue)) {
        setWindow5TestAddressFailureTip("阀地址格式错误\r\n请输入20-255");
        return false;
    }

    setWindow5NormalizedValveAddressText(normalizedValue);
    *pAddress = static_cast<int>(normalizedValue);
    return true;
}

static void handleWindow5ValveAddressTextChanged(const std::string &text) {
    setWindow5TestAddressTip("");
    if (sWindow5ValveAddressTextUpdating) {
        return;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    (void)normalizeWindow5AddressText(text, &normalizedValue);
    setWindow5NormalizedValveAddressText(normalizedValue);
}

static bool stepWindow5ValveAddress(int delta) {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    int address = WINDOW5_CONFIG_ADDRESS_MIN;
    if (mValveAddressEditTextPtr) {
        long parsedValue = WINDOW5_CONFIG_ADDRESS_MIN;
        if (normalizeWindow5AddressText(mValveAddressEditTextPtr->getText(), &parsedValue)) {
            address = static_cast<int>(parsedValue);
        }
    }

    address += delta;
    if (address < WINDOW5_CONFIG_ADDRESS_MIN) {
        address = WINDOW5_CONFIG_ADDRESS_MIN;
    } else if (address > WINDOW5_CONFIG_ADDRESS_MAX) {
        address = WINDOW5_CONFIG_ADDRESS_MAX;
    }

    setWindow5NormalizedValveAddressText(address);
    setWindow5TestAddressTip("");
    return true;
}

static bool parseWindow5TestAddressEditText(int *pAddress) {
    if (pAddress == NULL) {
        return false;
    }

    if (!mTestAdressEditTextPtr) {
        setWindow5TestAddressFailureTip("地址输入框无效");
        return false;
    }

    const std::string text = mTestAdressEditTextPtr->getText();
    const char *pStart = text.c_str();
    while (isWindow5AsciiSpace(*pStart)) {
        ++pStart;
    }

    if (*pStart == '\0') {
        setWindow5TestAddressFailureTip("请输入地址\r\n范围20-255");
        return false;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    if (!normalizeWindow5AddressText(text, &normalizedValue)) {
        setWindow5TestAddressFailureTip("地址格式错误\r\n请输入20-255");
        return false;
    }

    setWindow5NormalizedAddressText(normalizedValue);
    *pAddress = static_cast<int>(normalizedValue);
    return true;
}

static bool parseWindow5SourceAddressEditText(int *pAddress) {
    if (pAddress == NULL) {
        return false;
    }

    if (!mSrouceAddressEditTextPtr) {
        setWindow5TestAddressFailureTip("源地址输入框无效");
        return false;
    }

    const std::string text = mSrouceAddressEditTextPtr->getText();
    const char *pStart = text.c_str();
    while (isWindow5AsciiSpace(*pStart)) {
        ++pStart;
    }

    if (*pStart == '\0') {
        setWindow5TestAddressFailureTip("请输入源地址\r\n范围20-255或8888");
        return false;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    if (!normalizeWindow5SourceAddressText(text, &normalizedValue)) {
        setWindow5TestAddressFailureTip("源地址格式错误\r\n请输入20-255或8888");
        return false;
    }

    setWindow5NormalizedSourceAddressText(normalizedValue);
    *pAddress = static_cast<int>(normalizedValue);
    return true;
}

static const char* getWindow5AddressStatusText(BYTE status) {
    switch (status) {
    case 0U:
        return Cj96I18n::translateRuntimeText("成功", Cj96I18n::getLanguage());
    case 1U:
        return Cj96I18n::translateRuntimeText("数据长度错误", Cj96I18n::getLanguage());
    case 2U:
        return Cj96I18n::translateRuntimeText("地址越界", Cj96I18n::getLanguage());
    case 3U:
        return Cj96I18n::translateRuntimeText("从机固件不支持类型配置", Cj96I18n::getLanguage());
    case 4U:
        return Cj96I18n::translateRuntimeText("EEPROM保存失败", Cj96I18n::getLanguage());
    case 5U:
        return Cj96I18n::translateRuntimeText("解码器类型错误", Cj96I18n::getLanguage());
    default:
        return Cj96I18n::translateRuntimeText("未知错误", Cj96I18n::getLanguage());
    }
}

static void setWindow5ConfigFailureTip(int requestedAddress,
                                       BYTE requestedDecoderType,
                                       const SWindow5Rs485Result &result,
                                       const char *pReason) {
    const int displayAddress = (result.returnedAddress >= 0) ?
                               result.returnedAddress : requestedAddress;
    const BYTE displayDecoderType = result.hasReturnedDecoderType ?
                                    result.returnedDecoderType : requestedDecoderType;
    char statusText[96] = {0};
    // pReason is a complete table entry like "失败：地址不匹配"
    // setWindow5ConfigTip will translate it via translateRuntimeText
    snprintf(statusText, sizeof(statusText), "%s",
             pReason ? pReason : Cj96I18n::translateRuntimeText("未知错误", Cj96I18n::getLanguage()));
    setWindow5ConfigTip(displayAddress, displayDecoderType, statusText);
}

static void setWindow5AddressNoReplyTip(int address,
                                        BYTE decoderType,
                                        const SWindow5Rs485Result &result) {
    setWindow5ConfigFailureTip(address, decoderType, result,
                              result.sendOk ? "未收到应答" : "发送失败");
}

static bool checkWindow5ValveAddressReady(int address, BYTE decoderType) {
    BYTE requestData[2] = {0};
    putWindow5Address(requestData, address);

    LOGD("[Window5Rs485] manual valve precheck address=%d decoderType=%u\r\n",
         address, decoderType);
    setWindow5ConfigTip(address, decoderType, "正在检测");
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
        WINDOW5_CMD_GET_DEVICE_STATE, requestData, sizeof(requestData),
        "CHECK_VALVE");

    if (result.replyType == 0) {
        setWindow5AddressNoReplyTip(address, decoderType, result);
        return false;
    }

    if ((result.replyType == 1) && (result.status == 0U)) {
        if ((result.returnedAddress >= 0) && (result.returnedAddress != address)) {
            LOGD("[Window5Rs485] manual valve precheck address mismatch expected=%d actual=%d\r\n",
                 address, result.returnedAddress);
            setWindow5ConfigTip(result.returnedAddress,
                                result.hasReturnedDecoderType ?
                                result.returnedDecoderType : decoderType,
                                "失败：地址不匹配");
            return false;
        }
        if (result.hasReturnedDecoderType &&
            (result.returnedDecoderType != decoderType)) {
            LOGD("[Window5Rs485] manual valve precheck type mismatch address=%d expected=%u actual=%u\r\n",
                 address, decoderType, result.returnedDecoderType);
            setWindow5ConfigTip(address, result.returnedDecoderType,
                                "失败：类型不匹配");
            return false;
        }
        return true;
    }

    setWindow5ConfigFailureTip(address, decoderType, result,
                               getWindow5AddressStatusText(result.status));
    return false;
}

static bool checkWindow5AddressOccupied(int address, SWindow5Rs485Result *pResult) {
    BYTE requestData[2] = {0};
    putWindow5Address(requestData, address);
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
        WINDOW5_CMD_GET_CONFIG, requestData, sizeof(requestData), "CHECK_ADDRESS_OCCUPIED");
    if (pResult != NULL) {
        *pResult = result;
    }

    if (result.replyType == 0) {
        return false;
    }

    if (result.returnedAddress == address) {
        return true;
    }

    // A valid reply to this addressed query means a device handled the target
    // address even when older firmware omits or misreports returnedAddress.
    return (result.replyType == 1) || (result.replyType == 2);
}

static void handleWindow5TestAddressTextChanged(const std::string &text) {
    // Programmatic normalization after a successful address change must not
    // clear the success popup that was just shown.
    if (sWindow5AddressTextUpdating) {
        return;
    }
    setWindow5TestAddressTip("");

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    (void)normalizeWindow5AddressText(text, &normalizedValue);
    setWindow5NormalizedAddressText(normalizedValue);
}

static void handleWindow5SourceAddressTextChanged(const std::string &text) {
    if (sWindow5SourceAddressTextUpdating) {
        return;
    }
    setWindow5TestAddressTip("");

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    (void)normalizeWindow5SourceAddressTextForEdit(text, &normalizedValue);
    setWindow5NormalizedSourceAddressText(normalizedValue);
}

static bool stepWindow5TestAddress(int delta) {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    int address = WINDOW5_CONFIG_ADDRESS_MIN;
    if (mTestAdressEditTextPtr) {
        long parsedValue = WINDOW5_CONFIG_ADDRESS_MIN;
        if (normalizeWindow5AddressText(mTestAdressEditTextPtr->getText(), &parsedValue)) {
            address = static_cast<int>(parsedValue);
        }
    }

    address += delta;
    if (address < WINDOW5_CONFIG_ADDRESS_MIN) {
        address = WINDOW5_CONFIG_ADDRESS_MIN;
    } else if (address > WINDOW5_CONFIG_ADDRESS_MAX) {
        address = WINDOW5_CONFIG_ADDRESS_MAX;
    }

    setWindow5NormalizedAddressText(address);
    setWindow5TestAddressTip("");
    return true;
}

static bool stepWindow5SourceAddress(int delta) {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    int address = WINDOW5_CONFIG_ADDRESS_MIN;
    if (mSrouceAddressEditTextPtr) {
        long parsedValue = WINDOW5_CONFIG_ADDRESS_MIN;
        if (normalizeWindow5AddressText(mSrouceAddressEditTextPtr->getText(), &parsedValue)) {
            address = static_cast<int>(parsedValue);
        }
    }

    address += delta;
    if (address < WINDOW5_CONFIG_ADDRESS_MIN) {
        address = WINDOW5_CONFIG_ADDRESS_MIN;
    } else if (address > WINDOW5_CONFIG_ADDRESS_MAX) {
        address = WINDOW5_CONFIG_ADDRESS_MAX;
    }

    setWindow5NormalizedSourceAddressText(address);
    setWindow5TestAddressTip("");
    return true;
}

static bool sendWindow5SetConfigCommandLegacy() {
    BYTE decoderType = 0U;
    if (!getWindow5SelectedDecoderType(&decoderType)) {
        return false;
    }

    int address = 0;
    if (!parseWindow5TestAddressEditText(&address)) {
        return false;
    }

    BYTE data[3] = {0};
    putWindow5Address(data, address);
    data[2] = decoderType;

    LOGD("[Window5Rs485] set config request address=%d decoderType=%u\r\n", address, decoderType);
    setWindow5ConfigTip(address, decoderType, "正在修改");
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
        WINDOW5_CMD_SET_CONFIG, data, sizeof(data), "SET_CONFIG");

    if (result.replyType == 0) {
        setWindow5AddressNoReplyTip(address, decoderType, result);
        return false;
    }

    if ((result.replyType == 1) && (result.status == 0U)) {
        if (!result.hasReturnedDecoderType) {
            setWindow5ConfigTip(address, decoderType, "失败：应答缺少类型");
            return false;
        }

        if ((result.returnedAddress == address) &&
            (result.returnedDecoderType == decoderType)) {
            setWindow5ConfigTip(address, decoderType, "成功");
            return true;
        }

        setWindow5ConfigTip(result.returnedAddress, result.returnedDecoderType,
                            "失败：配置不匹配");
        return false;
    }

    setWindow5ConfigFailureTip(address, decoderType, result,
                               getWindow5AddressStatusText(result.status));
    return false;
}

static bool sendWindow5SetAddressCommand() {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    int sourceAddress = 0;
    int destAddress = 0;
    if (!parseWindow5SourceAddressEditText(&sourceAddress)) {
        return false;
    }
    if (!parseWindow5TestAddressEditText(&destAddress)) {
        return false;
    }
    if (sourceAddress == destAddress) {
        setWindow5TestAddressTipWithColor("源地址和目标地址相同\r\n请更换目标地址",
                                          WINDOW5_CONFIG_TIP_COLOR_FAILURE);
        return false;
    }

    setWindow5TestAddressTip("正在检测目标地址");
    SWindow5Rs485Result occupiedResult = makeWindow5Rs485Result();
    if (checkWindow5AddressOccupied(destAddress, &occupiedResult)) {
        char tip[128] = {0};
        BYTE displayType = WINDOW5_DECODER_TYPE_VALUE;
        if (occupiedResult.hasReturnedDecoderType) {
            displayType = occupiedResult.returnedDecoderType;
        }
        char fmtBuf[96] = {0};
        snprintf(fmtBuf, sizeof(fmtBuf), "%s",
                 Cj96I18n::translateRuntimeText("目标地址%d已有设备\r\n类型：%s\r\n请更换目标地址", Cj96I18n::getLanguage()));
        snprintf(tip, sizeof(tip), fmtBuf,
                 destAddress, getWindow5DecoderDisplayText(displayType));
        setWindow5TestAddressTipWithColor(tip, WINDOW5_CONFIG_TIP_COLOR_FAILURE);
        LOGD("[Window5Rs485] refuse set address source=%d dest=%d, target occupied, replyType=%d status=%u type=%u hasType=%d\r\n",
             sourceAddress, destAddress, occupiedResult.replyType,
             occupiedResult.status, occupiedResult.returnedDecoderType,
             occupiedResult.hasReturnedDecoderType);
        return false;
    }

    BYTE data[4] = {0};
    putWindow5Address(data, sourceAddress);
    putWindow5Address(data + 2, destAddress);
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
        WINDOW5_CMD_SET_ADDRESS, data, sizeof(data), "SET_ADDRESS");
    if ((result.replyType == 1) && (result.status == 0U) &&
        (result.returnedAddress == destAddress)) {
        setWindow5TestAddressSuccessTip("地址修改成功");
        if (sourceAddress == WINDOW5_DEFAULT_UNCONFIGURED_ADDRESS) {
            const int nextAddress = (destAddress < WINDOW5_CONFIG_ADDRESS_MAX) ?
                                    (destAddress + 1) : WINDOW5_CONFIG_ADDRESS_MAX;
            setWindow5NormalizedAddressText(nextAddress);
        }
        return true;
    }
    setWindow5TestAddressFailureTip(result.sendOk ? "源地址无响应或修改失败" : "发送失败");
    return false;
}

static bool sendWindow5ForceSetAddressCommand() {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    int destAddress = 0;
    if (!parseWindow5TestAddressEditText(&destAddress)) {
        return false;
    }

    BYTE data[2] = {0};
    putWindow5Address(data, destAddress);
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
        WINDOW5_CMD_SET_ADDRESS, data, sizeof(data), "FORCE_ADDRESS");
    if ((result.replyType == 1) && (result.status == 0U) &&
        (result.returnedAddress == destAddress)) {
        setWindow5TestAddressSuccessTip("强制修改地址成功");
        return true;
    }

    if (result.replyType == 1) {
        char tip[128] = {0};
        if (result.status == 0U) {
            char fmtBuf[128] = {0};
        snprintf(fmtBuf, sizeof(fmtBuf), "%s",
                 Cj96I18n::translateRuntimeText("强制修改回包地址不匹配\r\n返回地址%d，请用新地址核对", Cj96I18n::getLanguage()));
        snprintf(tip, sizeof(tip), fmtBuf, result.returnedAddress);
        } else {
            char fmtBuf[96] = {0};
        snprintf(fmtBuf, sizeof(fmtBuf), "%s",
                 Cj96I18n::translateRuntimeText("强制修改失败：%s", Cj96I18n::getLanguage()));
        snprintf(tip, sizeof(tip), fmtBuf, getWindow5AddressStatusText(result.status));
        }
        setWindow5TestAddressTipWithColor(tip, WINDOW5_CONFIG_TIP_COLOR_FAILURE);
        return false;
    }

    if (result.replyType == 2) {
        char tip[128] = {0};
        char fmtBuf[128] = {0};
        snprintf(fmtBuf, sizeof(fmtBuf), "%s",
                 Cj96I18n::translateRuntimeText("强制修改被从机拒绝\r\n原因：%s", Cj96I18n::getLanguage()));
        snprintf(tip, sizeof(tip), fmtBuf, getWindow5AddressStatusText(result.status));
        setWindow5TestAddressTipWithColor(tip, WINDOW5_CONFIG_TIP_COLOR_FAILURE);
        return false;
    }

    if (result.sendOk) {
        setWindow5TestAddressFailureTip("强制命令已发送\r\n未收到确认，请用新地址核对");
        return false;
    }

    setWindow5TestAddressFailureTip("强制修改命令发送失败");
    return false;
}

static bool sendWindow5CheckAddressCommand() {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    BYTE expectedDecoderType = 0U;
    if (!getWindow5SelectedDecoderType(&expectedDecoderType)) {
        return false;
    }

    int expectedAddress = 0;
    if (!parseWindow5TestAddressEditText(&expectedAddress)) {
        return false;
    }

    LOGD("[Window5Rs485] check config request address=%d decoderType=%u\r\n",
         expectedAddress, expectedDecoderType);
    setWindow5ConfigTip(expectedAddress, expectedDecoderType, "正在核对");
    BYTE requestData[2] = {0};
    putWindow5Address(requestData, expectedAddress);
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
        WINDOW5_CMD_GET_CONFIG, requestData, sizeof(requestData), "GET_CONFIG");

    if (result.replyType == 0) {
        setWindow5AddressNoReplyTip(expectedAddress, expectedDecoderType, result);
        return false;
    }

    if ((result.replyType == 1) && (result.status == 0U)) {
        if (!result.hasReturnedDecoderType) {
            setWindow5ConfigTip(expectedAddress, expectedDecoderType,
                                "失败：固件不支持类型");
            return false;
        }

        if ((result.returnedAddress == expectedAddress) &&
            (result.returnedDecoderType == expectedDecoderType)) {
            setWindow5ConfigTip(expectedAddress, expectedDecoderType, "成功");
            return true;
        }

        setWindow5ConfigTip(result.returnedAddress, result.returnedDecoderType,
                            "失败：配置不匹配");
        return false;
    }

    setWindow5ConfigFailureTip(expectedAddress, expectedDecoderType, result,
                               getWindow5AddressStatusText(result.status));
    return false;
}

static bool requestWindow5CheckConfigForW2Add(int address, bool sensor,
                                              char *pMessage, size_t messageSize) {
    if (pMessage && messageSize > 0U) {
        pMessage[0] = '\0';
    }
    if (address < WINDOW5_CONFIG_ADDRESS_MIN || address > WINDOW5_CONFIG_ADDRESS_MAX) {
        if (pMessage && messageSize > 0U) {
            snprintf(pMessage, messageSize, "%s",
            Cj96I18n::translateRuntimeText("地址范围20-255", Cj96I18n::getLanguage()));
        }
        return false;
    }
    if (isWindow5ValveCommandBusy()) {
        if (pMessage && messageSize > 0U) {
            snprintf(pMessage, messageSize, "%s",
            Cj96I18n::translateRuntimeText("请等待当前指令完成", Cj96I18n::getLanguage()));
        }
        return false;
    }

    const BYTE expectedDecoderType = sensor ?
            WINDOW5_DECODER_TYPE_SENSER : WINDOW5_DECODER_TYPE_VALUE;
    if (!beginWindow5ManualConfigTransaction()) {
        if (pMessage && messageSize > 0U) {
            snprintf(pMessage, messageSize, "%s",
                Cj96I18n::translateRuntimeText(
                    "请等待当前指令完成", Cj96I18n::getLanguage()));
        }
        return false;
    }

    BYTE requestData[2] = {0};
    putWindow5Address(requestData, address);
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
        WINDOW5_CMD_GET_CONFIG, requestData, sizeof(requestData), "W2_ADD_GET_CONFIG");
    endWindow5ManualConfigTransaction();

    if (result.replyType == 0) {
        if (pMessage && messageSize > 0U) {
            snprintf(pMessage, messageSize, "%s",
            result.sendOk ? Cj96I18n::translateRuntimeText("地址无应答", Cj96I18n::getLanguage()) : Cj96I18n::translateRuntimeText("测试指令发送失败", Cj96I18n::getLanguage()));
        }
        return false;
    }

    if ((result.replyType == 1) && (result.status == 0U)) {
        if (!result.hasReturnedDecoderType) {
            if (pMessage && messageSize > 0U) {
                snprintf(pMessage, messageSize, "%s",
            Cj96I18n::translateRuntimeText("固件未返回设备类型", Cj96I18n::getLanguage()));
            }
            return false;
        }
        if (result.returnedAddress != address) {
            if (pMessage && messageSize > 0U) {
                char fmtBuf[64] = {0};
        snprintf(fmtBuf, sizeof(fmtBuf), "%s",
            Cj96I18n::translateRuntimeText("返回地址%d不匹配", Cj96I18n::getLanguage()));
        snprintf(pMessage, messageSize, fmtBuf, result.returnedAddress);
            }
            return false;
        }
        if (result.returnedDecoderType != expectedDecoderType) {
            if (pMessage && messageSize > 0U) {
                snprintf(pMessage, messageSize, Cj96I18n::translateRuntimeText(
            "设备类型不匹配\r\n地址%d为%s", Cj96I18n::getLanguage()),
                         address, getWindow5DecoderTypeText(result.returnedDecoderType));
            }
            return false;
        }
        if (pMessage && messageSize > 0U) {
            snprintf(pMessage, messageSize, "%s",
            Cj96I18n::translateRuntimeText("测试通过", Cj96I18n::getLanguage()));
        }
        return true;
    }

    if (pMessage && messageSize > 0U) {
        char fmtBuf[64] = {0};
        snprintf(fmtBuf, sizeof(fmtBuf), "%s",
            Cj96I18n::translateRuntimeText("测试失败：%s", Cj96I18n::getLanguage()));
        snprintf(pMessage, messageSize, fmtBuf,
                 getWindow5AddressStatusText(result.status));
    }
    return false;
}

static void onPage5Show() {
    if (mValveAddressEditTextPtr && mValveAddressEditTextPtr->getText().empty()) {
        setWindow5NormalizedValveAddressText(WINDOW5_CONFIG_ADDRESS_MIN);
    }
    updateWindow5DecoderTypeTitle();
    setWindow5TestAddressTip("");
    hideWindow5TypePopupOnly();

    // A legacy direct Window5 entry can bypass showMainPage(). In that case
    // the current page is still Window2, so keep the leave warning visible.
    if (mWindow2Ptr && mWindow2Ptr->isWndShow() && !sW2TipWindowVisible) {
        sW2TipCheckedThisVisit = true;
        showW2UngroupedValveTipIfNeeded();
    }
}

static void onPage5Hide() {
    setWindow5TestAddressTip("");
    hideWindow5TypePopupOnly();
}
