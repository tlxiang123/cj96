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
#include <termio.h>
#include <unistd.h>
#include <vector>

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
static const BYTE WINDOW5_CONFIG_ADDRESS_MIN = 20U;
static const int WINDOW5_CONFIG_ADDRESS_MAX = 255;
static const int WINDOW5_DEFAULT_UNCONFIGURED_ADDRESS = 8888;
static const BYTE WINDOW5_PROTOCOL_MAX_DATA_LEN = 32U;
static const int WINDOW5_RSP_WAIT_LOOPS = 30;
static const int WINDOW5_VALVE_RSP_WAIT_LOOPS = 400;
static const size_t WINDOW5_QUEUE_MAX = 255U;
static const size_t WINDOW5_DEVICE_NAME_MAX = 32U;
static const BYTE WINDOW5_DISCOVERY_WINDOW_SIZE = 236U;
static const int WINDOW5_DISCOVERY_SLOT_MS = 60;
static const int WINDOW5_DISCOVERY_START_GUARD_MS = 200;
static const int WINDOW5_DISCOVERY_END_GUARD_MS = 500;
static const int WINDOW5_DISCOVERY_PASS_COUNT = 2;

struct SWindow5Rs485Result {
    int replyType;
    BYTE status;
    int returnedAddress;
    BYTE returnedDecoderType;
    bool hasReturnedDecoderType;
    BYTE returnedDeviceState;
    bool hasReturnedDeviceState;
    bool sendOk;
};

struct SWindow5Rs485Request {
    BYTE cmd;
    BYTE dataLen;
    BYTE data[WINDOW5_PROTOCOL_MAX_DATA_LEN];
    char frameName[16];
    bool trackDeviceState;
    int targetAddress;
    bool discoverDevices;
    bool valveCommand;
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
static bool sWindow5DiscoveryRunning = false;
static bool sWindow5DiscoveryCompleted = false;
static volatile bool sWindow5UrgentNoReplyPending = false;
static bool sWindow5ValveCommandBusy = false;
static int sWindow5ValveCommandPendingCount = 0;
static bool sWindow5ValveCommandHadFailure = false;
static BYTE sWindow5ValveCommandFinalState = 0xFFU;
static BYTE sWindow5ValveCommandTargetState = 0xFFU;
static long long sWindow5ValveSuccessTipHideDeadlineMs = 0;
static bool sWindow5TypePopupVisible = false;
static BYTE sWindow5SelectedDecoderLabelType = WINDOW5_DECODER_TYPE_VALUE;
static const char *sWindow5SelectedDecoderLabel = "电磁阀";

static SWindow5Rs485Result sendWindow5Rs485CommandDetailedSync(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName);
static void* window5Rs485Worker(void *arg);
static bool ensureWindow5Rs485Worker();
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
static void showWindow5ValveWaitTip();
static void addWindow5ValveCommandPending(BYTE targetState);
static void finishWindow5ValveCommandWait(const SWindow5Rs485Result &result);
static bool requestWindow5GroupDevicesState(int groupNo, bool open,
                                            bool includeValves,
                                            bool includePumps);
static bool sendWindow5ForceSetAddressCommand();
static bool checkWindow5AddressOccupied(int address, SWindow5Rs485Result *pResult);
static void hideWindow5TypePopupOnly();
static void updateWindow5DecoderTypeTitle();

static SWindow5Rs485Result makeWindow5Rs485Result() {
    SWindow5Rs485Result result;
    result.replyType = 0;
    result.status = 0xFFU;
    result.returnedAddress = -1;
    result.returnedDecoderType = 0xFFU;
    result.hasReturnedDecoderType = false;
    result.returnedDeviceState = 0xFFU;
    result.hasReturnedDeviceState = false;
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
    LOGD("[Window5Rs485] rx %u bytes: %s\n", len, buf);
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
                LOGD("[Window5Rs485] write retry timeout, written=%u/%u\n", writtenLen, len);
                return false;
            }
            usleep(10000);
            continue;
        }

        LOGD("[Window5Rs485] write failed errno=%d(%s), written=%u/%u\n",
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
                                  BYTE *pStatus,
                                  int *pReturnedAddress,
                                  BYTE *pReturnedDecoderType,
                                  bool *pHasReturnedDecoderType,
                                  BYTE *pReturnedDeviceState,
                                  bool *pHasReturnedDeviceState) {
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
        if (dataLen < 2U) {
            continue;
        }
        if (frameLen > (len - i)) {
            break;
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
                                 BYTE *pStatus,
                                 int *pReturnedAddress,
                                 BYTE *pReturnedDecoderType,
                                 bool *pHasReturnedDecoderType,
                                 BYTE *pReturnedDeviceState,
                                 bool *pHasReturnedDeviceState) {
    BYTE rxBuf[64] = {0};
    UINT rxLen = 0;
    const int waitLoops = (expectedCmd == WINDOW5_CMD_SET_VALVE_STATE) ?
        WINDOW5_VALVE_RSP_WAIT_LOOPS : WINDOW5_RSP_WAIT_LOOPS;

    for (int loop = 0; loop < waitLoops; ++loop) {
        if (sWindow5UrgentNoReplyPending) {
            LOGD("[Window5Rs485] abort reply wait for urgent no-reply command\n");
            return 0;
        }
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        const int pollRet = poll(&pfd, 1, 10);
        if (pollRet < 0) {
            if (errno == EINTR) {
                --loop;
                continue;
            }
            LOGD("[Window5Rs485] ack poll failed errno=%d(%s)\n", errno, strerror(errno));
            return 0;
        }

        if ((pollRet <= 0) || ((pfd.revents & POLLIN) == 0)) {
            continue;
        }

        while (rxLen < sizeof(rxBuf)) {
            const int ret = read(fd, rxBuf + rxLen, sizeof(rxBuf) - rxLen);
            if (ret > 0) {
                rxLen += (UINT)ret;
                dumpWindow5Hex(rxBuf, rxLen);
                continue;
            }

            if ((ret < 0) && ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))) {
                break;
            }

            break;
        }

        const int replyType = parseWindow5Rs485Reply(rxBuf, rxLen, expectedCmd,
                                                     pStatus, pReturnedAddress,
                                                     pReturnedDecoderType,
                                                     pHasReturnedDecoderType,
                                                     pReturnedDeviceState,
                                                     pHasReturnedDeviceState);
        if (replyType != 0) {
            return replyType;
        }
    }

    return 0;
}

