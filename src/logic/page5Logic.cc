// Page5 logic.
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <deque>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>
#include <vector>

static const BYTE WINDOW5_RSP_ACK = 0x80U;
static const BYTE WINDOW5_RSP_NACK = 0x7FU;
static const BYTE WINDOW5_CMD_SET_CONFIG = 0x42U;
static const BYTE WINDOW5_CMD_GET_CONFIG = 0x43U;
static const BYTE WINDOW5_CMD_GET_DEVICE_STATE = 0x44U;
static const BYTE WINDOW5_CMD_SET_VALVE_STATE = 0x45U;
static const BYTE WINDOW5_CMD_DISCOVER_DEVICES = 0x46U;
static const BYTE WINDOW5_DECODER_TYPE_VALUE = 1U;
static const BYTE WINDOW5_DECODER_TYPE_SENSER = 2U;
static const BYTE WINDOW5_DEVICE_ADDRESS_MIN = 1U;
static const BYTE WINDOW5_DEVICE_ADDRESS_MAX = 255U;
static const BYTE WINDOW5_CONFIG_ADDRESS_MIN = 20U;
static const BYTE WINDOW5_CONFIG_ADDRESS_MAX = 255U;
static const BYTE WINDOW5_PROTOCOL_MAX_DATA_LEN = 32U;
static const int WINDOW5_RSP_WAIT_LOOPS = 40;
static const size_t WINDOW5_QUEUE_MAX = 255U;
static const size_t WINDOW5_DEVICE_NAME_MAX = 32U;
static const BYTE WINDOW5_DISCOVERY_WINDOW_SIZE = 8U;
static const int WINDOW5_DISCOVERY_SLOT_MS = 80;
static const int WINDOW5_DISCOVERY_START_GUARD_MS = 100;
static const int WINDOW5_DISCOVERY_END_GUARD_MS = 250;
static const int WINDOW5_DISCOVERY_PASS_COUNT = 2;

struct SWindow5Rs485Result {
    int replyType;
    BYTE status;
    BYTE returnedAddress;
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
    BYTE targetAddress;
    bool discoverDevices;
};

struct SWindow5DeviceStateUpdate {
    BYTE targetAddress;
    SWindow5Rs485Result result;
};

