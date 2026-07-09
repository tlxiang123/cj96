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

static const BYTE WINDOW5_RSP_ACK = 0x80U;
static const BYTE WINDOW5_RSP_NACK = 0x7FU;
static const BYTE WINDOW5_CMD_SET_ADDRESS = 0x40U;
static const BYTE WINDOW5_CMD_GET_ADDRESS = 0x41U;
static const BYTE WINDOW5_TEST_ADDRESS_MIN = 1U;
static const BYTE WINDOW5_TEST_ADDRESS_MAX = 200U;
static const BYTE WINDOW5_PROTOCOL_MAX_DATA_LEN = 32U;
static const int WINDOW5_RSP_WAIT_LOOPS = 20;
static const size_t WINDOW5_QUEUE_MAX = 16U;
static const size_t WINDOW5_DEVICE_NAME_MAX = 32U;

struct SWindow5Rs485Request {
    BYTE cmd;
    BYTE dataLen;
    BYTE data[WINDOW5_PROTOCOL_MAX_DATA_LEN];
    char frameName[16];
};

struct SWindow5Rs485Result {
    int replyType;
    BYTE status;
    BYTE returnedAddress;
    bool sendOk;
};

static pthread_mutex_t sWindow5QueueMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sWindow5QueueCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t sWindow5RouteMutex = PTHREAD_MUTEX_INITIALIZER;
static std::deque<SWindow5Rs485Request> sWindow5RequestQueue;
static pthread_t sWindow5WorkerThread;
static bool sWindow5WorkerStarted = false;
static char sWindow5KnownDevice[WINDOW5_DEVICE_NAME_MAX] = {0};

static SWindow5Rs485Result sendWindow5Rs485CommandDetailedSync(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName);
static bool sendWindow5Rs485CommandSync(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName);
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
                                  BYTE *pReturnedAddress) {
    if (pStatus != NULL) {
        *pStatus = 0xFFU;
    }
    if (pReturnedAddress != NULL) {
        *pReturnedAddress = 0xFFU;
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

        if (cmd == WINDOW5_RSP_ACK) {
            return 1;
        }
        if (cmd == WINDOW5_RSP_NACK) {
            return 2;
        }
    }

    return 0;
}