static bool sendWindow5Rs485Device(const char *pFileName,
                                   const BYTE *pFrame, UINT frameLen,
                                   const char *pFrameName,
                                   bool updateKnownRoute,
                                   SWindow5Rs485Result *pResult) {
    LOGD("[Window5Rs485] try %s baud=2400, frame=%s\n", pFileName, pFrameName);

    const int fd = open(pFileName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        LOGD("[Window5Rs485] open %s failed errno=%d(%s)\n", pFileName, errno, strerror(errno));
        return false;
    }

    if (!configureWindow5Rs485Port(fd)) {
        LOGD("[Window5Rs485] config %s failed errno=%d(%s)\n", pFileName, errno, strerror(errno));
        close(fd);
        return false;
    }

    const bool sendOk = writeWindow5Rs485Frame(fd, pFrame, frameLen);
    if (pResult != NULL) {
        pResult->sendOk = sendOk;
    }
    if (!sendOk) {
        LOGD("[Window5Rs485] send %s %s FAIL\n", pFileName, pFrameName);
        close(fd);
        return false;
    }

    BYTE replyStatus = 0xFFU;
    int returnedAddress = -1;
    BYTE returnedDecoderType = 0xFFU;
    bool hasReturnedDecoderType = false;
    BYTE returnedDeviceState = 0xFFU;
    bool hasReturnedDeviceState = false;
    const int replyType = waitWindow5Rs485Reply(fd, pFrame[2], &replyStatus,
                                                &returnedAddress,
                                                &returnedDecoderType,
                                                &hasReturnedDecoderType,
                                                &returnedDeviceState,
                                                &hasReturnedDeviceState);
    if (pResult != NULL) {
        pResult->replyType = replyType;
        pResult->status = replyStatus;
        pResult->returnedAddress = returnedAddress;
        pResult->returnedDecoderType = returnedDecoderType;
        pResult->hasReturnedDecoderType = hasReturnedDecoderType;
        pResult->returnedDeviceState = returnedDeviceState;
        pResult->hasReturnedDeviceState = hasReturnedDeviceState;
    }
    if (replyType == 1) {
        if (hasReturnedDeviceState) {
            LOGD("[Window5Rs485] send %s %s OK, reply=ACK, status=%u, address=%u, decoderType=%u, state=%u\n",
                 pFileName, pFrameName, replyStatus, returnedAddress,
                 returnedDecoderType, returnedDeviceState);
        } else if (hasReturnedDecoderType) {
            LOGD("[Window5Rs485] send %s %s OK, reply=ACK, status=%u, address=%u, decoderType=%u\n",
                 pFileName, pFrameName, replyStatus, returnedAddress, returnedDecoderType);
        } else if (returnedAddress >= 0) {
            LOGD("[Window5Rs485] send %s %s OK, reply=ACK, status=%u, address=%u\n",
                 pFileName, pFrameName, replyStatus, returnedAddress);
        } else {
            LOGD("[Window5Rs485] send %s %s OK, reply=ACK, status=%u\n", pFileName, pFrameName, replyStatus);
        }
        if (updateKnownRoute) {
            setWindow5KnownDevice(pFileName);
        }
        close(fd);
        return true;
    }

    if (replyType == 2) {
        if (hasReturnedDeviceState) {
            LOGD("[Window5Rs485] send %s %s OK, reply=NACK, status=%u, address=%u, decoderType=%u, state=%u\n",
                 pFileName, pFrameName, replyStatus, returnedAddress,
                 returnedDecoderType, returnedDeviceState);
        } else if (hasReturnedDecoderType) {
            LOGD("[Window5Rs485] send %s %s OK, reply=NACK, status=%u, address=%u, decoderType=%u\n",
                 pFileName, pFrameName, replyStatus, returnedAddress, returnedDecoderType);
        } else if (returnedAddress >= 0) {
            LOGD("[Window5Rs485] send %s %s OK, reply=NACK, status=%u, address=%u\n",
                 pFileName, pFrameName, replyStatus, returnedAddress);
        } else {
            LOGD("[Window5Rs485] send %s %s OK, reply=NACK, status=%u\n", pFileName, pFrameName, replyStatus);
        }
        if (updateKnownRoute) {
            setWindow5KnownDevice(pFileName);
        }
        close(fd);
        return true;
    }

    LOGD("[Window5Rs485] send %s %s OK, reply timeout\n", pFileName, pFrameName);
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
        LOGD("[Window5Rs485] data too long, frame=%s len=%u\n", pFrameName, dataLen);
        return result;
    }
    if ((dataLen > 0U) && (pData == NULL)) {
        LOGD("[Window5Rs485] data null, frame=%s len=%u\n", pFrameName, dataLen);
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
    LOGD("[Window5Rs485] scan send start, candidate=%d, baud=2400, frame=%s cmd=0x%02X len=%u\n",
         total, pFrameName, cmd, dataLen);

    char knownDevice[WINDOW5_DEVICE_NAME_MAX] = {0};
    getWindow5KnownDevice(knownDevice, sizeof(knownDevice));
    if (knownDevice[0] != '\0') {
        LOGD("[Window5Rs485] try cached route -> %s\n", knownDevice);
        SWindow5Rs485Result routeResult = makeWindow5Rs485Result();
        if (sendWindow5Rs485Device(knownDevice, frame, frameLen, pFrameName, false, &routeResult)) {
            LOGD("[Window5Rs485] scan matched cached route=%s frame=%s\n", knownDevice, pFrameName);
            return routeResult;
        }
        if (routeResult.sendOk) {
            result = routeResult;
        }
        LOGD("[Window5Rs485] cached route failed, fall back to scan\n");
    }

    for (int i = 0; i < total; ++i) {
        if ((knownDevice[0] != '\0') &&
            (strcmp(knownDevice, kUartDeviceList[i]) == 0)) {
            continue;
        }
        LOGD("[Window5Rs485] scan send %d/%d -> %s\n", i + 1, total, kUartDeviceList[i]);
        SWindow5Rs485Result routeResult = makeWindow5Rs485Result();
        if (sendWindow5Rs485Device(kUartDeviceList[i], frame, frameLen, pFrameName, true, &routeResult)) {
            LOGD("[Window5Rs485] scan matched route=%s frame=%s\n", kUartDeviceList[i], pFrameName);
            return routeResult;
        }
        if (routeResult.sendOk) {
            result = routeResult;
        }
        usleep(20000);
    }

    LOGD("[Window5Rs485] scan send finish, frame=%s, matched=0, candidate=%d\n",
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
            LOGD("[Window5Rs485] discovery address conflict address=%u type=%u/%u\n",
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

static void parseWindow5DiscoveryFrames(BYTE *pBuffer,
                                        UINT *pBufferLen,
                                        BYTE windowStart,
                                        BYTE windowCount,
                                        BYTE token,
                                        std::vector<SWindow5DiscoveredDevice> &devices) {
    if ((pBuffer == NULL) || (pBufferLen == NULL)) {
        return;
    }

    UINT scan = 0U;
    while ((scan + 5U) <= *pBufferLen) {
        if ((pBuffer[scan] != 0x55U) || (pBuffer[scan + 1U] != 0xAAU)) {
            ++scan;
            continue;
        }

        const BYTE responseCmd = pBuffer[scan + 2U];
        const BYTE dataLen = pBuffer[scan + 3U];
        if (dataLen > WINDOW5_PROTOCOL_MAX_DATA_LEN) {
            ++scan;
            continue;
        }

        const UINT frameLen = 5U + dataLen;
        if ((scan + frameLen) > *pBufferLen) {
            break;
        }

        BYTE checksum = responseCmd + dataLen;
        for (UINT i = 0; i < dataLen; ++i) {
            checksum += pBuffer[scan + 4U + i];
        }
        if (checksum != pBuffer[scan + 4U + dataLen]) {
            LOGD("[Window5Rs485] discovery checksum error offset=%u\n", scan);
            ++scan;
            continue;
        }

        if ((responseCmd == WINDOW5_RSP_ACK) && (dataLen >= 7U) &&
            (pBuffer[scan + 4U] == WINDOW5_CMD_DISCOVER_DEVICES) &&
            (pBuffer[scan + 5U] == 0U) &&
            (pBuffer[scan + 10U] == token)) {
            const int address = getWindow5Address(&pBuffer[scan + 6U]);
            const BYTE decoderType = pBuffer[scan + 8U];
            const BYTE deviceState = pBuffer[scan + 9U];
            const int windowEnd = static_cast<int>(windowStart) + windowCount;
            const bool inWindow = (address >= windowStart) &&
                                  (static_cast<int>(address) < windowEnd);
            const bool validType = (decoderType == WINDOW5_DECODER_TYPE_VALUE) ||
                                   (decoderType == WINDOW5_DECODER_TYPE_SENSER);
            const bool validState = (deviceState <= 1U) || (deviceState == 0xFFU);
            if (inWindow && validType && validState) {
                addWindow5DiscoveredDevice(devices, address, decoderType, deviceState);
            }
        }
        scan += frameLen;
    }

    if (scan > 0U) {
        memmove(pBuffer, pBuffer + scan, *pBufferLen - scan);
        *pBufferLen -= scan;
    }
}

static bool sendWindow5DiscoveryWindow(int fd,
                                       BYTE windowStart,
                                       BYTE windowCount,
                                       BYTE token,
                                       std::vector<SWindow5DiscoveredDevice> &devices) {
    const BYTE data[3] = { windowStart, windowCount, token };
    BYTE frame[8] = {
        0x55U,
        0xAAU,
        WINDOW5_CMD_DISCOVER_DEVICES,
        sizeof(data),
        data[0],
        data[1],
        data[2],
        0U,
    };
    frame[7] = WINDOW5_CMD_DISCOVER_DEVICES + sizeof(data) +
               data[0] + data[1] + data[2];

    tcflush(fd, TCIFLUSH);
    if (!writeWindow5Rs485Frame(fd, frame, sizeof(frame))) {
        return false;
    }

    BYTE rxBuffer[256] = {0};
    UINT rxLen = 0U;
    const int waitMs = WINDOW5_DISCOVERY_START_GUARD_MS +
                       (windowCount * WINDOW5_DISCOVERY_SLOT_MS) +
                       WINDOW5_DISCOVERY_END_GUARD_MS;
    const int waitLoops = (waitMs + 9) / 10;

    for (int loop = 0; loop < waitLoops; ++loop) {
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        const int pollRet = poll(&pfd, 1, 10);
        if (pollRet < 0) {
            if (errno == EINTR) {
                --loop;
                continue;
            }
            return false;
        }
        if ((pollRet <= 0) || ((pfd.revents & POLLIN) == 0)) {
            continue;
        }

        while (rxLen < sizeof(rxBuffer)) {
            const int readLen = read(fd, rxBuffer + rxLen, sizeof(rxBuffer) - rxLen);
            if (readLen > 0) {
                rxLen += static_cast<UINT>(readLen);
                continue;
            }
            break;
        }
        parseWindow5DiscoveryFrames(rxBuffer, &rxLen, windowStart, windowCount,
                                    token, devices);
        if (rxLen == sizeof(rxBuffer)) {
            rxLen = 0U;
        }
    }

    parseWindow5DiscoveryFrames(rxBuffer, &rxLen, windowStart, windowCount,
                                token, devices);
    return true;
}

static void finishWindow5DeviceDiscovery(const std::vector<SWindow5DiscoveredDevice> &devices) {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    sWindow5DiscoveryResults = devices;
    sWindow5DiscoveryRunning = false;
    sWindow5DiscoveryCompleted = true;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
}

static void runWindow5DeviceDiscovery() {
    std::vector<SWindow5DiscoveredDevice> devices;
    const char* pDeviceName = "/dev/ttyS2";
    const int fd = open(pDeviceName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        finishWindow5DeviceDiscovery(devices);
        return;
    }
    if (!configureWindow5Rs485Port(fd)) {
        close(fd);
        finishWindow5DeviceDiscovery(devices);
        return;
    }

    int windowIndex = 0;
    for (int pass = 0;
         (pass < WINDOW5_DISCOVERY_PASS_COUNT) && !sWindow5UrgentNoReplyPending;
         ++pass) {
        for (int start = WINDOW5_CONFIG_ADDRESS_MIN;
             (start <= WINDOW5_CONFIG_ADDRESS_MAX) && !sWindow5UrgentNoReplyPending;
             start += WINDOW5_DISCOVERY_WINDOW_SIZE, ++windowIndex) {
            const int remaining = WINDOW5_CONFIG_ADDRESS_MAX - start + 1;
            const BYTE count = static_cast<BYTE>(
                (remaining < WINDOW5_DISCOVERY_WINDOW_SIZE) ?
                remaining : WINDOW5_DISCOVERY_WINDOW_SIZE);
            const BYTE token = static_cast<BYTE>(((pass + 1) * 67 + windowIndex) & 0xFF);
            LOGD("[Window5Rs485] discovery pass=%d start=%d count=%u token=%u\n",
                 pass + 1, start, count, token);
            (void)sendWindow5DiscoveryWindow(fd, static_cast<BYTE>(start), count,
                                             token, devices);
        }
    }

    close(fd);
    if (!devices.empty()) {
        setWindow5KnownDevice(pDeviceName);
    }

    for (size_t i = 0; i < devices.size(); ++i) {
        for (size_t j = i + 1U; j < devices.size(); ++j) {
            if (devices[j].address < devices[i].address) {
                const SWindow5DiscoveredDevice temp = devices[i];
                devices[i] = devices[j];
                devices[j] = temp;
            }
        }
    }
    finishWindow5DeviceDiscovery(devices);
}

static void* window5Rs485Worker(void *arg) {
    (void)arg;

    while (true) {
        SWindow5Rs485Request req;

        pthread_mutex_lock(&sWindow5QueueMutex);
        while (sWindow5RequestQueue.empty()) {
            pthread_cond_wait(&sWindow5QueueCond, &sWindow5QueueMutex);
        }
        req = sWindow5RequestQueue.front();
        sWindow5RequestQueue.pop_front();
        pthread_mutex_unlock(&sWindow5QueueMutex);

        LOGD("[Window5Rs485] worker start frame=%s cmd=0x%02X\n", req.frameName, req.cmd);
        const bool interactiveCommand = (req.cmd == WINDOW5_CMD_SET_VALVE_STATE) ||
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
            LOGD("[Window5Rs485] create worker failed ret=%d\n", ret);
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
                                               bool discoverDevices) {
    if (!ensureWindow5Rs485Worker()) {
        return false;
    }
    if (dataLen > WINDOW5_PROTOCOL_MAX_DATA_LEN) {
        LOGD("[Window5Rs485] queue data too long, frame=%s len=%u\n", pFrameName, dataLen);
        return false;
    }
    if ((dataLen > 0U) && (pData == NULL)) {
        LOGD("[Window5Rs485] queue data null, frame=%s len=%u\n", pFrameName, dataLen);
        return false;
    }

    pthread_mutex_lock(&sWindow5QueueMutex);
    if (cmd == WINDOW5_CMD_GET_DEVICE_STATE) {
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
        LOGD("[Window5Rs485] queue full, drop frame=%s cmd=0x%02X\n", pFrameName, cmd);
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
    const bool interactiveCommand = (cmd == WINDOW5_CMD_SET_VALVE_STATE) ||
                                    discoverDevices;
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

    LOGD("[Window5Rs485] queue push frame=%s cmd=0x%02X\n", pFrameName, cmd);
    if (req.valveCommand) {
        addWindow5ValveCommandPending((req.dataLen >= 4U) ? req.data[3] : 0xFFU);
    }
    return true;
}

static bool enqueueWindow5Rs485Command(BYTE cmd,
                                       const BYTE *pData,
                                       BYTE dataLen,
                                       const char *pFrameName) {
    return enqueueWindow5Rs485CommandInternal(cmd, pData, dataLen, pFrameName,
                                              false, 0U, false);
}

static bool enqueueWindow5TrackedRs485Command(BYTE cmd,
                                              const BYTE *pData,
                                              BYTE dataLen,
                                              const char *pFrameName,
                                              int targetAddress) {
    return enqueueWindow5Rs485CommandInternal(cmd, pData, dataLen, pFrameName,
                                              true, targetAddress, false);
}

bool isWindow5DeviceDiscoveryRunning() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const bool running = sWindow5DiscoveryRunning;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return running;
}

bool requestWindow5DeviceDiscovery() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    if (sWindow5DiscoveryRunning || sWindow5DiscoveryCompleted) {
        pthread_mutex_unlock(&sWindow5StateUpdateMutex);
        return false;
    }
    sWindow5DiscoveryRunning = true;
    sWindow5DiscoveryCompleted = false;
    sWindow5DiscoveryResults.clear();
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);

    const bool queued = enqueueWindow5Rs485CommandInternal(
        WINDOW5_CMD_DISCOVER_DEVICES, NULL, 0U, "DISCOVER",
        false, 0U, true);
    if (!queued) {
        pthread_mutex_lock(&sWindow5StateUpdateMutex);
        sWindow5DiscoveryRunning = false;
        pthread_mutex_unlock(&sWindow5StateUpdateMutex);
        return false;
    }

    refreshDeviceListViews();
    return true;
}

static bool takeWindow5DiscoveryResults(std::vector<SWindow5DiscoveredDevice> *pDevices) {
    if (pDevices == NULL) {
        return false;
    }

    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    if (!sWindow5DiscoveryCompleted) {
        pthread_mutex_unlock(&sWindow5StateUpdateMutex);
        return false;
    }
    *pDevices = sWindow5DiscoveryResults;
    sWindow5DiscoveryResults.clear();
    sWindow5DiscoveryCompleted = false;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return true;
}

static bool applyWindow5DiscoveryResults() {
    std::vector<SWindow5DiscoveredDevice> devices;
    if (!takeWindow5DiscoveryResults(&devices)) {
        return false;
    }

    int conflictCount = 0;
    int addedValveCount = 0;
    int addedSensorCount = 0;
    for (size_t i = 0; i < devices.size(); ++i) {
        const SWindow5DiscoveredDevice &device = devices[i];
        if (device.addressConflict) {
            ++conflictCount;
            continue;
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
    }
    LOGD("[Window5Rs485] discovery complete devices=%u conflicts=%d\n",
         static_cast<UINT>(devices.size()), conflictCount);
    char tipText[128] = {0};
    snprintf(tipText, sizeof(tipText),
             "此次共添加\n电磁阀 数量%d\n传感器 数量%d",
             addedValveCount, addedSensorCount);
    (void)showW2TipText(tipText);
    return true;
}

static bool sendWindow5ValveOnCommand() {
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
    BYTE requestData[4] = {0};
    putWindow5Address(requestData, address);
    requestData[2] = decoderType;
    requestData[3] = 1U;
    return enqueueWindow5TrackedRs485Command(WINDOW5_CMD_SET_VALVE_STATE,
                                             requestData, sizeof(requestData),
                                             "VALVE_ON", address);
}

static bool sendWindow5ValveOffCommand() {
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
    BYTE requestData[4] = {0};
    putWindow5Address(requestData, address);
    requestData[2] = decoderType;
    requestData[3] = 0U;
    return enqueueWindow5TrackedRs485Command(WINDOW5_CMD_SET_VALVE_STATE,
                                             requestData, sizeof(requestData),
                                             "VALVE_OFF", address);
}

static bool isWindow5ManagedDecoderDevice(const SDATA *data) {
    return data && ((strcmp(data->type, "电磁阀") == 0) ||
                    (strcmp(data->type, "传感器") == 0));
}

bool requestWindow5DeviceState(int deviceIndex) {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return false;
    }

    const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
    if (!isWindow5ManagedDecoderDevice(data) ||
        (data->address < WINDOW5_DEVICE_ADDRESS_MIN) ||
        (data->address > WINDOW5_DEVICE_ADDRESS_MAX)) {
        return false;
    }

    BYTE requestData[2] = {0};
    putWindow5Address(requestData, data->address);
    return enqueueWindow5TrackedRs485Command(WINDOW5_CMD_GET_DEVICE_STATE,
                                             requestData, sizeof(requestData),
                                             "GET_STATE", data->address);
}

static bool requestWindow5ValveStateInternal(int deviceIndex, bool open) {
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
    return enqueueWindow5TrackedRs485Command(WINDOW5_CMD_SET_VALVE_STATE,
                                             requestData, sizeof(requestData),
                                             open ? "VALVE_ON" : "VALVE_OFF",
                                             data->address);
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
        const bool valve = std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0;
        const bool pump = std::strcmp(data->type, "水泵") == 0;
        if ((!includeValves || !valve) && (!includePumps || !pump)) {
            continue;
        }
        requested = requestWindow5ValveStateInternal(i, open) || requested;
    }
    return requested;
}

static bool requestWindow5GroupPumpState(int groupNo, bool open) {
    return requestWindow5GroupDevicesState(groupNo, open, false, true);
}

static bool requestWindow5GroupIrrigationState(int groupNo, bool open) {
    return requestWindow5GroupDevicesState(groupNo, open, true, true);
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
        const bool identified = (result.replyType != 0) &&
                                (result.returnedAddress == update.targetAddress) &&
                                result.hasReturnedDecoderType &&
                                result.hasReturnedDeviceState &&
                                validType &&
                                ((result.returnedDeviceState <= 1U) ||
                                 (result.returnedDeviceState == 0xFFU));
        deviceStateApplied = DeviceDataStore::updateRuntimeStateByAddress(
                                 update.targetAddress,
                                 identified,
                                 identified ? result.returnedDecoderType : DEVICE_DECODER_TYPE_UNKNOWN,
                                 identified && (result.returnedDeviceState <= 1U),
                                 identified && (result.returnedDeviceState != 0U)) ||
                             deviceStateApplied;
        if (update.valveCommand) {
            finishWindow5ValveCommandWait(result);
        }
    }

    if (discoveryApplied || deviceStateApplied) {
        refreshDeviceListViews();
        refreshWindow4ListViews();
        refreshWindow8IrrigationState();
        refreshRunStatusValueText();
        if (discoveryApplied) {
            showDeviceListEmptyRow();
        }
    }
}