struct SWindow5DiscoveredDevice {
    BYTE address;
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

static SWindow5Rs485Result sendWindow5Rs485CommandDetailedSync(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName);
static void* window5Rs485Worker(void *arg);
static bool ensureWindow5Rs485Worker();
static bool enqueueWindow5Rs485Command(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName);
static void setWindow5KnownDevice(const char *pFileName);
static void getWindow5KnownDevice(char *pOut, size_t outSize);

static SWindow5Rs485Result makeWindow5Rs485Result() {
    SWindow5Rs485Result result;
    result.replyType = 0;
    result.status = 0xFFU;
    result.returnedAddress = 0xFFU;
    result.returnedDecoderType = 0xFFU;
    result.hasReturnedDecoderType = false;
    result.returnedDeviceState = 0xFFU;
    result.hasReturnedDeviceState = false;
    result.sendOk = false;
    return result;
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
                                  BYTE *pReturnedAddress,
                                  BYTE *pReturnedDecoderType,
                                  bool *pHasReturnedDecoderType,
                                  BYTE *pReturnedDeviceState,
                                  bool *pHasReturnedDeviceState) {
    if (pStatus != NULL) {
        *pStatus = 0xFFU;
    }
    if (pReturnedAddress != NULL) {
        *pReturnedAddress = 0xFFU;
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
        if ((pReturnedAddress != NULL) && (dataLen >= 3U)) {
            *pReturnedAddress = pData[i + 6U];
        }
        if (dataLen >= 4U) {
            if (pReturnedDecoderType != NULL) {
                *pReturnedDecoderType = pData[i + 7U];
            }
            if (pHasReturnedDecoderType != NULL) {
                *pHasReturnedDecoderType = true;
            }
        }
        if (dataLen >= 5U) {
            if (pReturnedDeviceState != NULL) {
                *pReturnedDeviceState = pData[i + 8U];
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
                                 BYTE *pReturnedAddress,
                                 BYTE *pReturnedDecoderType,
                                 bool *pHasReturnedDecoderType,
                                 BYTE *pReturnedDeviceState,
                                 bool *pHasReturnedDeviceState) {
    BYTE rxBuf[64] = {0};
    UINT rxLen = 0;

    for (int loop = 0; loop < WINDOW5_RSP_WAIT_LOOPS; ++loop) {
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
    BYTE returnedAddress = 0xFFU;
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
        } else if (returnedAddress != 0xFFU) {
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
        } else if (returnedAddress != 0xFFU) {
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

static void pushWindow5DeviceStateUpdate(BYTE targetAddress,
                                         const SWindow5Rs485Result &result) {
    SWindow5DeviceStateUpdate update;
    update.targetAddress = targetAddress;
    update.result = result;

    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    if (sWindow5StateUpdateQueue.size() >= WINDOW5_QUEUE_MAX) {
        sWindow5StateUpdateQueue.pop_front();
    }
    sWindow5StateUpdateQueue.push_back(update);
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
}

static void addWindow5DiscoveredDevice(std::vector<SWindow5DiscoveredDevice> &devices,
                                       BYTE address,
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

        if ((responseCmd == WINDOW5_RSP_ACK) && (dataLen >= 6U) &&
            (pBuffer[scan + 4U] == WINDOW5_CMD_DISCOVER_DEVICES) &&
            (pBuffer[scan + 5U] == 0U) &&
            (pBuffer[scan + 9U] == token)) {
            const BYTE address = pBuffer[scan + 6U];
            const BYTE decoderType = pBuffer[scan + 7U];
            const BYTE deviceState = pBuffer[scan + 8U];
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
    for (int pass = 0; pass < WINDOW5_DISCOVERY_PASS_COUNT; ++pass) {
        for (int start = WINDOW5_CONFIG_ADDRESS_MIN;
             start <= WINDOW5_CONFIG_ADDRESS_MAX;
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
        if (req.discoverDevices) {
            pthread_mutex_lock(&sWindow5IoMutex);
            runWindow5DeviceDiscovery();
            pthread_mutex_unlock(&sWindow5IoMutex);
            continue;
        }
        const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
            req.cmd, req.data, req.dataLen, req.frameName);
        if (req.trackDeviceState) {
            pushWindow5DeviceStateUpdate(req.targetAddress, result);
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
                                               BYTE targetAddress,
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
    sWindow5RequestQueue.push_back(req);
    pthread_cond_signal(&sWindow5QueueCond);
    pthread_mutex_unlock(&sWindow5QueueMutex);

    LOGD("[Window5Rs485] queue push frame=%s cmd=0x%02X\n", pFrameName, cmd);
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
                                              BYTE targetAddress) {
    return enqueueWindow5Rs485CommandInternal(cmd, pData, dataLen, pFrameName,
                                              true, targetAddress, false);
}

static bool isWindow5DeviceDiscoveryRunning() {
    pthread_mutex_lock(&sWindow5StateUpdateMutex);
    const bool running = sWindow5DiscoveryRunning;
    pthread_mutex_unlock(&sWindow5StateUpdateMutex);
    return running;
}

static bool requestWindow5DeviceDiscovery() {
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

static bool sendWindow5Rs485Command(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName) {
    return enqueueWindow5Rs485Command(cmd, pData, dataLen, pFrameName);
}

static bool sendWindow5LedOnCommand() {
    return sendWindow5Rs485Command(0x10, NULL, 0, "LED_ON");
}

static bool sendWindow5LedOffCommand() {
    return sendWindow5Rs485Command(0x12, NULL, 0, "LED_OFF");
}

static bool sendWindow5Led2OnCommand() {
    return sendWindow5Rs485Command(0x11, NULL, 0, "LED2_ON");
}

static bool sendWindow5Led2OffCommand() {
    return sendWindow5Rs485Command(0x12, NULL, 0, "LED2_OFF");
}

static bool sendWindow5ValveOnCommand() {
    return sendWindow5Rs485Command(0x20, NULL, 0, "VALVE_ON");
}

static bool sendWindow5ValveOffCommand() {
    return sendWindow5Rs485Command(0x21, NULL, 0, "VALVE_OFF");
}

static bool isWindow5ManagedDecoderDevice(const SDATA *data) {
    return data && ((strcmp(data->type, "电磁阀") == 0) ||
                    (strcmp(data->type, "传感器") == 0));
}

static bool requestWindow5DeviceState(int deviceIndex) {
    const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
    if (!isWindow5ManagedDecoderDevice(data) ||
        (data->address < WINDOW5_DEVICE_ADDRESS_MIN) ||
        (data->address > WINDOW5_DEVICE_ADDRESS_MAX)) {
        return false;
    }

    const BYTE requestData[1] = { static_cast<BYTE>(data->address) };
    return enqueueWindow5TrackedRs485Command(WINDOW5_CMD_GET_DEVICE_STATE,
                                             requestData, sizeof(requestData),
                                             "GET_STATE", requestData[0]);
}

static bool requestWindow5ValveState(int deviceIndex, bool open) {
    const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
    if (!data || (strcmp(data->type, "电磁阀") != 0) ||
        (data->address < WINDOW5_DEVICE_ADDRESS_MIN) ||
        (data->address > WINDOW5_DEVICE_ADDRESS_MAX)) {
        return false;
    }

    const BYTE requestData[2] = {
        static_cast<BYTE>(data->address),
        static_cast<BYTE>(open ? 1U : 0U),
    };
    return enqueueWindow5TrackedRs485Command(WINDOW5_CMD_SET_VALVE_STATE,
                                             requestData, sizeof(requestData),
                                             open ? "VALVE_ON" : "VALVE_OFF",
                                             requestData[0]);
}

static bool requestWindow5GroupValveState(int groupNo, bool open) {
    bool requested = false;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || (strcmp(data->type, "电磁阀") != 0) ||
            (atoi(data->arre) != groupNo)) {
            continue;
        }
        requested = requestWindow5ValveState(i, open) || requested;
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
    bool changed = false;
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
        changed = DeviceDataStore::updateRuntimeStateByAddress(
                      update.targetAddress,
                      identified,
                      identified ? result.returnedDecoderType : DEVICE_DECODER_TYPE_UNKNOWN,
                      identified && (result.returnedDeviceState <= 1U),
                      identified && (result.returnedDeviceState != 0U)) || changed;
    }

    if (changed || discoveryApplied) {
        refreshDeviceListViews();
        refreshWindow4ListViews();
    }
}

static void requestWindow5NextDeviceState() {
    if (isWindow5DeviceDiscoveryRunning()) {
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

static void updateWindow5DeviceStatePolling() {
    applyWindow5DeviceStateUpdates();
    requestWindow5NextDeviceState();
}

static bool sWindow5TestAddressTipVisible = false;
static bool sWindow5AddressTextUpdating = false;
static const int WINDOW5_CONFIG_TIP_COLOR_NEUTRAL = static_cast<int>(0xFF1D1D1FU);
static const int WINDOW5_CONFIG_TIP_COLOR_SUCCESS = static_cast<int>(0xFF248A3DU);
static const int WINDOW5_CONFIG_TIP_COLOR_FAILURE = static_cast<int>(0xFFD92D20U);

static void setWindow5TestAddressTipWithColor(const char *pText, int textColor) {
    const bool visible = (pText != NULL) && (pText[0] != '\0');

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

static bool hideWindow5TestAddressTipIfVisible() {
    if (!sWindow5TestAddressTipVisible) {
        return false;
    }

    setWindow5TestAddressTip("");
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

    setWindow5TestAddressTip("未选择解码器类型");
    return false;
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
             address, getWindow5DecoderTypeText(decoderType),
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

static bool parseWindow5TestAddressEditText(int *pAddress) {
    if (pAddress == NULL) {
        return false;
    }

    if (!mTestAdressEditTextPtr) {
        setWindow5TestAddressTip("地址输入框无效");
        return false;
    }

    const std::string text = mTestAdressEditTextPtr->getText();
    const char *pStart = text.c_str();
    while (isWindow5AsciiSpace(*pStart)) {
        ++pStart;
    }

    if (*pStart == '\0') {
        setWindow5TestAddressTip("请输入地址\n范围20-255");
        return false;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    if (!normalizeWindow5AddressText(text, &normalizedValue)) {
        setWindow5TestAddressTip("地址格式错误\n请输入20-255");
        return false;
    }

    setWindow5NormalizedAddressText(normalizedValue);
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
    const int displayAddress = (result.returnedAddress != 0xFFU) ?
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

static void handleWindow5TestAddressTextChanged(const std::string &text) {
    setWindow5TestAddressTip("");
    if (sWindow5AddressTextUpdating) {
        return;
    }

    long normalizedValue = WINDOW5_CONFIG_ADDRESS_MIN;
    (void)normalizeWindow5AddressText(text, &normalizedValue);
    setWindow5NormalizedAddressText(normalizedValue);
}

static bool sendWindow5SetAddressCommand() {
    BYTE decoderType = 0U;
    if (!getWindow5SelectedDecoderType(&decoderType)) {
        return false;
    }

    int address = 0;
    if (!parseWindow5TestAddressEditText(&address)) {
        return false;
    }

    const BYTE data[2] = { static_cast<BYTE>(address), decoderType };

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

        if ((result.returnedAddress == static_cast<BYTE>(address)) &&
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

static bool sendWindow5CheckAddressCommand() {
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
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(
        WINDOW5_CMD_GET_CONFIG, NULL, 0, "GET_CONFIG");

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

        if ((result.returnedAddress == static_cast<BYTE>(expectedAddress)) &&
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

static void onPage5Show() {
    setWindow5TestAddressTip("");
}

static void onPage5Hide() {
    setWindow5TestAddressTip("");
}