static int waitWindow5Rs485Reply(int fd, BYTE expectedCmd, BYTE *pStatus, BYTE *pReturnedAddress) {
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

        const int replyType = parseWindow5Rs485Reply(rxBuf, rxLen, expectedCmd, pStatus, pReturnedAddress);
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
    const int replyType = waitWindow5Rs485Reply(fd, pFrame[2], &replyStatus, &returnedAddress);
    if (pResult != NULL) {
        pResult->replyType = replyType;
        pResult->status = replyStatus;
        pResult->returnedAddress = returnedAddress;
    }
    if (replyType == 1) {
        if (returnedAddress != 0xFFU) {
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
        if (returnedAddress != 0xFFU) {
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

static SWindow5Rs485Result sendWindow5Rs485CommandDetailedSync(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName) {
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

static bool sendWindow5Rs485CommandSync(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName) {
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(cmd, pData, dataLen, pFrameName);
    return result.replyType != 0;
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
        (void)sendWindow5Rs485CommandSync(req.cmd, req.data, req.dataLen, req.frameName);
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

static bool enqueueWindow5Rs485Command(BYTE cmd, const BYTE *pData, BYTE dataLen, const char *pFrameName) {
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
    sWindow5RequestQueue.push_back(req);
    pthread_cond_signal(&sWindow5QueueCond);
    pthread_mutex_unlock(&sWindow5QueueMutex);

    LOGD("[Window5Rs485] queue push frame=%s cmd=0x%02X\n", pFrameName, cmd);
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
    return sendWindow5Rs485Command(0x21, NULL, 0, "VALVE_OFF");
}

static bool sendWindow5ValveOffCommand() {
    return sendWindow5Rs485Command(0x20, NULL, 0, "VALVE_ON");
}

static void setWindow5TestAddressTip(const char *pText) {
    LOGD("[Window5Rs485] address tip: %s\n", pText ? pText : "");
    if (mTestAdressTipsTextPtr) {
        mTestAdressTipsTextPtr->setText(pText ? pText : "");
        mTestAdressTipsTextPtr->setVisible((pText != NULL) && (pText[0] != '\0'));
    }
}

static bool isWindow5AsciiSpace(char c) {
    return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
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
        setWindow5TestAddressTip("请输入地址，范围1-200");
        return false;
    }

    errno = 0;
    char *pEnd = NULL;
    const long value = strtol(pStart, &pEnd, 10);
    while ((pEnd != NULL) && isWindow5AsciiSpace(*pEnd)) {
        ++pEnd;
    }

    if ((errno != 0) || (pEnd == pStart) || ((pEnd != NULL) && (*pEnd != '\0'))) {
        setWindow5TestAddressTip("地址格式错误，请输入1-200");
        return false;
    }

    if ((value < WINDOW5_TEST_ADDRESS_MIN) || (value > WINDOW5_TEST_ADDRESS_MAX)) {
        setWindow5TestAddressTip("地址越界，请输入1-200");
        return false;
    }

    char normalizedText[16] = {0};
    snprintf(normalizedText, sizeof(normalizedText), "%ld", value);
    if (text != normalizedText) {
        mTestAdressEditTextPtr->setText(normalizedText);
    }

    *pAddress = static_cast<int>(value);
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
        return "从机不支持该指令";
    case 4U:
        return "EEPROM保存失败";
    default:
        return "未知错误";
    }
}

static void setWindow5AddressNoReplyTip(const char *pAction, const SWindow5Rs485Result &result) {
    char tip[96] = {0};
    snprintf(tip, sizeof(tip), "%s%s", pAction, result.sendOk ? "未收到应答" : "发送失败或未收到应答");
    setWindow5TestAddressTip(tip);
}

static void handleWindow5TestAddressTextChanged(const std::string &text) {
    (void)text;
    setWindow5TestAddressTip("");
}

static bool sendWindow5SetAddressCommand() {
    int address = 0;
    if (!parseWindow5TestAddressEditText(&address)) {
        return false;
    }

    const BYTE data[1] = { static_cast<BYTE>(address) };

    LOGD("[Window5Rs485] set address request=%d\n", address);
    setWindow5TestAddressTip("正在修改地址...");
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(WINDOW5_CMD_SET_ADDRESS, data, sizeof(data), "SET_ADDRESS");

    if (result.replyType == 0) {
        setWindow5AddressNoReplyTip("地址修改", result);
        return false;
    }

    if ((result.replyType == 1) && (result.status == 0U)) {
        if (result.returnedAddress == static_cast<BYTE>(address)) {
            char tip[96] = {0};
            snprintf(tip, sizeof(tip), "地址修改成功：%d", address);
            setWindow5TestAddressTip(tip);
            return true;
        }

        char tip[128] = {0};
        snprintf(tip, sizeof(tip), "地址修改异常，返回地址%u", result.returnedAddress);
        setWindow5TestAddressTip(tip);
        return false;
    }

    char tip[128] = {0};
    snprintf(tip, sizeof(tip), "地址修改失败：%s", getWindow5AddressStatusText(result.status));
    setWindow5TestAddressTip(tip);
    return false;
}

static bool sendWindow5CheckAddressCommand() {
    int expectedAddress = 0;
    if (!parseWindow5TestAddressEditText(&expectedAddress)) {
        return false;
    }

    LOGD("[Window5Rs485] check address request=%d\n", expectedAddress);
    setWindow5TestAddressTip("正在核对地址...");
    const SWindow5Rs485Result result = sendWindow5Rs485CommandDetailedSync(WINDOW5_CMD_GET_ADDRESS, NULL, 0, "GET_ADDRESS");

    if (result.replyType == 0) {
        setWindow5AddressNoReplyTip("地址核对", result);
        return false;
    }

    if ((result.replyType == 1) && (result.status == 0U)) {
        if (result.returnedAddress == static_cast<BYTE>(expectedAddress)) {
            char tip[96] = {0};
            snprintf(tip, sizeof(tip), "地址核对成功：%d", expectedAddress);
            setWindow5TestAddressTip(tip);
            return true;
        }

        char tip[128] = {0};
        snprintf(tip, sizeof(tip), "地址核对失败：当前%u，输入%d", result.returnedAddress, expectedAddress);
        setWindow5TestAddressTip(tip);
        return false;
    }

    char tip[128] = {0};
    snprintf(tip, sizeof(tip), "地址核对失败：%s", getWindow5AddressStatusText(result.status));
    setWindow5TestAddressTip(tip);
    return false;
}

static void onPage5Show() {
    setWindow5TestAddressTip("");
}

static void onPage5Hide() {
}