static void requestWindow5NextDeviceState() {
    if (isWindow5DeviceDiscoveryRunning()) {
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

    for (int checked = 0; checked < total; ++checked) {
        const int index = sWindow5NextDevicePollIndex;
        sWindow5NextDevicePollIndex = (sWindow5NextDevicePollIndex + 1) % total;
        const SDATA* data = DeviceDataStore::getDevice(index);
        if (isWindow5ManagedDecoderDevice(data)) {
            (void)requestWindow5DeviceState(index);
            return;
        }
    }
}

void updateWindow5DeviceStatePolling() {
    applyWindow5DeviceStateUpdates();
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

    LOGD("[Window5Rs485] address tip: %s\n", pText ? pText : "");
    if (mTestAdressTipsTextPtr) {
        mTestAdressTipsTextPtr->setTextColor(textColor);
        mTestAdressTipsTextPtr->setText(pText ? pText : "");
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

static void showWindow5ValveWaitTip() {
    if (sWindow5ValveCommandTargetState == 1U) {
        setWindow5TestAddressTipWithColor("正在开阀\n请等待",
                                          WINDOW5_CONFIG_TIP_COLOR_WAIT);
    } else if (sWindow5ValveCommandTargetState == 0U) {
        setWindow5TestAddressTipWithColor("正在关阀\n请等待",
                                          WINDOW5_CONFIG_TIP_COLOR_WAIT);
    } else {
        setWindow5TestAddressTipWithColor("正在执行阀门动作\n请等待",
                                          WINDOW5_CONFIG_TIP_COLOR_WAIT);
    }
}

static void addWindow5ValveCommandPending(BYTE targetState) {
    if (sWindow5ValveCommandPendingCount <= 0) {
        sWindow5ValveCommandPendingCount = 0;
        sWindow5ValveCommandHadFailure = false;
        sWindow5ValveCommandFinalState = 0xFFU;
        sWindow5ValveCommandTargetState = targetState;
    } else if (sWindow5ValveCommandTargetState != targetState) {
        sWindow5ValveCommandTargetState = 0xFFU;
    }
    ++sWindow5ValveCommandPendingCount;
    sWindow5ValveCommandBusy = true;
    showWindow5ValveWaitTip();
}

static bool isWindow5ValveResultOk(const SWindow5Rs485Result &result) {
    return (result.replyType == 1) &&
           (result.status == 0U) &&
           result.hasReturnedDeviceState &&
           (result.returnedDeviceState <= 1U);
}

static void finishWindow5ValveCommandWait(const SWindow5Rs485Result &result) {
    if (isWindow5ValveResultOk(result)) {
        sWindow5ValveCommandFinalState = result.returnedDeviceState;
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
    sWindow5ValveCommandTargetState = 0xFFU;
    if (sWindow5ValveCommandHadFailure) {
        sWindow5ValveCommandHadFailure = false;
        setWindow5TestAddressFailureTip("已超时");
        return;
    }
    if (sWindow5ValveCommandFinalState == 1U) {
        setWindow5TestAddressSuccessTip("已开阀");
        scheduleWindow5ValveSuccessTipAutoHide();
    } else if (sWindow5ValveCommandFinalState == 0U) {
        setWindow5TestAddressSuccessTip("已关阀");
        scheduleWindow5ValveSuccessTipAutoHide();
    } else {
        setWindow5TestAddressSuccessTip("阀门动作完成");
        scheduleWindow5ValveSuccessTipAutoHide();
    }
}

static bool hideWindow5TestAddressTipIfVisible() {
    if (isWindow5ValveCommandBusy()) {
        showWindow5ValveWaitTip();
        return true;
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
    LOGD("[Window5Rs485] decoder type not initialized, default to value, checkedID=%d\n",
         checkedID);
    *pDecoderType = WINDOW5_DECODER_TYPE_VALUE;
    return true;
}

static const char* getWindow5DecoderTypeText(BYTE decoderType) {
    switch (decoderType) {
    case WINDOW5_DECODER_TYPE_VALUE:
        return "电磁阀";
    case WINDOW5_DECODER_TYPE_SENSER:
        return "传感器";
    default:
        return "未知";
    }
}

static const char* getWindow5DecoderDisplayText(BYTE decoderType) {
    if ((sWindow5SelectedDecoderLabel != NULL) &&
        (sWindow5SelectedDecoderLabel[0] != '\0') &&
        (sWindow5SelectedDecoderLabelType == decoderType)) {
        return sWindow5SelectedDecoderLabel;
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
        mWindow5TypePopupTitleTextPtr->setText(sensorMode ? "选择传感器类型" : "选择电磁阀类型");
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
    snprintf(title, sizeof(title), "解码器类型：%s", pText);
    mButton40Ptr->setText(title);
}

static void setWindow5ConfigTip(int address, BYTE decoderType, const char *pStatusText) {
    char tip[160] = {0};
    int textColor = WINDOW5_CONFIG_TIP_COLOR_NEUTRAL;
    if ((pStatusText != NULL) && (strcmp(pStatusText, "成功") == 0)) {
        textColor = WINDOW5_CONFIG_TIP_COLOR_SUCCESS;
    } else if ((pStatusText != NULL) &&
               (strncmp(pStatusText, "失败", strlen("失败")) == 0)) {
        textColor = WINDOW5_CONFIG_TIP_COLOR_FAILURE;
    }
    snprintf(tip, sizeof(tip), "地址：%d\n类型：%s\n%s",
             address, getWindow5DecoderDisplayText(decoderType),
             pStatusText ? pStatusText : "");
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
        setWindow5TestAddressFailureTip("请输入阀地址\n范围20-255");
        return false;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    if (!normalizeWindow5AddressText(text, &normalizedValue)) {
        setWindow5TestAddressFailureTip("阀地址格式错误\n请输入20-255");
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
        setWindow5TestAddressFailureTip("请输入地址\n范围20-255");
        return false;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    if (!normalizeWindow5AddressText(text, &normalizedValue)) {
        setWindow5TestAddressFailureTip("地址格式错误\n请输入20-255");
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
        setWindow5TestAddressFailureTip("请输入源地址\n范围20-255或8888");
        return false;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    if (!normalizeWindow5SourceAddressText(text, &normalizedValue)) {
        setWindow5TestAddressFailureTip("源地址格式错误\n请输入20-255或8888");
        return false;
    }

    setWindow5NormalizedSourceAddressText(normalizedValue);
    *pAddress = static_cast<int>(normalizedValue);
    return true;
}

static const char* getWindow5AddressStatusText(BYTE status) {
    switch (status) {
    case 0U:
        return "成功";
    case 1U:
        return "数据长度错误";
    case 2U:
        return "地址越界";
    case 3U:
        return "从机固件不支持类型配置";
    case 4U:
        return "EEPROM保存失败";
    case 5U:
        return "解码器类型错误";
    default:
        return "未知错误";
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
    snprintf(statusText, sizeof(statusText), "失败：%s", pReason ? pReason : "未知错误");
    setWindow5ConfigTip(displayAddress, displayDecoderType, statusText);
}

static void setWindow5AddressNoReplyTip(int address,
                                        BYTE decoderType,
                                        const SWindow5Rs485Result &result) {
    setWindow5ConfigFailureTip(address, decoderType, result,
                              result.sendOk ? "未收到应答" : "发送失败");
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
    setWindow5TestAddressTip("");
    if (sWindow5AddressTextUpdating) {
        return;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    (void)normalizeWindow5AddressText(text, &normalizedValue);
    setWindow5NormalizedAddressText(normalizedValue);
}

static void handleWindow5SourceAddressTextChanged(const std::string &text) {
    setWindow5TestAddressTip("");
    if (sWindow5SourceAddressTextUpdating) {
        return;
    }

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

    LOGD("[Window5Rs485] set config request address=%d decoderType=%u\n", address, decoderType);
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
        setWindow5TestAddressTipWithColor("源地址和目标地址相同\n请更换目标地址",
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
        snprintf(tip, sizeof(tip),
                 "目标地址%d已有设备\n类型：%s\n请更换目标地址",
                 destAddress, getWindow5DecoderDisplayText(displayType));
        setWindow5TestAddressTipWithColor(tip, WINDOW5_CONFIG_TIP_COLOR_FAILURE);
        LOGD("[Window5Rs485] refuse set address source=%d dest=%d, target occupied, replyType=%d status=%u type=%u hasType=%d\n",
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
            snprintf(tip, sizeof(tip),
                     "强制修改回包地址不匹配\n返回地址%d，请用新地址核对",
                     result.returnedAddress);
        } else {
            snprintf(tip, sizeof(tip),
                     "强制修改失败：%s", getWindow5AddressStatusText(result.status));
        }
        setWindow5TestAddressTipWithColor(tip, WINDOW5_CONFIG_TIP_COLOR_FAILURE);
        return false;
    }

    if (result.replyType == 2) {
        char tip[128] = {0};
        snprintf(tip, sizeof(tip),
                 "强制修改被从机拒绝\n原因：%s", getWindow5AddressStatusText(result.status));
        setWindow5TestAddressTipWithColor(tip, WINDOW5_CONFIG_TIP_COLOR_FAILURE);
        return false;
    }

    if (result.sendOk) {
        setWindow5TestAddressFailureTip("强制命令已发送\n未收到确认，请用新地址核对");
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

    LOGD("[Window5Rs485] check config request address=%d decoderType=%u\n",
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
            snprintf(pMessage, messageSize, "地址范围20-255");
        }
        return false;
    }
    if (isWindow5ValveCommandBusy()) {
        if (pMessage && messageSize > 0U) {
            snprintf(pMessage, messageSize, "请等待当前指令完成");
        }
        return false;
    }

    const BYTE expectedDecoderType = sensor ?
            WINDOW5_DECODER_TYPE_SENSER : WINDOW5_DECODER_TYPE_VALUE;
    BYTE requestData[2] = {0};
    putWindow5Address(requestData, address);
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
        WINDOW5_CMD_GET_CONFIG, requestData, sizeof(requestData), "W2_ADD_GET_CONFIG");

    if (result.replyType == 0) {
        if (pMessage && messageSize > 0U) {
            snprintf(pMessage, messageSize, result.sendOk ? "地址无应答" : "测试指令发送失败");
        }
        return false;
    }

    if ((result.replyType == 1) && (result.status == 0U)) {
        if (!result.hasReturnedDecoderType) {
            if (pMessage && messageSize > 0U) {
                snprintf(pMessage, messageSize, "固件未返回设备类型");
            }
            return false;
        }
        if (result.returnedAddress != address) {
            if (pMessage && messageSize > 0U) {
                snprintf(pMessage, messageSize, "返回地址%d不匹配", result.returnedAddress);
            }
            return false;
        }
        if (result.returnedDecoderType != expectedDecoderType) {
            if (pMessage && messageSize > 0U) {
                snprintf(pMessage, messageSize, "设备类型不匹配\n地址%d为%s",
                         address, getWindow5DecoderTypeText(result.returnedDecoderType));
            }
            return false;
        }
        if (pMessage && messageSize > 0U) {
            snprintf(pMessage, messageSize, "测试通过");
        }
        return true;
    }

    if (pMessage && messageSize > 0U) {
        snprintf(pMessage, messageSize, "测试失败：%s",
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
}

static void onPage5Hide() {
    setWindow5TestAddressTip("");
    hideWindow5TypePopupOnly();
}
