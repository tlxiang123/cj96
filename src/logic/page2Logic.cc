#include "DeviceDataStore.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/time.h>
#include <vector>

static const char* W2_DEVICE_TYPE_VALVE = "电磁阀";
static const char* W2_DEVICE_TYPE_SENSOR = "传感器";
static int sW2EditingIndex = -1;
static bool sW2AddingDevice = false;
static int sW2CurrentAddress = 0;
static bool sW2SetWindowOpen = false;
static bool sW2TipWindowVisible = false;
static long long sW2TipShownAtMs = 0;
static bool sW2ActionTipWindowVisible = false;
static long long sW2ActionTipShownAtMs = 0;
static bool sGroupRenameWindowVisible = false;
static bool sIrrCapacityWindowVisible = false;
static bool sClearIrrWindowVisible = false;
static bool sW2Window11Visible = false;
static bool sW2TransferMode = false;
static int sW2TransferSourceGroupNo = -1;
static int sW2TransferTargetGroupNo = -1;
static bool sW2Window11TargetUpdating = false;
static int sW2ChoiceDialogMode = 0;
static int sW2ChoiceDialogGroupNo = 1;
static bool sGroupBindAllGroups = false;
static int sIrrCapacityPending = 2;
static int sIrrCapacityLast = 2;
static const int DEFAULT_IRR_GROUP_VALVE_LIMIT = 3;
static const int DEFAULT_IRR_CAPACITY_MODE = 1; // 0=single group, 1=all groups
static int sIrrCapacityMode = DEFAULT_IRR_CAPACITY_MODE;
static int sIrrCapacityGroupNo = 1;
static int sIrrGroupCapacity[129] = {0};
static int sIrrGroupValveLimit = DEFAULT_IRR_GROUP_VALVE_LIMIT;
static bool sIrrCapacityDialogSnapshotValid = false;
static int sIrrCapacityDialogOriginalMode = DEFAULT_IRR_CAPACITY_MODE;
static int sIrrCapacityDialogOriginalGroupNo = 1;
static int sIrrCapacityDialogOriginalPending = 2;
static int sIrrCapacityDialogOriginalLast = 2;
static int sIrrCapacityDialogOriginalValveLimit = DEFAULT_IRR_GROUP_VALVE_LIMIT;
static int sIrrCapacityDialogOriginalGroupCapacity[129] = {0};
static bool sW2AutoAssignPreviewPending = false;
static std::vector<SDATA> sW2AutoAssignDeviceSnapshot;
static std::vector<int> sW2AutoAssignGroupSnapshot;
static bool sW2AutoAssignOriginalAddingDevice = false;
static int sW2AutoAssignOriginalEditingIndex = -1;
static int sW2AutoAssignOriginalSelectedGroupNo = -1;
static int sW2AutoAssignOriginalValveLimit = DEFAULT_IRR_GROUP_VALVE_LIMIT;
// Window2 editor transaction: all group/device edits stay in this working
// snapshot until the editor's confirm button commits them.
static bool sW2EditorSessionActive = false;
static bool sW2EditorRefreshAfterClose = false;
static std::vector<SDATA> sW2EditorDeviceSnapshot;
static std::vector<int> sW2EditorGroupSnapshot;
static std::vector<std::string> sW2EditorGroupNameSnapshot;
static int sW2EditorOriginalCapacityPending = 2;
static int sW2EditorOriginalCapacityLast = 2;
static int sW2EditorOriginalCapacityMode = DEFAULT_IRR_CAPACITY_MODE;
static int sW2EditorOriginalCapacityGroupNo = 1;
static int sW2EditorOriginalValveLimit = DEFAULT_IRR_GROUP_VALVE_LIMIT;
static int sW2EditorOriginalGroupCapacity[129] = {0};
static bool sW2TipCheckedThisVisit = false;
static int sW2SelectedTypeIndex = 0;
static int sSelectedIrrGroupNo = -1;
// Keeps a newly typed transfer target in the editor list even before the
// editor is saved and even after Window11 is closed.
static int sW2EditorPinnedGroupNo = -1;
static int sW2PendingGroupSelectionNo = -1;
static long long sW2PendingGroupSelectionAtMs = 0;
static const int DEFAULT_PUMP_ADVANCE_SECONDS = 5;
static const int MAX_PUMP_ADVANCE_SECONDS = 999;
static int sIrrGroupPumpAdvanceSeconds[128] = {0};
static bool sIrrGroupPumpAdvanceSecondsSet[128] = {false};
static ZKEditText* sPumpAdvanceSecEditTextPtr = NULL;
static std::vector<int> createInitialIrrGroupNumbers() {
    std::vector<int> groups;
    groups.push_back(1);
    groups.push_back(2);
    groups.push_back(3);
    groups.push_back(4);
    return groups;
}
static std::vector<int> sIrrGroupNumbers = createInitialIrrGroupNumbers();
static bool sIrrEmptyItemLayoutCaptured = false;

static bool isValidIrrGroupNo(int groupNo);
static int normalizeW2TransferTargetGroupNo(int groupNo);
static bool isDeviceBoundToIrrGroup(const SDATA* data, int groupNo);

static void ensureIrrGroupNumberVisible(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }
    if (std::find(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), groupNo) ==
            sIrrGroupNumbers.end()) {
        sIrrGroupNumbers.push_back(groupNo);
        std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
    }
}

static void syncIrrGroupNumbersFromDeviceData() {
    for (int groupNo = 1; groupNo <= 128; ++groupNo) {
        bool found = false;
        for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
            const SDATA* data = DeviceDataStore::getDevice(i);
            if (data && DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
                found = true;
                break;
            }
        }
        if (found) {
            ensureIrrGroupNumberVisible(groupNo);
        }
    }

    // Keep an editor-only target visible while the current valve is still a
    // preview and has not been saved to DeviceDataStore yet.  Without this,
    // the next list refresh rebuilds the visible rows from persisted devices
    // and a target such as group 28 falls back to the old tail group 4.
    if (sW2SetWindowOpen) {
        ensureIrrGroupNumberVisible(sSelectedIrrGroupNo);
        ensureIrrGroupNumberVisible(sW2EditorPinnedGroupNo);
        if (sW2Window11Visible) {
            ensureIrrGroupNumberVisible(sW2TransferTargetGroupNo);
        }
    }
    std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
}
static LayoutPosition sIrrNumSubItemPosition;
static LayoutPosition sIrrArrSubItemPosition;
static std::vector<int> sSelectedPumpDeviceIndexes;
static std::vector<int> sSelectedSensorDeviceIndexes;

static void setListSubItemText(ZKListView::ZKListItem *pListItem, int id, const char* text);
static void setListSubItemAlignment(ZKListView::ZKListItem *pListItem, int id, ZKTextView::EAlignH h, ZKTextView::EAlignV v);
static void setListSubItemVisible(ZKListView::ZKListItem *pListItem, int id, bool visible);
static void setListSubItemPosition(ZKListView::ZKListItem *pListItem, int id, const LayoutPosition &position);
static bool isValidIrrGroupNo(int groupNo);
static bool isIrrGroupEmptyRow(int index);
static void setIrrGroupSubItemTexts(ZKListView::ZKListItem *pListItem, int index, int numSubItemId, int nameSubItemId);
static void hideAllPageWindows();
static void hideGroupBindWindowOnly();
static void hideW2SetWindowOnly();
static void hideW2TipWindowOnly();
static void updateClearIrrButtonText();
static void hideW2Window11Only();
static bool hideW2Window11IfTouchedOutside(const MotionEvent &ev);
static void beginW2EditorSession();
static void restoreW2EditorSession();
static void commitW2EditorSession();
static void openW2Window11(int groupNo);
static void updateW2PendingGroupSelection();
static void updateW2ChoiceDialogDisplay();
static void stepW2ChoiceDialogGroup(int delta);
static void openW2GroupChoiceWindow(int mode);
static void openDeleteGroupWindow();
static void openRenameGroupChoiceWindow();
static void deleteSelectedIrrGroupFromOverview(int groupNo);
static void removeCurrentW2DeviceFromGroup();
static void beginW2DeviceTransfer();
static void updateW2Window11TargetGroupDisplay(int sourceGroupNo);
static void onW2Window11TargetGroupTextChanged(const std::string &text);
static void incrementW2Window11TargetGroup();
static bool hideW2TipWindowIfVisible();
static bool handlePage2BeforeMainPageSwitch(int targetPageIndex);
static void closeW2SetWindow();
static void openW2SetWindow(int index);
static const char* getIrrGroupName(int groupNo);
static void buildIrrGroupDisplayText(int groupNo, char* text, size_t size);
static void closeGroupBindWindow();
static void updateGroupBindSelectionEditTexts();
static void updateW2AddressDisplay(int address);
static const char* getW2SelectedDeviceType();
static bool isPumpDevice(const SDATA* data);
static bool isGroupBindPumpDevice(const SDATA* data);
static bool isSensorBindDevice(const SDATA* data);
static bool isSensorTextDevice(const SDATA* data);
static void refreshRunTimeListView();
static bool isW2PreviewValveForGroup(int groupNo);
static int getW2PreviewAddress();
static int getW2DefaultAddress();

static void setW2TipText(const char* text);
static bool showW2TipText(const char* text);
static bool collectUngroupedValveAddresses(char* text, size_t size);
static void openIrrCapacityWindow();
static void selectIrrCapacity(int capacity);
static void setIrrCapacityMode(int mode);
static void stepIrrCapacityGroup(int delta);
static void stepIrrCapacityValue(int delta);
static void closeIrrCapacityWindow(bool save);
static void autoAssignUngroupedValves(int capacity);
static int getIrrGroupCapacity(int groupNo);
static int getIrrGroupValveCount(int groupNo);
static bool canAssignCurrentW2ValveToIrrGroup(int groupNo, bool showTip);
static bool sPage2Active = false;
static bool sPage2CachedDiscoveryRunning = false;
static bool sPage2DiscoveryTipActive = false;
static long long sPage2DiscoveryTipStartedAtMs = 0;
static int sPage2DiscoveryTipLastRemainingSeconds = -1;

// The board scans addresses 20..255 in one 236-slot pass. The discovery
// response already carries address, decoder type, and device state, so the
// local board uses that single pass as its completion boundary.
static const int PAGE2_DISCOVERY_ADDRESS_COUNT = 255 - 20 + 1;
static const int PAGE2_DISCOVERY_SLOT_MS = 60;
static const int PAGE2_DISCOVERY_START_GUARD_MS = 200;
static const int PAGE2_DISCOVERY_END_GUARD_MS = 500;
static const int PAGE2_DISCOVERY_MAX_MS =
        PAGE2_DISCOVERY_START_GUARD_MS
        + PAGE2_DISCOVERY_ADDRESS_COUNT * PAGE2_DISCOVERY_SLOT_MS
        + PAGE2_DISCOVERY_END_GUARD_MS;
static const int PAGE2_DISCOVERY_MAX_SECONDS =
        (PAGE2_DISCOVERY_MAX_MS + 999) / 1000;
static bool sPage2TuyaSyncPending = false;
static unsigned int sPage2TuyaSyncSession = 0;
static const char* PAGE2_TUYA_SYNC_QUEUE_PATH =
        "/tmp/cj96_tuya_demo/device_sync_report.queue";
// DP101 maxlen is 1024. Keep protocol frames below 960 ASCII bytes.
static const size_t PAGE2_TUYA_SYNC_CHUNK_HEX_MAX = 900U;
static bool sW2AddDeviceWindowVisible = false;
static bool sW2AddDeviceSensor = false;
static int sW2AddDeviceAddress = CUSTOM_DEVICE_START_ID;
static const int W2_ADD_DEVICE_ADDRESS_MIN = CUSTOM_DEVICE_START_ID;
static const int W2_ADD_DEVICE_ADDRESS_MAX = CUSTOM_DEVICE_END_ID;

static unsigned int page2Fnv1a(const unsigned char* data, size_t size) {
    unsigned int hash = 2166136261U;
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<unsigned int>(data[i]);
        hash *= 16777619U;
    }
    return hash;
}

static void appendPage2SyncByte(std::vector<unsigned char>& bytes,
                                unsigned int value) {
    bytes.push_back(static_cast<unsigned char>(value & 0xFFU));
}

static void appendPage2SyncText(std::vector<unsigned char>& bytes,
                                const char* text,
                                size_t capacity) {
    const char* source = text ? text : "";
    const size_t length = strnlen(source, capacity);
    appendPage2SyncByte(bytes, static_cast<unsigned int>(length));
    bytes.insert(bytes.end(), source, source + length);
}

static unsigned int getPage2SyncType(const SDATA* data) {
    if (!data) return 0U;
    if (std::strcmp(data->type, "水泵") == 0) return 1U;
    if (std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0) return 2U;
    if (std::strcmp(data->type, W2_DEVICE_TYPE_SENSOR) == 0) return 3U;
    return 0U;
}

static unsigned int encodePage2SyncGroups(const char* text,
                                          unsigned char groups[16]) {
    std::memset(groups, 0, 16);
    if (!text || text[0] == '\0' || std::strcmp(text, "-") == 0) {
        return 0U;
    }
    if (std::strcmp(text, "*") == 0) {
        return 2U;
    }

    const char* cursor = text;
    bool found = false;
    while (*cursor != '\0') {
        char* end = NULL;
        const long value = std::strtol(cursor, &end, 10);
        if (end == cursor || value < 1 || value > 128) {
            break;
        }
        const unsigned int index = static_cast<unsigned int>(value - 1);
        groups[index / 8U] |= static_cast<unsigned char>(1U << (index % 8U));
        found = true;
        cursor = (*end == ',') ? end + 1 : end;
        if (*end == '\0') break;
    }
    return found ? 1U : 0U;
}

static std::string makePage2Hex(const std::vector<unsigned char>& bytes) {
    static const char kHex[] = "0123456789ABCDEF";
    std::string result;
    result.resize(bytes.size() * 2U);
    for (size_t i = 0; i < bytes.size(); ++i) {
        result[i * 2U] = kHex[(bytes[i] >> 4) & 0x0FU];
        result[i * 2U + 1U] = kHex[bytes[i] & 0x0FU];
    }
    return result;
}

static std::vector<unsigned char> buildPage2DeviceTableBytes() {
    std::vector<unsigned char> bytes;
    const int count = DeviceDataStore::getDeviceCount();
    bytes.reserve(static_cast<size_t>(count) * 64U);
    for (int i = 0; i < count; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data) continue;
        appendPage2SyncByte(bytes, static_cast<unsigned int>(data->address));
        appendPage2SyncByte(bytes, getPage2SyncType(data));
        appendPage2SyncByte(bytes,
            (data->connected ? 1U : 0U) |
            (data->stateKnown ? 2U : 0U) |
            (data->state ? 4U : 0U));
        unsigned char groups[16] = {0};
        appendPage2SyncByte(bytes, encodePage2SyncGroups(data->arre, groups));
        bytes.insert(bytes.end(), groups, groups + 16);
        appendPage2SyncText(bytes, data->name, sizeof(data->name));
        appendPage2SyncText(bytes, data->status, sizeof(data->status));
    }
    return bytes;
}

static std::vector<unsigned char> buildPage2GroupCapacityBytes() {
    std::vector<unsigned char> bytes;
    bytes.reserve(128U);
    for (int groupNo = 1; groupNo <= 128; ++groupNo) {
        appendPage2SyncByte(bytes, static_cast<unsigned int>(getIrrGroupCapacity(groupNo)));
    }
    return bytes;
}

static unsigned int getPage2DeviceTableHash() {
    const std::vector<unsigned char> bytes = buildPage2DeviceTableBytes();
    const std::vector<unsigned char> groupCapacityBytes = buildPage2GroupCapacityBytes();
    std::vector<unsigned char> hashBytes = bytes;
    hashBytes.insert(hashBytes.end(), groupCapacityBytes.begin(), groupCapacityBytes.end());
    return page2Fnv1a(hashBytes.empty() ? NULL : &hashBytes[0], hashBytes.size());
}

bool isPage2DeviceTableHashCurrent(unsigned int expectedHash) {
    const unsigned int actualHash = getPage2DeviceTableHash();
    LOGD("[Page2DeviceSync] command revision expected=%08X actual=%08X match=%d\n",
         expectedHash, actualHash, expectedHash == actualHash ? 1 : 0);
    return expectedHash == actualHash;
}

static bool writePage2TuyaSyncQueue(unsigned int session,
                                    int addedValveCount,
                                    int addedSensorCount,
                                    bool versionCheck = false,
                                    unsigned int expectedHash = 0U) {
    const std::vector<unsigned char> bytes = buildPage2DeviceTableBytes();
    const std::vector<unsigned char> groupCapacityBytes = buildPage2GroupCapacityBytes();
    const int count = DeviceDataStore::getDeviceCount();
    const std::string hex = makePage2Hex(bytes);
    const std::string groupCapacityHex = makePage2Hex(groupCapacityBytes);
    const size_t chunkCount = hex.empty() ? 0U :
        (hex.size() + PAGE2_TUYA_SYNC_CHUNK_HEX_MAX - 1U) /
        PAGE2_TUYA_SYNC_CHUNK_HEX_MAX;
    const unsigned int dataHash = getPage2DeviceTableHash();
    std::vector<std::string> frames;
    char frame[1024] = {0};
    const bool sameVersion = versionCheck && expectedHash == dataHash;
    if (versionCheck) {
        snprintf(frame, sizeof(frame), "CJDS1|V|%04X|%c|%d|%08X",
                 session & 0xFFFFU, sameVersion ? 'S' : 'C', count, dataHash);
        if (std::strlen(frame) > 960U) {
            LOGD("[Page2DeviceSync] oversized version frame len=%u\n",
                 static_cast<UINT>(std::strlen(frame)));
            return false;
        }
        frames.push_back(frame);
        LOGD("[Page2DeviceSync] version check expected=%08X actual=%08X same=%d\n",
             expectedHash, dataHash, sameVersion ? 1 : 0);
    }
    // Version checks only answer with V.  The APP requests the full table with
    // F502 when the hash differs, avoiding duplicate V/G/T and fallback reads.
    if (!versionCheck) {
        snprintf(frame, sizeof(frame), "CJDS1|G|%04X|%s|%08X",
                 session & 0xFFFFU, groupCapacityHex.c_str(), dataHash);
        if (std::strlen(frame) > 960U) {
            LOGD("[Page2DeviceSync] oversized capacity frame len=%u\n",
                 static_cast<UINT>(std::strlen(frame)));
            return false;
        }
        frames.push_back(frame);
        snprintf(frame, sizeof(frame), "CJDS1|T|%04X|%d|%d|%d|%s|%08X",
                 session & 0xFFFFU, count, addedValveCount, addedSensorCount,
                 hex.c_str(), dataHash);
        if (std::strlen(frame) <= 960U) {
            frames.push_back(frame);
            LOGD("[Page2DeviceSync] using single frame devices=%d len=%u\n",
                 count, static_cast<UINT>(std::strlen(frame)));
        } else {
            snprintf(frame, sizeof(frame), "CJDS1|S|%04X|%d|%04u|%08X",
                     session & 0xFFFFU, count,
                     static_cast<unsigned int>(chunkCount), dataHash);
            frames.push_back(frame);
            for (size_t i = 0; i < chunkCount; ++i) {
                const std::string payload = hex.substr(
                    i * PAGE2_TUYA_SYNC_CHUNK_HEX_MAX,
                    PAGE2_TUYA_SYNC_CHUNK_HEX_MAX);
                const unsigned int chunkHash = page2Fnv1a(
                    reinterpret_cast<const unsigned char*>(payload.data()),
                    payload.size());
                snprintf(frame, sizeof(frame), "CJDS1|C|%04X|%04u|%04u|%s|%08X",
                         session & 0xFFFFU, static_cast<unsigned int>(i + 1U),
                         static_cast<unsigned int>(chunkCount), payload.c_str(), chunkHash);
                if (std::strlen(frame) > 960U) {
                    LOGD("[Page2DeviceSync] oversized frame len=%u\n",
                         static_cast<UINT>(std::strlen(frame)));
                    return false;
                }
                frames.push_back(frame);
            }
            snprintf(frame, sizeof(frame), "CJDS1|E|%04X|%d|%04u|%08X",
                     session & 0xFFFFU, count,
                     static_cast<unsigned int>(chunkCount), dataHash);
            frames.push_back(frame);
            snprintf(frame, sizeof(frame), "CJDS1|R|%04X|%d|%d|%d",
                     session & 0xFFFFU, addedValveCount, addedSensorCount, count);
            frames.push_back(frame);
        }
    }

    (void)mkdir("/tmp/cj96_tuya_demo", 0755);
    const std::string temporaryPath = std::string(PAGE2_TUYA_SYNC_QUEUE_PATH) + ".new";
    FILE* output = fopen(temporaryPath.c_str(), "wb");
    if (!output) return false;
    bool okay = true;
    for (size_t i = 0; i < frames.size(); ++i) {
        if (fprintf(output, "%s\n", frames[i].c_str()) < 0) {
            okay = false;
            break;
        }
    }
    okay = okay && fflush(output) == 0 && fsync(fileno(output)) == 0;
    okay = fclose(output) == 0 && okay;
    if (!okay || rename(temporaryPath.c_str(), PAGE2_TUYA_SYNC_QUEUE_PATH) != 0) {
        (void)unlink(temporaryPath.c_str());
        return false;
    }
    LOGD("[Page2DeviceSync] queued session=%04X devices=%d chunks=%u bytes=%u\n",
         session & 0xFFFFU, count, static_cast<UINT>(chunkCount),
         static_cast<UINT>(bytes.size()));
    return true;
}

static long long getW2CurrentTimeMs() {
    struct timeval value;
    gettimeofday(&value, NULL);
    return static_cast<long long>(value.tv_sec) * 1000LL
            + static_cast<long long>(value.tv_usec) / 1000LL;
}

static bool isPage2DeviceDiscoveryTipActive() {
    return sPage2DiscoveryTipActive;
}

static int getPage2DeviceDiscoveryRemainingSeconds() {
    if (sPage2DiscoveryTipStartedAtMs <= 0) {
        return PAGE2_DISCOVERY_MAX_SECONDS;
    }
    const long long elapsedMs = getW2CurrentTimeMs() - sPage2DiscoveryTipStartedAtMs;
    const long long remainingMs =
            static_cast<long long>(PAGE2_DISCOVERY_MAX_MS) - elapsedMs;
    if (remainingMs <= 0) {
        return 0;
    }
    return static_cast<int>((remainingMs + 999LL) / 1000LL);
}

static void makePage2DeviceDiscoveryTipText(int remainingSeconds,
                                             char* text,
                                             size_t size) {
    if (!text || size == 0) {
        return;
    }
    snprintf(text, size,
             "\xE5\x90\x8C\xE6\xAD\xA5\xE4\xB8\xAD\xEF\xBC\x8C\xE8\xAF\xB7\xE7\xAD\x89\xE5\xBE\x85\n"
             "\xE5\x89\xA9\xE4\xBD\x99 %d \xE7\xA7\x92",
             remainingSeconds);
}

static void startPage2DeviceDiscoveryTip() {
    sPage2DiscoveryTipActive = true;
    sPage2DiscoveryTipStartedAtMs = getW2CurrentTimeMs();
    sPage2DiscoveryTipLastRemainingSeconds = -1;
    char text[128] = {0};
    const int remainingSeconds = getPage2DeviceDiscoveryRemainingSeconds();
    makePage2DeviceDiscoveryTipText(remainingSeconds, text, sizeof(text));
    (void)showW2TipText(text);
    sPage2DiscoveryTipLastRemainingSeconds = remainingSeconds;
    LOGD("[Page2DeviceSync] modal started max_seconds=%d\n",
         PAGE2_DISCOVERY_MAX_SECONDS);
}

static void updatePage2DeviceDiscoveryCountdown() {
    updateW2PendingGroupSelection();
    if (!sPage2DiscoveryTipActive) {
        return;
    }
    if (!isWindow5DeviceDiscoveryRunning()) {
        sPage2DiscoveryTipActive = false;
        sPage2DiscoveryTipStartedAtMs = 0;
        sPage2DiscoveryTipLastRemainingSeconds = -1;
        LOGD("[Page2DeviceSync] modal finished\n");
        return;
    }

    const int remainingSeconds = getPage2DeviceDiscoveryRemainingSeconds();
    if (remainingSeconds == sPage2DiscoveryTipLastRemainingSeconds) {
        return;
    }
    char text[128] = {0};
    makePage2DeviceDiscoveryTipText(remainingSeconds, text, sizeof(text));
    setW2TipText(text);
    sPage2DiscoveryTipLastRemainingSeconds = remainingSeconds;
}

bool requestPage2DeviceDiscoveryFromTuya() {
    if (isWindow5DeviceDiscoveryRunning()) {
        return false;
    }
    if (!requestWindow5DeviceDiscovery()) {
        return false;
    }
    sPage2TuyaSyncPending = true;
    sPage2TuyaSyncSession = static_cast<unsigned int>(getW2CurrentTimeMs()) & 0xFFFFU;
    startPage2DeviceDiscoveryTip();
    return true;
}

static void completePage2DeviceDiscoveryForTuya(int addedValveCount,
                                                 int addedSensorCount) {
    if (!sPage2TuyaSyncPending) {
        return;
    }
    const unsigned int session = sPage2TuyaSyncSession;
    sPage2TuyaSyncPending = false;
    sPage2TuyaSyncSession = 0U;
    const bool queued = writePage2TuyaSyncQueue(
        session, addedValveCount, addedSensorCount);
    LOGD("[Page2DeviceSync] cloud result session=%04X valves=%d sensors=%d queued=%d\n",
         session, addedValveCount, addedSensorCount, queued ? 1 : 0);
}

static bool hasBoundValveInIrrGroup(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return false;
    }

    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data
                && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
                && DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            return true;
        }
    }
    return false;
}

static int resolveSelectedIrrGroupNo() {
    // A scope action is allowed to use the group already shown in the editor,
    // including an empty/full group.  Membership is not a prerequisite here.
    if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return sSelectedIrrGroupNo;
    }

    // Window11 can leave a typed target pinned while the current device itself
    // is still ungrouped.  Keep that number as the next scope target.
    if (isValidIrrGroupNo(sW2EditorPinnedGroupNo)) {
        sSelectedIrrGroupNo = sW2EditorPinnedGroupNo;
        return sSelectedIrrGroupNo;
    }

    const int total = DeviceDataStore::getDeviceCount();
    if (sW2CurrentAddress > 0) {
        for (int i = 0; i < total; ++i) {
            const SDATA* data = DeviceDataStore::getDevice(i);
            if (!data || data->address != sW2CurrentAddress) {
                continue;
            }

            // Use the actual membership helper first so multi-group values and
            // the special all-groups marker are handled consistently.
            for (int groupNo = 1; groupNo <= 128; ++groupNo) {
                if (isDeviceBoundToIrrGroup(data, groupNo)) {
                    sSelectedIrrGroupNo = groupNo;
                    return groupNo;
                }
            }

            // Keep compatibility with the persisted single-number form.
            const int groupNo = atoi(data->arre);
            if (isValidIrrGroupNo(groupNo)) {
                sSelectedIrrGroupNo = groupNo;
                return groupNo;
            }
            break;
        }
    }

    // There is no current-device membership to infer (for example a newly
    // added or ungrouped valve).  Use the first visible real group instead of
    // forcing another tap on the left list.
    syncIrrGroupNumbersFromDeviceData();
    for (size_t i = 0; i < sIrrGroupNumbers.size(); ++i) {
        if (isValidIrrGroupNo(sIrrGroupNumbers[i])) {
            sSelectedIrrGroupNo = sIrrGroupNumbers[i];
            return sSelectedIrrGroupNo;
        }
    }

    // Keep the direct-action path usable even after all group rows were
    // removed from the working editor snapshot.
    ensureIrrGroupNumberVisible(1);
    sSelectedIrrGroupNo = 1;
    LOGD("[Window2] action group fallback=1 selected=%d pinned=%d address=%d\n",
         sSelectedIrrGroupNo, sW2EditorPinnedGroupNo, sW2CurrentAddress);
    return sSelectedIrrGroupNo;
}

static bool requireSelectedIrrGroup() {
    // List-row actions still require an explicit row selection. Scope-based
    // actions use requireW2ActionIrrGroup() below and may resolve the group
    // from the number already displayed on the W2 overview.
    if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return true;
    }
    if (mW2ActionTipTextViewPtr) {
        mW2ActionTipTextViewPtr->setText(
                "\xE8\xAF\xB7\xE5\x85\x88\xE9\x80\x89\xE6\x8B\xA9\xE9\x98\x80\xE7\xBB\x84");
    }
    if (mW2ActionTipWindowPtr) {
        mW2ActionTipWindowPtr->showWnd();
        sW2ActionTipWindowVisible = true;
        sW2ActionTipShownAtMs = getW2CurrentTimeMs();
    }
    return false;
}

static bool requireW2ActionIrrGroup() {
    // Window9 actions are group-scope operations. They must work even when
    // the current valve is not in the displayed group or that group is full.
    // Resolve the numeric group from the overview state/device membership,
    // without using capacity checks or requiring a row click first.
    const int beforeSelectedGroupNo = sSelectedIrrGroupNo;
    if (!isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        const int resolvedGroupNo = resolveSelectedIrrGroupNo();
        if (isValidIrrGroupNo(resolvedGroupNo)) {
            sSelectedIrrGroupNo = resolvedGroupNo;
            sW2EditorPinnedGroupNo = resolvedGroupNo;
            updateClearIrrButtonText();
        }
    }
    LOGD("[Window2] action group selected_before=%d selected=%d pinned=%d address=%d editing=%d adding=%d\n",
         beforeSelectedGroupNo, sSelectedIrrGroupNo, sW2EditorPinnedGroupNo,
         sW2CurrentAddress, sW2EditingIndex, sW2AddingDevice ? 1 : 0);
    if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return true;
    }
    if (mW2ActionTipTextViewPtr) {
        mW2ActionTipTextViewPtr->setText(
                "\xE8\xaf\xb7\xe5\x85\x88\xe6\x89\x8b\xe5\x8a\xa8\xe9\x80\x89\xe6\x8b\xa9\xe9\x98\x80\xe7\xbb\x84");
    }
    if (mW2ActionTipWindowPtr) {
        mW2ActionTipWindowPtr->showWnd();
        sW2ActionTipWindowVisible = true;
        sW2ActionTipShownAtMs = getW2CurrentTimeMs();
    }
    return false;
}

static void hideW2ActionTipWindowOnly() {
    if (mW2ActionTipWindowPtr) {
        mW2ActionTipWindowPtr->hideWnd();
    }
    sW2ActionTipWindowVisible = false;
    sW2ActionTipShownAtMs = 0;
}

static bool hideW2ActionTipWindowIfVisible() {
    if (!sW2ActionTipWindowVisible) {
        return false;
    }
    if (getW2CurrentTimeMs() - sW2ActionTipShownAtMs < 250LL) {
        return true;
    }
    hideW2ActionTipWindowOnly();
    return true;
}

static bool sW2RefreshingChangeIrrList = false;
static bool sW2ProgrammaticGroupSelection = false;

static void refreshDeviceListViews() {
    sPage2CachedDiscoveryRunning = isWindow5DeviceDiscoveryRunning();
    if (mDeviceTipListViewPtr) {
        mDeviceTipListViewPtr->refreshListView();
    }
    if (mDeviceListViewPtr) {
        mDeviceListViewPtr->refreshListView();
    }
    refreshRunTimeListView();
}

static void refreshChangeIrrListView() {
    // refreshListView() can synchronously invoke the row adapter and the
    // selection callback.  A callback that refreshes the same ListView again
    // re-enters the FTU event loop and can stop both touch dispatch and the
    // one-second UI timer.  Keep this refresh single-pass.
    if (sW2RefreshingChangeIrrList) {
        return;
    }
    sW2RefreshingChangeIrrList = true;

    syncIrrGroupNumbersFromDeviceData();
    if (mChangeIrr_ListViewPtr) {
        mChangeIrr_ListViewPtr->refreshListView();
        // Do not call setSelection() for ordinary refreshes.  The selected
        // group is rendered by setIrrGroupSubItemTexts() from
        // sSelectedIrrGroupNo; forcing the ListView selection here also moves
        // the viewport (for example, clicking group 2 changes 1..4 to 2..5).
        // Explicit navigation requests are handled only by
        // updateW2PendingGroupSelection().
        // The FTU list adapter may finish laying out its rows after
        // refreshListView() returns. Keep a short retry so a newly-created
        // target such as group 28 is actually brought into view instead of
        // leaving the viewport at the old tail row group 4.
        updateW2PendingGroupSelection();
    }
    refreshWindow4ListViews();
    sW2RefreshingChangeIrrList = false;
}

static void requestW2GroupSelection(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }
    sW2PendingGroupSelectionNo = groupNo;
    sW2PendingGroupSelectionAtMs = getW2CurrentTimeMs();
    updateW2PendingGroupSelection();
}

static void updateW2PendingGroupSelection() {
    if (!isValidIrrGroupNo(sW2PendingGroupSelectionNo)) {
        return;
    }
    const std::vector<int>::const_iterator targetIt = std::find(
            sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(),
            sW2PendingGroupSelectionNo);
    if (mChangeIrr_ListViewPtr && targetIt != sIrrGroupNumbers.end()) {
        sW2ProgrammaticGroupSelection = true;
        mChangeIrr_ListViewPtr->setSelection(
                static_cast<int>(targetIt - sIrrGroupNumbers.begin()));
        sW2ProgrammaticGroupSelection = false;
    }
    if (sW2PendingGroupSelectionAtMs > 0
            && getW2CurrentTimeMs() - sW2PendingGroupSelectionAtMs >= 2500LL) {
        sW2PendingGroupSelectionNo = -1;
        sW2PendingGroupSelectionAtMs = 0;
    }
}

static void removeDeletedDeviceFromSelection(std::vector<int>& indexes,
                                             int deletedIndex) {
    indexes.erase(
        std::remove(indexes.begin(), indexes.end(), deletedIndex),
        indexes.end());
    for (size_t i = 0; i < indexes.size(); ++i) {
        if (indexes[i] > deletedIndex) {
            --indexes[i];
        }
    }
}

static void clearDeletedDeviceSelection(int deletedIndex) {
    removeDeletedDeviceFromSelection(sSelectedPumpDeviceIndexes, deletedIndex);
    removeDeletedDeviceFromSelection(sSelectedSensorDeviceIndexes, deletedIndex);
    if (sW2EditingIndex == deletedIndex) {
        sW2EditingIndex = -1;
    } else if (sW2EditingIndex > deletedIndex) {
        --sW2EditingIndex;
    }
}

bool deletePage2DeviceByAddressFromTuya(int address) {
    bool deleted = false;
    const int count = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < count; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || data->address != address || address < CUSTOM_DEVICE_START_ID) {
            continue;
        }
        deleted = DeviceDataStore::deleteDevice(i);
        if (deleted) {
            clearDeletedDeviceSelection(i);
            if (sW2CurrentAddress == address) {
                sW2CurrentAddress = 0;
            }
            refreshDeviceListViews();
            refreshChangeIrrListView();
        }
        break;
    }

    const unsigned int session =
        static_cast<unsigned int>(getW2CurrentTimeMs()) & 0xFFFFU;
    const bool queued = writePage2TuyaSyncQueue(session, 0, 0);
    LOGD("[Page2DeviceSync] delete address=%d deleted=%d queued=%d\n",
         address, deleted ? 1 : 0, queued ? 1 : 0);
    return deleted;
}

static void hideAllPageWindows() {
    ZKWindow* windows[] = {
        mWindow1Ptr,
        mWindow2Ptr,
        mWindow3Ptr,
        mWindow4Ptr,
        mWindow5Ptr,
        mCycleWindowPtr,
        mw2set_windowPtr,
        mGroupBindValueWindowPtr,
        mWindow11Ptr,
    };

    for (int i = 0; i < static_cast<int>(sizeof(windows) / sizeof(windows[0])); ++i) {
        if (windows[i]) {
            windows[i]->hideWnd();
        }
    }
}

static ZKWindow* getW2AddDeviceWindow() {
    return mWindow2Ptr ?
            (ZKWindow*)mWindow2Ptr->findControlByID(ID_MAIN_W2AddDeviceWindow) :
            NULL;
}

static ZKEditText* getW2AddDeviceAddressEditText() {
    ZKWindow* window = getW2AddDeviceWindow();
    return window ?
            (ZKEditText*)window->findControlByID(ID_MAIN_W2AddDeviceAddressEditText) :
            NULL;
}

static ZKTextView* getW2AddDeviceStatusText() {
    ZKWindow* window = getW2AddDeviceWindow();
    return window ?
            (ZKTextView*)window->findControlByID(ID_MAIN_W2AddDeviceStatusText) :
            NULL;
}

static int clampW2AddDeviceAddress(int address) {
    if (address < W2_ADD_DEVICE_ADDRESS_MIN) {
        return W2_ADD_DEVICE_ADDRESS_MIN;
    }
    if (address > W2_ADD_DEVICE_ADDRESS_MAX) {
        return W2_ADD_DEVICE_ADDRESS_MAX;
    }
    return address;
}

static void setW2AddDeviceStatusText(const char* text) {
    ZKTextView* statusText = getW2AddDeviceStatusText();
    if (statusText) {
        statusText->setText(text ? text : "");
    }
}

static void setW2AddDeviceAddressText(int address) {
    sW2AddDeviceAddress = clampW2AddDeviceAddress(address);
    ZKEditText* editText = getW2AddDeviceAddressEditText();
    if (editText) {
        editText->setText(sW2AddDeviceAddress);
    }
}

static int readW2AddDeviceAddressText() {
    ZKEditText* editText = getW2AddDeviceAddressEditText();
    if (!editText) {
        return sW2AddDeviceAddress;
    }
    const std::string text = editText->getText();
    if (text.empty()) {
        return sW2AddDeviceAddress;
    }
    return clampW2AddDeviceAddress(atoi(text.c_str()));
}

static void updateW2AddDeviceTypeButtons() {
    ZKWindow* window = getW2AddDeviceWindow();
    ZKButton* sensorButton = window ?
            (ZKButton*)window->findControlByID(ID_MAIN_W2AddDeviceSensorButton) : NULL;
    ZKButton* valveButton = window ?
            (ZKButton*)window->findControlByID(ID_MAIN_W2AddDeviceValveButton) : NULL;
    if (sensorButton) {
        sensorButton->setSelected(sW2AddDeviceSensor);
    }
    if (valveButton) {
        valveButton->setSelected(!sW2AddDeviceSensor);
    }
}

static int findW2DeviceIndexByAddress(int address) {
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && data->address == address) {
            return i;
        }
    }
    return -1;
}

static void hideW2AddDeviceWindowOnly() {
    ZKWindow* window = getW2AddDeviceWindow();
    if (window) {
        window->hideWnd();
    }
    sW2AddDeviceWindowVisible = false;
}

static void openW2AddDeviceWindow() {
    sW2AddDeviceSensor = false;
    setW2AddDeviceAddressText(getW2DefaultAddress());
    setW2AddDeviceStatusText("");
    updateW2AddDeviceTypeButtons();
    ZKWindow* window = getW2AddDeviceWindow();
    if (window) {
        window->showWnd();
        sW2AddDeviceWindowVisible = true;
    }
}

static void stepW2AddDeviceAddress(int delta) {
    const int address = readW2AddDeviceAddressText();
    setW2AddDeviceAddressText(address + delta);
    setW2AddDeviceStatusText("");
}

static void openW2SetWindowFromAddDeviceDialog() {
    const int address = sW2AddDeviceAddress;
    const bool sensor = sW2AddDeviceSensor;
    hideW2AddDeviceWindowOnly();
    openW2SetWindow(DeviceDataStore::getDeviceCount());
    sW2SelectedTypeIndex = sensor ? 1 : 0;
    updateW2AddressDisplay(address);
    if (mW2_NameEditTextPtr) {
        mW2_NameEditTextPtr->setText(getW2SelectedDeviceType());
    }
    refreshChangeIrrListView();
}

static void confirmW2AddDeviceWindow() {
    const int address = readW2AddDeviceAddressText();
    setW2AddDeviceAddressText(address);
    setW2AddDeviceStatusText("正在测试地址...");

    char message[128] = {0};
    if (!requestWindow5CheckConfigForW2Add(
            sW2AddDeviceAddress, sW2AddDeviceSensor, message, sizeof(message))) {
        showW2TipText(message);
        return;
    }

    if (findW2DeviceIndexByAddress(sW2AddDeviceAddress) >= 0) {
        showW2TipText("地址已在列表中");
        return;
    }

    openW2SetWindowFromAddDeviceDialog();
}

static bool handleW2AddDeviceWindowClick(ZKBase *pBase) {
    if (!sW2AddDeviceWindowVisible || !pBase) {
        return false;
    }

    const int id = pBase->getID();
    switch (id) {
    case ID_MAIN_W2AddDeviceSensorButton:
        sW2AddDeviceSensor = true;
        updateW2AddDeviceTypeButtons();
        setW2AddDeviceStatusText("");
        return true;
    case ID_MAIN_W2AddDeviceValveButton:
        sW2AddDeviceSensor = false;
        updateW2AddDeviceTypeButtons();
        setW2AddDeviceStatusText("");
        return true;
    case ID_MAIN_W2AddDeviceAddressPrevButton:
        stepW2AddDeviceAddress(-1);
        return true;
    case ID_MAIN_W2AddDeviceAddressNextButton:
        stepW2AddDeviceAddress(1);
        return true;
    case ID_MAIN_W2AddDeviceCancelButton:
        hideW2AddDeviceWindowOnly();
        return true;
    case ID_MAIN_W2AddDeviceOkButton:
        confirmW2AddDeviceWindow();
        return true;
    case ID_MAIN_W2AddDeviceAddressEditText:
        return false;
    default:
        return true;
    }
}

static void setW2TipText(const char* text) {
#if defined(ID_MAIN_W2TextView)
    if (mW2TextViewPtr) {
        mW2TextViewPtr->setText(text);
    }
#elif defined(ID_MAIN_W2TipTextView)
    if (mW2TipTextViewPtr) {
        mW2TipTextViewPtr->setText(text);
    }
#elif defined(ID_MAIN_W2TipText)
    if (mW2TipTextPtr) {
        mW2TipTextPtr->setText(text);
    }
#elif defined(ID_MAIN_W2Tip_TextView)
    if (mW2Tip_TextViewPtr) {
        mW2Tip_TextViewPtr->setText(text);
    }
#else
    (void)text;
#endif
}

static bool showW2TipText(const char* text) {
    setW2TipText(text);
#if defined(ID_MAIN_W2TipWindow)
    if (mW2TipWindowPtr) {
        mW2TipWindowPtr->showWnd();
        sW2TipWindowVisible = true;
        sW2TipShownAtMs = getW2CurrentTimeMs();
        return true;
    }
#endif
    return false;
}

static void hideW2TipWindowOnly() {
#if defined(ID_MAIN_W2TipWindow)
    if (mW2TipWindowPtr) {
        mW2TipWindowPtr->hideWnd();
    }
#endif
    sW2TipWindowVisible = false;
    sW2TipShownAtMs = 0;
}

static bool hideW2TipWindowIfVisible() {
    if (!sW2TipWindowVisible) {
        return false;
    }
    if (getW2CurrentTimeMs() - sW2TipShownAtMs < 250LL) {
        return true;
    }
    hideW2TipWindowOnly();
    return true;
}

static void showDeviceListEmptyRow() {
    if (mDeviceListViewPtr) {
        const int rowCount = DeviceDataStore::getDeviceListRowCount();
        if (rowCount > 0) {
            mDeviceListViewPtr->setSelection(rowCount - 1);
        }
    }
}

static void showChangeIrrListEmptyRow() {
    if (mChangeIrr_ListViewPtr) {
        mChangeIrr_ListViewPtr->setSelection(static_cast<int>(sIrrGroupNumbers.size()));
    }
}

static void updateClearIrrButtonText() {
    if (mIrrNumValue_TextViewPtr) {
        char backgroundPic[64] = {0};
        if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            snprintf(backgroundPic, sizeof(backgroundPic),
                    "w2_set_irr_value_%03d.png", sSelectedIrrGroupNo);
        } else {
            snprintf(backgroundPic, sizeof(backgroundPic),
                    "w2_set_irr_value104_none.png");
        }
        mIrrNumValue_TextViewPtr->setText("");
        mIrrNumValue_TextViewPtr->setBackgroundPic(backgroundPic);
    }
}

static bool isValidIrrGroupNo(int groupNo) {
    return groupNo > 0 && groupNo <= 128;
}

static bool isDeviceBoundToIrrGroup(const SDATA* data, int groupNo) {
    if (!data || !isValidIrrGroupNo(groupNo)) {
        return false;
    }
    if (std::strcmp(data->arre, "*") == 0) {
        return hasBoundValveInIrrGroup(groupNo);
    }
    return DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo);
}

static const char* getIrrGroupName(int groupNo) {
    return isValidIrrGroupNo(groupNo) ? DeviceDataStore::getIrrGroupName(groupNo) : "";
#if 0
    static char nameText[32] = {0};
    if (isValidIrrGroupNo(groupNo)) {
        snprintf(nameText, sizeof(nameText), "阀组[%d]", groupNo);
    } else {
        nameText[0] = '\0';
    }
    return nameText;
#endif
}

static void appendTextPart(char* text, size_t size, const char* part, const char* separator) {
    if (!text || size == 0 || !part || part[0] == '\0') {
        return;
    }

    const bool hasText = text[0] != '\0';
    snprintf(text + strlen(text), size - strlen(text), "%s%s", hasText ? separator : "", part);
}

static bool collectUngroupedValveAddresses(char* text, size_t size) {
    if (!text || size == 0) {
        return false;
    }

    text[0] = '\0';
    std::vector<int> addresses;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) != 0) {
            continue;
        }

        if (isValidIrrGroupNo(atoi(data->arre))) {
            continue;
        }

        addresses.push_back(data->address);
    }

    if (addresses.empty()) {
        return false;
    }

    std::sort(addresses.begin(), addresses.end());
    addresses.erase(std::unique(addresses.begin(), addresses.end()), addresses.end());

    int lineLength = 0;
    int lineLimit = 24;
    for (size_t i = 0; i < addresses.size();) {
        const int first = addresses[i];
        int last = first;
        while (i + 1 < addresses.size() && addresses[i + 1] == last + 1) {
            last = addresses[++i];
        }

        char rangeText[32] = {0};
        if (first == last) {
            snprintf(rangeText, sizeof(rangeText), "%d", first);
        } else {
            snprintf(rangeText, sizeof(rangeText), "%d-%d", first, last);
        }

        const int separatorLength = text[0] == '\0' ? 0 : 2;
        const int rangeLength = static_cast<int>(strlen(rangeText));
        const bool wrapLine = lineLength > 0
                && lineLength + separatorLength + rangeLength > lineLimit;
        appendTextPart(text, size, rangeText, wrapLine ? ",\n" : ", ");
        lineLength = wrapLine ? rangeLength : lineLength + separatorLength + rangeLength;
        if (wrapLine) {
            lineLimit = 34;
        }
        ++i;
    }

    return true;
}

static bool showW2UngroupedValveTipIfNeeded() {
    char addresses[1024] = {0};
    if (!collectUngroupedValveAddresses(addresses, sizeof(addresses))) {
        return false;
    }

    char tipText[1152] = {0};
    snprintf(tipText, sizeof(tipText), "电磁阀[%s]未添加到阀组", addresses);
    return showW2TipText(tipText);
}

static bool handlePage2BeforeMainPageSwitch(int targetPageIndex) {
    if (targetPageIndex == BACK_GROUND_BTN_2) {
        return true;
    }

    if (hideW2TipWindowIfVisible()) {
        return false;
    }

    if (sW2TipCheckedThisVisit) {
        return true;
    }

    sW2TipCheckedThisVisit = true;
    return !showW2UngroupedValveTipIfNeeded();
}

static bool isW2PreviewValveForGroup(int groupNo) {
    return sW2SetWindowOpen
            && isValidIrrGroupNo(groupNo)
            && groupNo == sSelectedIrrGroupNo
            && sW2SelectedTypeIndex == 0;
}

static int getW2PreviewAddress() {
    if (sW2CurrentAddress > 0) {
        return sW2CurrentAddress;
    }
    return getW2DefaultAddress();
}

static void updateW2AddressDisplay(int address) {
    if (!mTextView1Ptr || address <= 0 || address > 255) {
        return;
    }

    char addressPic[64] = {0};
    snprintf(addressPic, sizeof(addressPic), "w2_set_address_combined_%03d.png", address);
    sW2CurrentAddress = address;
    mTextView1Ptr->setBackgroundPic(addressPic);
}

static void buildIrrGroupDisplayText(int groupNo, char* text, size_t size) {
    if (!text || size == 0) {
        return;
    }

    text[0] = '\0';
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }

    std::vector<int> valveAddresses;
    char waterPumpAddresses[64] = {0};
    char sensorNames[160] = {0};
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (sW2SetWindowOpen && i == sW2EditingIndex && DeviceDataStore::isCustomDevice(i)) {
            continue;
        }
        if (!isDeviceBoundToIrrGroup(data, groupNo)) {
            continue;
        }

        if (std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0) {
            valveAddresses.push_back(data->address);
            continue;
        }

        if (std::strcmp(data->type, "水泵") == 0) {
            char addressText[16] = {0};
            snprintf(addressText, sizeof(addressText), "%d", data->address);
            appendTextPart(waterPumpAddresses, sizeof(waterPumpAddresses), addressText, "，");
            continue;
        }

        if (!isSensorTextDevice(data)) {
            continue;
        }

        char shortName[32] = {0};
        snprintf(shortName, sizeof(shortName), "%s", data->name);
        char* suffix = strstr(shortName, "传感器");
        if (suffix && suffix[9] == '\0') {
            *suffix = '\0';
        }
        appendTextPart(sensorNames, sizeof(sensorNames), shortName, " ");
    }

    if (isW2PreviewValveForGroup(groupNo)) {
        valveAddresses.push_back(getW2PreviewAddress());
    }

    char summary[256] = {0};
    if (!valveAddresses.empty()) {
        std::sort(valveAddresses.begin(), valveAddresses.end());
        valveAddresses.erase(
                std::unique(valveAddresses.begin(), valveAddresses.end()),
                valveAddresses.end());
        char addresses[96] = {0};
        for (size_t i = 0; i < valveAddresses.size(); ++i) {
            char addressText[16] = {0};
            snprintf(addressText, sizeof(addressText), "%d", valveAddresses[i]);
            appendTextPart(addresses, sizeof(addresses), addressText, ", ");
        }
        char valveText[160] = {0};
        snprintf(valveText, sizeof(valveText), "%s[%s]", W2_DEVICE_TYPE_VALVE, addresses);
        appendTextPart(summary, sizeof(summary), valveText, "");
    }

    char pumpSensorText[224] = {0};
    if (waterPumpAddresses[0] != '\0') {
        snprintf(pumpSensorText, sizeof(pumpSensorText), "水泵%s", waterPumpAddresses);
    }
    if (sensorNames[0] != '\0') {
        appendTextPart(pumpSensorText, sizeof(pumpSensorText), sensorNames, " ");
    }
    if (pumpSensorText[0] != '\0') {
        char pumpSensorBracket[240] = {0};
        snprintf(pumpSensorBracket, sizeof(pumpSensorBracket), "[%s]", pumpSensorText);
        appendTextPart(summary, sizeof(summary), pumpSensorBracket, "");
    }

    if (summary[0] != '\0') {
        snprintf(text, size, "%s %s", getIrrGroupName(groupNo), summary);
    } else {
        snprintf(text, size, "%s     [空]", getIrrGroupName(groupNo));
    }
}

static void resetIrrGroupSelection() {
    sSelectedIrrGroupNo = -1;
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setText("");
        mGroupNameEditTextPtr->setTextColor(static_cast<int>(0x00FFFFFFU));
    }
    updateClearIrrButtonText();
    refreshChangeIrrListView();
}

static int getCurrentW2DeviceIndex() {
    if (!sW2AddingDevice && DeviceDataStore::isCustomDevice(sW2EditingIndex)) {
        return sW2EditingIndex;
    }
    if (sW2CurrentAddress <= 0) {
        return -1;
    }
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && data->address == sW2CurrentAddress) {
            return i;
        }
    }
    return -1;
}

static bool isCurrentW2DeviceInIrrGroup(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return false;
    }
    // The editor has a live preview: a valve already previewed in the selected
    // group must open Window11 even before the device is committed.
    if (isW2PreviewValveForGroup(groupNo)) {
        return true;
    }
    if (sW2AddingDevice) {
        return sSelectedIrrGroupNo == groupNo;
    }
    const int index = getCurrentW2DeviceIndex();
    const SDATA* data = DeviceDataStore::getDevice(index);
    return data && isDeviceBoundToIrrGroup(data, groupNo);
}

static void selectIrrGroup(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }
    if (isCurrentW2DeviceInIrrGroup(groupNo)) {
        sSelectedIrrGroupNo = groupNo;
        sW2EditorPinnedGroupNo = groupNo;
        updateClearIrrButtonText();
        refreshChangeIrrListView();
        openW2Window11(groupNo);
        return;
    }
    if (!canAssignCurrentW2ValveToIrrGroup(groupNo, true)) {
        return;
    }

    if (sW2TransferMode) {
        const int sourceGroupNo = sW2TransferSourceGroupNo;
        if (groupNo == sourceGroupNo) {
            openW2Window11(sourceGroupNo);
            return;
        }
        const int index = getCurrentW2DeviceIndex();
        bool changed = false;
        if (sW2AddingDevice) {
            changed = true;
        } else if (index >= 0) {
            // Bind first: valves move directly, while multi-group devices are
            // added to the target before the source relation is removed.
            changed = DeviceDataStore::bindDeviceToIrrGroup(index, groupNo);
            changed = DeviceDataStore::unbindDeviceFromIrrGroup(index, sourceGroupNo) || changed;
        }
        if (changed) {
            ensureIrrGroupNumberVisible(groupNo);
            sW2EditorPinnedGroupNo = groupNo;
            sSelectedIrrGroupNo = groupNo;
            refreshChangeIrrListView();
        }
        sW2TransferMode = false;
        sW2TransferSourceGroupNo = -1;
        hideW2TipWindowOnly();
        return;
    }

    // A normal group-row click is an edit operation for the current valve,
    // not merely a visual selection. Bind the current address immediately so
    // the list becomes a live preview (Cancel restores the editor snapshot;
    // Confirm commits it). For a single-group valve this also moves it from
    // its previous group to the clicked group.
    if (!sW2AddingDevice && sW2SelectedTypeIndex == 0) {
        const int index = getCurrentW2DeviceIndex();
        if (index >= 0) {
            DeviceDataStore::bindDeviceToIrrGroup(index, groupNo);
        }
    }

    sSelectedIrrGroupNo = groupNo;
    sW2EditorPinnedGroupNo = groupNo;
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setText("");
    }
    updateClearIrrButtonText();
    // Keep this callback single-pass; save/cancel performs the full table refresh.
    refreshChangeIrrListView();
}

static void renameSelectedIrrGroup(const std::string &name) {
    if (!isValidIrrGroupNo(sSelectedIrrGroupNo) || name.empty()) {
        return;
    }
    if (DeviceDataStore::setIrrGroupName(sSelectedIrrGroupNo, name.c_str())) {
        refreshChangeIrrListView();
    }
}

static void closeGroupRenameWindow() {
    if (mGroupRenameWindowPtr) {
        mGroupRenameWindowPtr->hideWnd();
    }
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setText("");
    }
    sGroupRenameWindowVisible = false;
}

static void openGroupRenameWindow() {
    if (!requireSelectedIrrGroup()) {
        return;
    }
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setTextColor(static_cast<int>(0xFF005BBBU));
        mGroupNameEditTextPtr->setText(getIrrGroupName(sSelectedIrrGroupNo));
    }
    if (mGroupRenameWindowPtr) {
        mGroupRenameWindowPtr->showWnd();
        sGroupRenameWindowVisible = true;
    }
}

static void saveGroupRenameWindow() {
    if (sGroupRenameWindowVisible && mGroupNameEditTextPtr) {
        const std::string name = mGroupNameEditTextPtr->getText();
        if (!name.empty()) {
            renameSelectedIrrGroup(name);
        }
    }
    closeGroupRenameWindow();
}

static int normalizeIrrCapacity(int capacity) {
    if (capacity < 1) {
        return 1;
    }
    if (capacity > 3) {
        return 3;
    }
    return capacity;
}

static int getIrrGroupCapacity(int groupNo) {
    if (isValidIrrGroupNo(groupNo) && sIrrGroupCapacity[groupNo] >= 1
            && sIrrGroupCapacity[groupNo] <= 3) {
        return sIrrGroupCapacity[groupNo];
    }
    return normalizeIrrCapacity(sIrrCapacityLast);
}

static bool compareValveDeviceIndexesByAddress(int left, int right) {
    const SDATA* leftData = DeviceDataStore::getDevice(left);
    const SDATA* rightData = DeviceDataStore::getDevice(right);
    if (!leftData || !rightData) {
        return left < right;
    }
    if (leftData->address != rightData->address) {
        return leftData->address < rightData->address;
    }
    return left < right;
}

static void ensureIrrGroupCapacityDefaults(int fallback) {
    fallback = normalizeIrrCapacity(fallback);
    for (int groupNo = 1; groupNo <= 128; ++groupNo) {
        if (sIrrGroupCapacity[groupNo] < 1 || sIrrGroupCapacity[groupNo] > 3) {
            sIrrGroupCapacity[groupNo] = fallback;
        }
    }
}

static void styleIrrCapacityText(ZKTextView* textView, bool bold) {
    if (!textView) {
        return;
    }
    textView->setTextColor(static_cast<int>(0xFF005BBBU));
    textView->setFontFamily("Alibaba-PuHuiTi-Regular");
    textView->setAlignment(ZKTextView::E_ALIGN_H_CENTER, ZKTextView::E_ALIGN_V_CENTER);
    (void)bold;  // ZKTextView in this firmware SDK has no setBold symbol.
}

static void styleIrrCapacityValueText(ZKTextView* textView) {
    if (!textView) {
        return;
    }
    // Match the baked 1/2/3 capacity buttons: #0051C6, 42 px, centered.
    textView->setTextColor(static_cast<int>(0xFF0051C6U));
    textView->setFontFamily("Alibaba-PuHuiTi-Regular");
    textView->setAlignment(ZKTextView::E_ALIGN_H_CENTER, ZKTextView::E_ALIGN_V_CENTER);
}

static void updateIrrCapacitySelection() {
    const bool separateMode = sIrrCapacityMode == 0;
    const int groupNo = isValidIrrGroupNo(sIrrCapacityGroupNo)
            ? sIrrCapacityGroupNo : 1;
    const int capacity = separateMode
            ? getIrrGroupCapacity(groupNo)
            : normalizeIrrCapacity(sIrrCapacityPending);
    char valueText[16] = {0};
    styleIrrCapacityText(mIrrCapacityModeTitleTextPtr, true);
    styleIrrCapacityText(mIrrCapacitySeparateModeLabelTextPtr, true);
    styleIrrCapacityText(mIrrCapacityUniformModeLabelTextPtr, true);
    styleIrrCapacityText(mIrrCapacityGroupLabelTextPtr, true);
    styleIrrCapacityText(mIrrCapacityValueLabelTextPtr, true);
    styleIrrCapacityValueText(mIrrCapacityGroupValueButtonPtr);
    styleIrrCapacityValueText(mIrrCapacityCapacityValueButtonPtr);
    if (mIrrCapacitySeparateModeButtonPtr) {
        mIrrCapacitySeparateModeButtonPtr->setSelected(separateMode);
    }
    if (mIrrCapacityUniformModeButtonPtr) {
        mIrrCapacityUniformModeButtonPtr->setSelected(!separateMode);
    }
    // The two modes show different editing scopes.  While the uniform mode
    // is active, the left-side per-group values are intentionally masked with
    // a dash; switching back restores the current group number and capacity.
    if (mIrrCapacityGroupValueButtonPtr) {
        if (separateMode) {
            snprintf(valueText, sizeof(valueText), "%d", groupNo);
            mIrrCapacityGroupValueButtonPtr->setText(valueText);
        } else {
            mIrrCapacityGroupValueButtonPtr->setText("-");
        }
    }
    if (mIrrCapacityCapacityValueButtonPtr) {
        if (separateMode) {
            snprintf(valueText, sizeof(valueText), "%d", capacity);
            mIrrCapacityCapacityValueButtonPtr->setText(valueText);
        } else {
            mIrrCapacityCapacityValueButtonPtr->setText("-");
        }
    }
    // Capacity buttons are meaningful only for all-groups mode.  Clear their
    // selected state in single-group mode so the green frame disappears.
    if (mIrrCapacity1ButtonPtr) {
        mIrrCapacity1ButtonPtr->setSelected(!separateMode && capacity == 1);
    }
    if (mIrrCapacity2ButtonPtr) {
        mIrrCapacity2ButtonPtr->setSelected(!separateMode && capacity == 2);
    }
    if (mIrrCapacity3ButtonPtr) {
        mIrrCapacity3ButtonPtr->setSelected(!separateMode && capacity == 3);
    }
    // The left controls belong to single-group mode; the three capacity
    // choices belong to all-groups mode. Keep the inactive side touch-safe.
    if (mIrrCapacityGroupPrevButtonPtr) {
        mIrrCapacityGroupPrevButtonPtr->setTouchable(separateMode);
    }
    if (mIrrCapacityGroupNextButtonPtr) {
        mIrrCapacityGroupNextButtonPtr->setTouchable(separateMode);
    }
    if (mIrrCapacityGroupValueButtonPtr) {
        mIrrCapacityGroupValueButtonPtr->setTouchable(separateMode);
    }
    if (mIrrCapacityValuePrevButtonPtr) {
        mIrrCapacityValuePrevButtonPtr->setTouchable(true);
    }
    if (mIrrCapacityValueNextButtonPtr) {
        mIrrCapacityValueNextButtonPtr->setTouchable(true);
    }
    if (mIrrCapacity1ButtonPtr) {
        mIrrCapacity1ButtonPtr->setTouchable(!separateMode);
    }
    if (mIrrCapacity2ButtonPtr) {
        mIrrCapacity2ButtonPtr->setTouchable(!separateMode);
    }
    if (mIrrCapacity3ButtonPtr) {
        mIrrCapacity3ButtonPtr->setTouchable(!separateMode);
    }
}

static void selectIrrCapacity(int capacity) {
    if (capacity < 1 || capacity > 3) {
        return;
    }
    sIrrCapacityPending = capacity;
    if (sIrrCapacityMode == 0) {
        sIrrGroupCapacity[isValidIrrGroupNo(sIrrCapacityGroupNo)
                ? sIrrCapacityGroupNo : 1] = capacity;
    }
    updateIrrCapacitySelection();
}

static void setIrrCapacityMode(int mode) {
    if (mode != 0 && mode != 1) {
        return;
    }
    sIrrCapacityMode = mode;
    if (mode == 1) {
        sIrrCapacityPending = normalizeIrrCapacity(sIrrCapacityLast);
    } else {
        sIrrCapacityPending = getIrrGroupCapacity(sIrrCapacityGroupNo);
    }
    updateIrrCapacitySelection();
}

static void stepIrrCapacityGroup(int delta) {
    if (sIrrCapacityMode != 0) {
        return;
    }
    int groupNo = sIrrCapacityGroupNo + delta;
    if (groupNo < 1) {
        groupNo = 1;
    }
    if (groupNo > 128) {
        groupNo = 128;
    }
    sIrrCapacityGroupNo = groupNo;
    sIrrCapacityPending = getIrrGroupCapacity(groupNo);
    updateIrrCapacitySelection();
}

static void stepIrrCapacityValue(int delta) {
    int capacity = sIrrCapacityMode == 0
            ? getIrrGroupCapacity(sIrrCapacityGroupNo)
            : normalizeIrrCapacity(sIrrCapacityPending);
    capacity = normalizeIrrCapacity(capacity + delta);
    selectIrrCapacity(capacity);
}

static void beginW2EditorSession() {
    if (sW2EditorSessionActive) {
        return;
    }

    sW2EditorDeviceSnapshot = w2_DeviceDataList;
    sW2EditorGroupSnapshot = sIrrGroupNumbers;
    sW2EditorGroupNameSnapshot.clear();
    sW2EditorGroupNameSnapshot.reserve(128);
    for (int groupNo = 1; groupNo <= 128; ++groupNo) {
        sW2EditorGroupNameSnapshot.push_back(
                DeviceDataStore::getIrrGroupName(groupNo));
    }
    sW2EditorOriginalCapacityPending = sIrrCapacityPending;
    sW2EditorOriginalCapacityLast = sIrrCapacityLast;
    sW2EditorOriginalCapacityMode = sIrrCapacityMode;
    sW2EditorOriginalCapacityGroupNo = sIrrCapacityGroupNo;
    sW2EditorOriginalValveLimit = sIrrGroupValveLimit;
    for (int groupNo = 0; groupNo <= 128; ++groupNo) {
        sW2EditorOriginalGroupCapacity[groupNo] = sIrrGroupCapacity[groupNo];
    }
    sW2EditorSessionActive = true;
}

static void restoreW2EditorSession() {
    if (!sW2EditorSessionActive) {
        return;
    }

    // Restore only the data here. List refreshes are intentionally deferred
    // until every Window2 child has been hidden and the page state is closed.
    // Refreshing an FTU ListView while its editor window is still visible can
    // re-enter row callbacks on the board and leave the UI event loop stuck.
    w2_DeviceDataList = sW2EditorDeviceSnapshot;
    sIrrGroupNumbers = sW2EditorGroupSnapshot;
    for (int groupNo = 1; groupNo <= 128; ++groupNo) {
        const size_t index = static_cast<size_t>(groupNo - 1);
        if (index < sW2EditorGroupNameSnapshot.size()) {
            DeviceDataStore::setIrrGroupName(
                    groupNo, sW2EditorGroupNameSnapshot[index].c_str());
        }
    }
    sIrrCapacityPending = sW2EditorOriginalCapacityPending;
    sIrrCapacityLast = sW2EditorOriginalCapacityLast;
    sIrrCapacityMode = sW2EditorOriginalCapacityMode;
    sIrrCapacityGroupNo = sW2EditorOriginalCapacityGroupNo;
    sIrrGroupValveLimit = sW2EditorOriginalValveLimit;
    for (int groupNo = 0; groupNo <= 128; ++groupNo) {
        sIrrGroupCapacity[groupNo] = sW2EditorOriginalGroupCapacity[groupNo];
    }

    sW2EditorSessionActive = false;
    sW2EditorDeviceSnapshot.clear();
    sW2EditorGroupSnapshot.clear();
    sW2EditorGroupNameSnapshot.clear();
}

static void commitW2EditorSession() {
    if (!sW2EditorSessionActive) {
        return;
    }
    sW2EditorSessionActive = false;
    sW2EditorDeviceSnapshot.clear();
    sW2EditorGroupSnapshot.clear();
    sW2EditorGroupNameSnapshot.clear();
}

static void restoreW2AutoAssignSnapshot() {
    w2_DeviceDataList = sW2AutoAssignDeviceSnapshot;
    sIrrGroupNumbers = sW2AutoAssignGroupSnapshot;
    sW2AddingDevice = sW2AutoAssignOriginalAddingDevice;
    sW2EditingIndex = sW2AutoAssignOriginalEditingIndex;
    sSelectedIrrGroupNo = sW2AutoAssignOriginalSelectedGroupNo;
    sIrrGroupValveLimit = sW2AutoAssignOriginalValveLimit;
}

static void beginW2AutoAssignPreview() {
    if (sW2AutoAssignPreviewPending) {
        restoreW2AutoAssignSnapshot();
        return;
    }

    sW2AutoAssignDeviceSnapshot = w2_DeviceDataList;
    sW2AutoAssignGroupSnapshot = sIrrGroupNumbers;
    sW2AutoAssignOriginalAddingDevice = sW2AddingDevice;
    sW2AutoAssignOriginalEditingIndex = sW2EditingIndex;
    sW2AutoAssignOriginalSelectedGroupNo = sSelectedIrrGroupNo;
    sW2AutoAssignOriginalValveLimit = sIrrGroupValveLimit;
    sW2AutoAssignPreviewPending = true;
}

static void finishW2AutoAssignPreview(bool commit) {
    if (!sW2AutoAssignPreviewPending) {
        return;
    }
    if (!commit) {
        // As with the editor snapshot, defer all ListView refreshes until the
        // complete Window2 close/rollback sequence has finished.
        restoreW2AutoAssignSnapshot();
    }
    sW2AutoAssignPreviewPending = false;
    sW2AutoAssignDeviceSnapshot.clear();
    sW2AutoAssignGroupSnapshot.clear();
}

static void openIrrCapacityWindow() {
    if (sIrrCapacityWindowVisible) {
        return;
    }
    sIrrCapacityDialogSnapshotValid = true;
    sIrrCapacityDialogOriginalMode = sIrrCapacityMode;
    sIrrCapacityDialogOriginalGroupNo = sIrrCapacityGroupNo;
    sIrrCapacityDialogOriginalPending = sIrrCapacityPending;
    sIrrCapacityDialogOriginalLast = sIrrCapacityLast;
    sIrrCapacityDialogOriginalValveLimit = sIrrGroupValveLimit;
    for (int groupNo = 0; groupNo <= 128; ++groupNo) {
        sIrrCapacityDialogOriginalGroupCapacity[groupNo] = sIrrGroupCapacity[groupNo];
    }
    ensureIrrGroupCapacityDefaults(sIrrCapacityLast);
    if (sIrrCapacityMode == 1) {
        sIrrCapacityPending = normalizeIrrCapacity(sIrrCapacityLast);
    } else {
        sIrrCapacityPending = getIrrGroupCapacity(sIrrCapacityGroupNo);
    }
    updateIrrCapacitySelection();
    if (mIrrCapacityWindowPtr) {
        mIrrCapacityWindowPtr->showWnd();
        sIrrCapacityWindowVisible = true;
    }
}

static int getIrrGroupValveCount(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
                && DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            ++count;
        }
    }
    return count;
}

static bool isCurrentW2ValveInIrrGroup(int groupNo) {
    if (!isValidIrrGroupNo(groupNo) || sW2SelectedTypeIndex != 0) {
        return false;
    }

    const SDATA* data = DeviceDataStore::getDevice(sW2EditingIndex);
    if (data && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
            && DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
        return true;
    }

    if (sW2CurrentAddress <= 0) {
        return false;
    }
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        data = DeviceDataStore::getDevice(i);
        if (data && data->address == sW2CurrentAddress
                && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
                && DeviceDataStore::isDeviceBoundToIrrGroup(data, groupNo)) {
            return true;
        }
    }
    return false;
}

static bool canAssignCurrentW2ValveToIrrGroup(int groupNo, bool showTip) {
    if (!isValidIrrGroupNo(groupNo) || sW2SelectedTypeIndex != 0
            || isCurrentW2ValveInIrrGroup(groupNo)
            || getIrrGroupValveCount(groupNo) < getIrrGroupCapacity(groupNo)) {
        return true;
    }

    if (showTip) {
        char tipText[96] = {0};
        snprintf(tipText, sizeof(tipText),
                "该阀组最多可添加%d个电磁阀", getIrrGroupCapacity(groupNo));
        showW2TipText(tipText);
    }
    return false;
}

static int saveCurrentW2ValveForAutoAssign() {
    if (!sW2SetWindowOpen || sW2SelectedTypeIndex != 0
            || sW2CurrentAddress <= 0) {
        return -1;
    }

    std::string nameText;
    if (mW2_NameEditTextPtr) {
        nameText = mW2_NameEditTextPtr->getText();
    }

    if (sW2AddingDevice) {
        if (!DeviceDataStore::addDevice(sW2CurrentAddress, nameText.c_str(),
                W2_DEVICE_TYPE_VALVE)) {
            return -1;
        }
        sW2EditingIndex = DeviceDataStore::getDeviceCount() - 1;
        sW2AddingDevice = false;
        return sW2EditingIndex;
    }

    if (!DeviceDataStore::isCustomDevice(sW2EditingIndex)) {
        return -1;
    }
    if (!DeviceDataStore::updateDevice(sW2EditingIndex, sW2CurrentAddress,
            nameText.c_str(), W2_DEVICE_TYPE_VALVE)) {
        return -1;
    }
    return sW2EditingIndex;
}

static void autoAssignUngroupedValves(int capacity) {
    if (capacity < 1 || capacity > 3) {
        return;
    }

    const int currentValveIndex = saveCurrentW2ValveForAutoAssign();
    std::vector<int> ungrouped;
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
                && !isValidIrrGroupNo(atoi(data->arre))) {
            ungrouped.push_back(i);
        }
    }
    for (size_t i = 1; i < ungrouped.size(); ++i) {
        const int value = ungrouped[i];
        size_t j = i;
        while (j > 0 && compareValveDeviceIndexesByAddress(value, ungrouped[j - 1])) {
            ungrouped[j] = ungrouped[j - 1];
            --j;
        }
        ungrouped[j] = value;
    }
    std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
    size_t valveIndex = 0;
    int firstCreatedGroupNo = -1;
    for (size_t i = 0; i < sIrrGroupNumbers.size() && valveIndex < ungrouped.size(); ++i) {
        const int groupNo = sIrrGroupNumbers[i];
        const int groupCapacity = getIrrGroupCapacity(groupNo);
        int currentCount = getIrrGroupValveCount(groupNo);
        while (currentCount < groupCapacity && valveIndex < ungrouped.size()) {
            DeviceDataStore::bindDeviceToIrrGroup(ungrouped[valveIndex], groupNo);
            ++valveIndex;
            ++currentCount;
        }
    }

    while (valveIndex < ungrouped.size()) {
        int groupNo = 1;
        while (groupNo <= 128 && std::find(sIrrGroupNumbers.begin(),
                sIrrGroupNumbers.end(), groupNo) != sIrrGroupNumbers.end()) {
            ++groupNo;
        }
        if (groupNo > 128) {
            break;
        }
        if (firstCreatedGroupNo < 0) {
            firstCreatedGroupNo = groupNo;
        }
        sIrrGroupNumbers.push_back(groupNo);
        const int groupCapacity = getIrrGroupCapacity(groupNo);
        for (int count = 0; count < groupCapacity && valveIndex < ungrouped.size(); ++count) {
            DeviceDataStore::bindDeviceToIrrGroup(ungrouped[valveIndex], groupNo);
            ++valveIndex;
        }
    }

    std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
    int currentGroupNo = -1;
    const SDATA* currentValve = DeviceDataStore::getDevice(currentValveIndex);
    if (currentValve) {
        const int groupNo = atoi(currentValve->arre);
        if (isValidIrrGroupNo(groupNo)) {
            currentGroupNo = groupNo;
            sSelectedIrrGroupNo = groupNo;
            updateClearIrrButtonText();
        }
    }
    refreshDeviceListViews();
    refreshChangeIrrListView();
    const int selectionGroupNo = currentGroupNo > 0
            ? currentGroupNo : firstCreatedGroupNo;
    if (selectionGroupNo > 0 && mChangeIrr_ListViewPtr) {
        const std::vector<int>::const_iterator it = std::find(
                sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), selectionGroupNo);
        if (it != sIrrGroupNumbers.end()) {
            mChangeIrr_ListViewPtr->setSelection(
                    static_cast<int>(it - sIrrGroupNumbers.begin()));
        }
    }
}

static bool autoAssignPage2ValvesFromTuyaMode(int mode, int groupNo, int capacity) {
    if (capacity < 1 || capacity > 3) {
        return false;
    }
    if (mode == 0 && !isValidIrrGroupNo(groupNo)) {
        return false;
    }

    ensureIrrGroupCapacityDefaults(sIrrCapacityLast);
    if (mode == 1) {
        sIrrCapacityMode = 1;
        sIrrCapacityLast = normalizeIrrCapacity(capacity);
        for (int number = 1; number <= 128; ++number) {
            sIrrGroupCapacity[number] = sIrrCapacityLast;
        }
        sIrrGroupValveLimit = sIrrCapacityLast;
    } else {
        sIrrCapacityMode = 0;
        sIrrCapacityGroupNo = groupNo;
        sIrrCapacityPending = normalizeIrrCapacity(capacity);
        sIrrGroupCapacity[groupNo] = sIrrCapacityPending;
        sIrrGroupValveLimit = sIrrCapacityPending;
    }

    std::vector<int> ungrouped;
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0
                && !isValidIrrGroupNo(atoi(data->arre))) {
            ungrouped.push_back(i);
        }
    }
    for (size_t i = 1; i < ungrouped.size(); ++i) {
        const int value = ungrouped[i];
        size_t j = i;
        while (j > 0 && compareValveDeviceIndexesByAddress(value, ungrouped[j - 1])) {
            ungrouped[j] = ungrouped[j - 1];
            --j;
        }
        ungrouped[j] = value;
    }

    std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
    size_t valveIndex = 0;
    for (size_t i = 0; i < sIrrGroupNumbers.size() && valveIndex < ungrouped.size(); ++i) {
        const int number = sIrrGroupNumbers[i];
        const int groupCapacity = getIrrGroupCapacity(number);
        int currentCount = getIrrGroupValveCount(number);
        while (currentCount < groupCapacity && valveIndex < ungrouped.size()) {
            DeviceDataStore::bindDeviceToIrrGroup(ungrouped[valveIndex], number);
            ++valveIndex;
            ++currentCount;
        }
    }

    while (valveIndex < ungrouped.size()) {
        int number = 1;
        while (number <= 128 && std::find(sIrrGroupNumbers.begin(),
                sIrrGroupNumbers.end(), number) != sIrrGroupNumbers.end()) {
            ++number;
        }
        if (number > 128) {
            break;
        }
        sIrrGroupNumbers.push_back(number);
        const int groupCapacity = getIrrGroupCapacity(number);
        for (int count = 0; count < groupCapacity && valveIndex < ungrouped.size(); ++count) {
            DeviceDataStore::bindDeviceToIrrGroup(ungrouped[valveIndex], number);
            ++valveIndex;
        }
    }

    std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
    refreshDeviceListViews();
    refreshChangeIrrListView();

    const unsigned int session =
        static_cast<unsigned int>(getW2CurrentTimeMs()) & 0xFFFFU;
    const bool queued = writePage2TuyaSyncQueue(session, 0, 0);
    LOGD("[Page2DeviceSync] auto assign mode=%d group=%d capacity=%d assigned=%u queued=%d\n",
         mode, groupNo, capacity, static_cast<UINT>(valveIndex), queued ? 1 : 0);
    return valveIndex > 0 || queued;
}

bool bindPage2DevicesFromTuya(int groupCode, const std::vector<int>& addresses) {
    if (!((groupCode >= 1 && groupCode <= 128) || groupCode == 0xFF) ||
            addresses.empty()) {
        return false;
    }

    bool found = false;
    bool changed = false;
    for (size_t addressIndex = 0; addressIndex < addresses.size(); ++addressIndex) {
        const int address = addresses[addressIndex];
        for (int deviceIndex = 0; deviceIndex < DeviceDataStore::getDeviceCount(); ++deviceIndex) {
            const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
            if (!data || data->address != address) {
                continue;
            }
            found = true;
            if (groupCode == 0xFF) {
                changed = DeviceDataStore::bindDeviceToAllIrrGroups(deviceIndex) || changed;
            } else {
                // Match the board editor: multi-group pumps/sensors keep their
                // other memberships while a valve moves to the selected group.
                changed = DeviceDataStore::bindDeviceToIrrGroup(deviceIndex, groupCode) || changed;
                ensureIrrGroupNumberVisible(groupCode);
            }
            break;
        }
    }

    if (!found) {
        return false;
    }
    if (changed) {
        refreshDeviceListViews();
        refreshChangeIrrListView();
    }

    const unsigned int session =
        static_cast<unsigned int>(getW2CurrentTimeMs()) & 0xFFFFU;
    writePage2TuyaSyncQueue(session, 0, 0);
    LOGD("[Page2DeviceSync] group bind group=%d addresses=%u changed=%d\\n",
         groupCode, static_cast<UINT>(addresses.size()), changed ? 1 : 0);
    return true;
}

bool upsertPage2DeviceFromTuya(int address, int decoderType,
                               const char* groupText, const char* name) {
    if (address < CUSTOM_DEVICE_START_ID || address > CUSTOM_DEVICE_END_ID ||
            (decoderType != DEVICE_DECODER_TYPE_VALVE &&
             decoderType != DEVICE_DECODER_TYPE_SENSOR) ||
            !name || name[0] == '\0') {
        return false;
    }

    const char* typeText = decoderType == DEVICE_DECODER_TYPE_VALVE ?
                           W2_DEVICE_TYPE_VALVE : W2_DEVICE_TYPE_SENSOR;
    int index = -1;
    for (int deviceIndex = 0; deviceIndex < DeviceDataStore::getDeviceCount(); ++deviceIndex) {
        const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
        if (data && data->address == address) {
            index = deviceIndex;
            break;
        }
    }

    bool changed = false;
    if (index >= 0) {
        if (!DeviceDataStore::isCustomDevice(index)) return false;
        changed = DeviceDataStore::updateDevice(index, address, name, typeText) || changed;
    } else {
        if (!DeviceDataStore::addDevice(address, name, typeText)) return false;
        index = DeviceDataStore::getDeviceCount() - 1;
        changed = true;
    }

    if (groupText && groupText[0] != '\0' && std::strcmp(groupText, "=") != 0) {
        changed = DeviceDataStore::setDeviceIrrGroupText(index, groupText) || changed;
        const SDATA* data = DeviceDataStore::getDevice(index);
        if (!data || (std::strcmp(groupText, "-") != 0 &&
                std::strcmp(groupText, "*") != 0 &&
                std::strcmp(data->arre, groupText) != 0)) {
            return false;
        }
    }

    refreshDeviceListViews();
    refreshChangeIrrListView();
    syncIrrGroupNumbersFromDeviceData();
    const unsigned int session =
        static_cast<unsigned int>(getW2CurrentTimeMs()) & 0xFFFFU;
    const bool queued = writePage2TuyaSyncQueue(session, 0, 0);
    LOGD("[Page2DeviceSync] device upsert address=%d type=%d changed=%d queued=%d\n",
         address, decoderType, changed ? 1 : 0, queued ? 1 : 0);
    return queued;
}

bool reportPage2DeviceTableFromTuya() {
    const unsigned int session =
        static_cast<unsigned int>(getW2CurrentTimeMs()) & 0xFFFFU;
    const bool queued = writePage2TuyaSyncQueue(session, 0, 0);
    LOGD("[Page2DeviceSync] current table report queued=%d\n", queued ? 1 : 0);
    return queued;
}

bool reportPage2DeviceTableVersionFromTuya(unsigned int expectedHash) {
    const unsigned int session =
        static_cast<unsigned int>(getW2CurrentTimeMs()) & 0xFFFFU;
    const bool queued = writePage2TuyaSyncQueue(session, 0, 0, true, expectedHash);
    LOGD("[Page2DeviceSync] version table report expected=%08X queued=%d\n",
         expectedHash, queued ? 1 : 0);
    return queued;
}

bool autoAssignPage2ValvesFromTuya(int capacity) {
    return autoAssignPage2ValvesFromTuyaMode(1, 0, capacity);
}

bool autoAssignPage2ValvesFromTuyaSingle(int groupNo, int capacity) {
    return autoAssignPage2ValvesFromTuyaMode(0, groupNo, capacity);
}

bool autoAssignPage2ValvesFromTuyaAll(int capacity) {
    return autoAssignPage2ValvesFromTuyaMode(1, 0, capacity);
}

static void closeIrrCapacityWindow(bool save) {
    const bool apply = save && sIrrCapacityWindowVisible;
    if (mIrrCapacityWindowPtr) {
        mIrrCapacityWindowPtr->hideWnd();
    }
    sIrrCapacityWindowVisible = false;
    if (apply) {
        const int capacity = normalizeIrrCapacity(sIrrCapacityPending);
        if (sIrrCapacityMode == 1) {
            sIrrCapacityLast = capacity;
            for (int groupNo = 1; groupNo <= 128; ++groupNo) {
                sIrrGroupCapacity[groupNo] = capacity;
            }
            sIrrGroupValveLimit = capacity;
        } else {
            sIrrCapacityGroupNo = isValidIrrGroupNo(sIrrCapacityGroupNo)
                    ? sIrrCapacityGroupNo : 1;
            sIrrGroupCapacity[sIrrCapacityGroupNo] = capacity;
            sIrrCapacityPending = capacity;
            sIrrGroupValveLimit = capacity;
        }
        beginW2AutoAssignPreview();
        autoAssignUngroupedValves(capacity);
    } else if (sIrrCapacityDialogSnapshotValid) {
        sIrrCapacityMode = sIrrCapacityDialogOriginalMode;
        sIrrCapacityGroupNo = sIrrCapacityDialogOriginalGroupNo;
        sIrrCapacityPending = sIrrCapacityDialogOriginalPending;
        sIrrCapacityLast = sIrrCapacityDialogOriginalLast;
        sIrrGroupValveLimit = sIrrCapacityDialogOriginalValveLimit;
        for (int groupNo = 0; groupNo <= 128; ++groupNo) {
            sIrrGroupCapacity[groupNo] = sIrrCapacityDialogOriginalGroupCapacity[groupNo];
        }
        updateIrrCapacitySelection();
    }
    sIrrCapacityDialogSnapshotValid = false;
}

static int getChangeIrrListItemCount() {
    return static_cast<int>(sIrrGroupNumbers.size()) + 1;
}

static bool isIrrGroupEmptyRow(int index) {
    return index < 0 || index >= getChangeIrrListItemCount()
            || index == static_cast<int>(sIrrGroupNumbers.size());
}

static void setIrrGroupSubItemTexts(ZKListView::ZKListItem *pListItem, int index, int numSubItemId, int nameSubItemId) {
    if (!pListItem) {
        return;
    }

    const bool selected = !isIrrGroupEmptyRow(index)
            && sSelectedIrrGroupNo == sIrrGroupNumbers[index];
    const uint32_t rowColor = selected ? 0xFF0E97E8U : 0xFFF6FBFFU;
    // Keep both the explicit color and the selected state. The FTU row is a
    // ZKButton-derived item, so the selected status color is the reliable
    // rendering path on the board even when the ListView background is an
    // opaque table image.
    pListItem->setBackgroundColor(rowColor);
    pListItem->setBgStatusColor(ZK_CONTROL_STATUS_NORMAL, rowColor);
    pListItem->setBgStatusColor(ZK_CONTROL_STATUS_PRESSED, rowColor);
    pListItem->setBgStatusColor(ZK_CONTROL_STATUS_SELECTED, rowColor);
    pListItem->setBgStatusColor(
            ZK_CONTROL_STATUS_PRESSED | ZK_CONTROL_STATUS_SELECTED, rowColor);
    // Do not call setSelected/setOpaque while the FTU adapter is supplying row data.
    // The row color is driven explicitly below to avoid re-entrant list refreshes.

    ZKListView::ZKListSubItem* numSubItem = pListItem->findSubItemByID(numSubItemId);
    ZKListView::ZKListSubItem* nameSubItem = pListItem ? pListItem->findSubItemByID(nameSubItemId) : NULL;
    if (numSubItem) {
        numSubItem->setBackgroundColor(rowColor);
        numSubItem->setBgStatusColor(ZK_CONTROL_STATUS_NORMAL, rowColor);
        numSubItem->setBgStatusColor(ZK_CONTROL_STATUS_PRESSED, rowColor);
        numSubItem->setBgStatusColor(ZK_CONTROL_STATUS_SELECTED, rowColor);
        numSubItem->setBgStatusColor(
                ZK_CONTROL_STATUS_PRESSED | ZK_CONTROL_STATUS_SELECTED, rowColor);
    }
    if (nameSubItem) {
        nameSubItem->setBackgroundColor(rowColor);
        nameSubItem->setBgStatusColor(ZK_CONTROL_STATUS_NORMAL, rowColor);
        nameSubItem->setBgStatusColor(ZK_CONTROL_STATUS_PRESSED, rowColor);
        nameSubItem->setBgStatusColor(ZK_CONTROL_STATUS_SELECTED, rowColor);
        nameSubItem->setBgStatusColor(
                ZK_CONTROL_STATUS_PRESSED | ZK_CONTROL_STATUS_SELECTED, rowColor);
    }
    if (!sIrrEmptyItemLayoutCaptured && numSubItem && nameSubItem) {
        sIrrNumSubItemPosition = numSubItem->getPosition();
        sIrrArrSubItemPosition = nameSubItem->getPosition();
        sIrrEmptyItemLayoutCaptured = true;
    }
    if (isIrrGroupEmptyRow(index)) {
        setListSubItemText(pListItem, numSubItemId, "");
        setListSubItemText(pListItem, nameSubItemId, "点击添加");
        setListSubItemAlignment(pListItem, nameSubItemId, ZKTextView::E_ALIGN_H_CENTER, ZKTextView::E_ALIGN_V_CENTER);
        if (numSubItem) {
        setListSubItemVisible(pListItem, numSubItemId, false);
        }
        if (nameSubItem) {
            LayoutPosition lp = sIrrEmptyItemLayoutCaptured ? sIrrArrSubItemPosition : nameSubItem->getPosition();
            lp.mLeft = 0;
            lp.mWidth = pListItem->getPosition().mWidth;
            setListSubItemPosition(pListItem, nameSubItemId, lp);
            setListSubItemVisible(pListItem, nameSubItemId, true);
        }
        return;
    }

    const int groupNo = sIrrGroupNumbers[index];
    char numText[16] = {0};
    char nameText[256] = {0};
    snprintf(numText, sizeof(numText), "%d", groupNo);
    buildIrrGroupDisplayText(groupNo, nameText, sizeof(nameText));

    setListSubItemText(pListItem, numSubItemId, numText);
    setListSubItemText(pListItem, nameSubItemId, nameText);
    setListSubItemAlignment(pListItem, nameSubItemId, ZKTextView::E_ALIGN_H_CENTER, ZKTextView::E_ALIGN_V_CENTER);
    setListSubItemVisible(pListItem, numSubItemId, true);
    setListSubItemVisible(pListItem, nameSubItemId, true);
    if (sIrrEmptyItemLayoutCaptured) {
        setListSubItemPosition(pListItem, numSubItemId, sIrrNumSubItemPosition);
        setListSubItemPosition(pListItem, nameSubItemId, sIrrArrSubItemPosition);
    } else if (numSubItem && nameSubItem) {
        setListSubItemPosition(pListItem, numSubItemId, numSubItem->getPosition());
        setListSubItemPosition(pListItem, nameSubItemId, nameSubItem->getPosition());
    }
}

static void obtainChangeIrrListItemData(ZKListView::ZKListItem *pListItem, int index) {
    setIrrGroupSubItemTexts(pListItem, index, ID_MAIN_IrrNum_SubItem, ID_MAIN_IrrArr_SubItem);
}

static void onChangeIrrListItemClick(int index) {
    if (sW2ProgrammaticGroupSelection) {
        return;
    }
    if (index < 0 || index >= getChangeIrrListItemCount()) {
        return;
    }

    if (index == static_cast<int>(sIrrGroupNumbers.size())) {
        if (sIrrGroupNumbers.size() < 128U) {
            int newGroupNo = 1;
            while (std::find(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), newGroupNo)
                    != sIrrGroupNumbers.end()) {
                ++newGroupNo;
            }
            sIrrGroupNumbers.push_back(newGroupNo);
            std::sort(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end());
            refreshChangeIrrListView();
            showChangeIrrListEmptyRow();
        }
        return;
    }

    selectIrrGroup(sIrrGroupNumbers[index]);
}

static void clearSelectedIrrGroup() {
    if (!requireSelectedIrrGroup()) {
        return;
    }
    DeviceDataStore::clearIrrGroup(sSelectedIrrGroupNo);
    refreshDeviceListViews();
    resetIrrGroupSelection();
}

bool clearPage2IrrGroupFromTuya(int groupCode) {
    if (groupCode == 0xFF) {
        for (size_t i = 0; i < sIrrGroupNumbers.size(); ++i) {
            DeviceDataStore::clearIrrGroup(sIrrGroupNumbers[i]);
        }
        refreshDeviceListViews();
        resetIrrGroupSelection();
        return true;
    }
    if (!isValidIrrGroupNo(groupCode)) {
        return false;
    }
    DeviceDataStore::clearIrrGroup(groupCode);
    refreshDeviceListViews();
    resetIrrGroupSelection();
    return true;
}

bool deletePage2IrrGroupFromTuya(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return false;
    }
    deleteSelectedIrrGroupFromOverview(groupNo);
    return true;
}

bool renamePage2IrrGroupFromTuya(int groupNo, const std::string& name) {
    if (!isValidIrrGroupNo(groupNo) || name.empty()) {
        return false;
    }
    const bool changed = DeviceDataStore::setIrrGroupName(groupNo, name.c_str());
    if (changed) {
        refreshChangeIrrListView();
    }
    return changed;
}

static bool isClearIrrWindowVisible() {
    return sClearIrrWindowVisible;
}

static void closeClearIrrWindow() {
    if (mWindow9Ptr) {
        mWindow9Ptr->hideWnd();
    }
    sClearIrrWindowVisible = false;
}

static void updateW2ChoiceDialogDisplay() {
    sW2ChoiceDialogGroupNo = normalizeW2TransferTargetGroupNo(
            sW2ChoiceDialogGroupNo);

    if (mButton8Ptr) {
        // Keep the FTU-defined font, color, alignment, and artwork.
        // Only the numeric value is runtime data.
        mButton8Ptr->setTouchable(true);
        char groupText[16] = {0};
        snprintf(groupText, sizeof(groupText), "%d",
                 sW2ChoiceDialogGroupNo);
        mButton8Ptr->setText(groupText);
    }

    // Keep the FTU-defined title style and update only the operation/group data.
    if (mClearIrrPromptTextViewPtr) {
        char titleText[96] = {0};
        const char* operationLabel = "";
        switch (sW2ChoiceDialogMode) {
        case 1:
            operationLabel = "清空阀组";
            break;
        case 2:
            operationLabel = "关联传感器";
            break;
        case 3:
            operationLabel = "删除阀组";
            break;
        case 4:
            operationLabel = "修改名称";
            break;
        default:
            break;
        }
        snprintf(titleText, sizeof(titleText), "%s     阀组[%d]",
                 operationLabel, sW2ChoiceDialogGroupNo);
        mClearIrrPromptTextViewPtr->setText(titleText);
    }
}

static void stepW2ChoiceDialogGroup(int delta) {
    sW2ChoiceDialogGroupNo = normalizeW2TransferTargetGroupNo(
            sW2ChoiceDialogGroupNo + delta);
    updateW2ChoiceDialogDisplay();
}

static void openW2GroupChoiceWindow(int mode) {
    if (!requireW2ActionIrrGroup()) {
        return;
    }

    sW2ChoiceDialogMode = mode;
    LOGD("[Window9] open group choice dialog mode=%d group=%d\n",
         mode, sSelectedIrrGroupNo);
    sW2ChoiceDialogGroupNo = normalizeW2TransferTargetGroupNo(
            sSelectedIrrGroupNo);

    if (mWindow9Ptr) {
        mWindow9Ptr->showWnd();
        ZKButton* previousButton =
                (ZKButton*)mWindow9Ptr->findControlByID(ID_MAIN_Button45);
        ZKButton* nextButton =
                (ZKButton*)mWindow9Ptr->findControlByID(ID_MAIN_Button41);
        ZKButton* allButton =
                (ZKButton*)mWindow9Ptr->findControlByID(ID_MAIN_Button44);
        if (previousButton) {
            previousButton->setTouchable(true);
        }
        if (nextButton) {
            nextButton->setTouchable(true);
        }
        const bool showAllGroups = (mode == 1 || mode == 2);
        if (allButton) {
            allButton->setVisible(showAllGroups);
            allButton->setTouchable(showAllGroups);
        }
    }
    sClearIrrWindowVisible = true;
    updateW2ChoiceDialogDisplay();
}

static void openClearIrrWindow() {
    openW2GroupChoiceWindow(1);
}

static void openGroupBindScopeWindow() {
    openW2GroupChoiceWindow(2);
}

static void openDeleteGroupWindow() {
    openW2GroupChoiceWindow(3);
}

static void openRenameGroupChoiceWindow() {
    openW2GroupChoiceWindow(4);
}

static void clearCurrentIrrGroupFromDialog() {
    if (!sClearIrrWindowVisible) {
        return;
    }
    closeClearIrrWindow();
    clearSelectedIrrGroup();
}

static void clearAllIrrGroupsFromDialog() {
    if (!sClearIrrWindowVisible) {
        return;
    }
    closeClearIrrWindow();
    for (size_t i = 0; i < sIrrGroupNumbers.size(); ++i) {
        DeviceDataStore::clearIrrGroup(sIrrGroupNumbers[i]);
    }
    refreshDeviceListViews();
    resetIrrGroupSelection();
}

static bool prepareSelectedIrrGroupNameEdit() {
    if (!requireSelectedIrrGroup()) {
        return false;
    }
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setTextColor(static_cast<int>(0xFF005BBBU));
        mGroupNameEditTextPtr->setText(getIrrGroupName(sSelectedIrrGroupNo));
    }
    return true;
}

static bool isPumpDevice(const SDATA* data) {
    return data && (std::strcmp(data->type, "水泵") == 0 || std::strcmp(data->type, W2_DEVICE_TYPE_VALVE) == 0);
}

static bool isFactorySelectableSensorDevice(const SDATA* data) {
    return data && (data->address == 6 || data->address == 8);
}

static bool isGroupBindPumpDevice(const SDATA* data) {
    return data && data->address >= 1 && data->address <= 2 && isPumpDevice(data);
}

static bool isCustomSelectableDevice(const SDATA* data) {
    return data && data->address >= CUSTOM_DEVICE_START_ID;
}

static bool isSensorBindDevice(const SDATA* data) {
    return data
            && (std::strcmp(data->type, W2_DEVICE_TYPE_SENSOR) == 0
                    || std::strcmp(data->type, "传感器") == 0)
            && (isFactorySelectableSensorDevice(data) || isCustomSelectableDevice(data));
}

static bool isSensorTextDevice(const SDATA* data) {
    return data
            && (std::strcmp(data->type, W2_DEVICE_TYPE_SENSOR) == 0
                    || std::strcmp(data->type, "传感器") == 0);
}

static int getFilteredDeviceCount(bool pump) {
    int count = 0;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if ((pump && isGroupBindPumpDevice(data)) || (!pump && isSensorBindDevice(data))) {
            ++count;
        }
    }
    return count;
}

static int getFilteredDeviceIndex(bool pump, int rowIndex) {
    int count = 0;
    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if ((pump && isGroupBindPumpDevice(data)) || (!pump && isSensorBindDevice(data))) {
            if (count == rowIndex) {
                return i;
            }
            ++count;
        }
    }
    return -1;
}

static bool containsSelectedDevice(const std::vector<int>& selectedIndexes, int deviceIndex) {
    for (size_t i = 0; i < selectedIndexes.size(); ++i) {
        if (selectedIndexes[i] == deviceIndex) {
            return true;
        }
    }
    return false;
}

static void addSelectedDevice(std::vector<int>& selectedIndexes, int deviceIndex) {
    if (deviceIndex < 0 || containsSelectedDevice(selectedIndexes, deviceIndex)) {
        return;
    }

    std::vector<int>::iterator it = selectedIndexes.begin();
    for (; it != selectedIndexes.end(); ++it) {
        if (*it > deviceIndex) {
            break;
        }
    }
    selectedIndexes.insert(it, deviceIndex);
}

static void removeSelectedDevice(std::vector<int>& selectedIndexes, int deviceIndex) {
    for (std::vector<int>::iterator it = selectedIndexes.begin(); it != selectedIndexes.end(); ++it) {
        if (*it == deviceIndex) {
            selectedIndexes.erase(it);
            return;
        }
    }
}

static void buildSelectedDeviceText(
        const std::vector<int>& selectedIndexes, char* text, size_t textSize) {
    if (!text || textSize == 0) {
        return;
    }
    text[0] = '\0';
    size_t offset = 0;
    for (size_t i = 0; i < selectedIndexes.size(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(selectedIndexes[i]);
        if (!data) {
            continue;
        }

        const int written = snprintf(
                text + offset,
                textSize - offset,
                "%s%d",
                (offset == 0) ? "" : "-",
                data->address);
        if (written <= 0 || static_cast<size_t>(written) >= textSize - offset) {
            break;
        }
        offset += static_cast<size_t>(written);
    }
}

static void setSelectedDeviceText(ZKEditText* editText, const std::vector<int>& selectedIndexes) {
    if (!editText) {
        return;
    }

    char text[256] = {0};
    buildSelectedDeviceText(selectedIndexes, text, sizeof(text));
    editText->setText(text);
}

static int normalizePumpAdvanceSeconds(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > MAX_PUMP_ADVANCE_SECONDS) {
        return MAX_PUMP_ADVANCE_SECONDS;
    }
    return value;
}

static int getIrrGroupPumpAdvanceSeconds(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return DEFAULT_PUMP_ADVANCE_SECONDS;
    }
    if (!sIrrGroupPumpAdvanceSecondsSet[groupNo - 1]) {
        return DEFAULT_PUMP_ADVANCE_SECONDS;
    }
    return sIrrGroupPumpAdvanceSeconds[groupNo - 1];
}

static void setIrrGroupPumpAdvanceSeconds(int groupNo, int seconds) {
    if (!isValidIrrGroupNo(groupNo)) {
        return;
    }
    sIrrGroupPumpAdvanceSeconds[groupNo - 1] = normalizePumpAdvanceSeconds(seconds);
    sIrrGroupPumpAdvanceSecondsSet[groupNo - 1] = true;
}

static ZKEditText* getPumpAdvanceSecEditText() {
    if (!sPumpAdvanceSecEditTextPtr && mGroupBindValueWindowPtr) {
        sPumpAdvanceSecEditTextPtr =
                (ZKEditText*)mGroupBindValueWindowPtr->findControlByID(ID_MAIN_PumpAdvanceSecEditText);
    }
    return sPumpAdvanceSecEditTextPtr;
}

static int readPumpAdvanceSecondsFromEditText() {
    ZKEditText* editText = getPumpAdvanceSecEditText();
    if (!editText) {
        return DEFAULT_PUMP_ADVANCE_SECONDS;
    }
    const std::string text = editText->getText();
    if (text.empty()) {
        return DEFAULT_PUMP_ADVANCE_SECONDS;
    }
    return normalizePumpAdvanceSeconds(atoi(text.c_str()));
}

static void updatePumpAdvanceSecEditText() {
    ZKEditText* editText = getPumpAdvanceSecEditText();
    if (!editText) {
        return;
    }
    const int seconds = sGroupBindAllGroups ?
            DEFAULT_PUMP_ADVANCE_SECONDS :
            getIrrGroupPumpAdvanceSeconds(sSelectedIrrGroupNo);
    editText->setText(seconds);
}

static void savePumpAdvanceSecondsFromEditText() {
    const int seconds = readPumpAdvanceSecondsFromEditText();
    if (sGroupBindAllGroups) {
        for (size_t i = 0; i < sIrrGroupNumbers.size(); ++i) {
            setIrrGroupPumpAdvanceSeconds(sIrrGroupNumbers[i], seconds);
        }
    } else {
        setIrrGroupPumpAdvanceSeconds(sSelectedIrrGroupNo, seconds);
    }
}

static void updateGroupBindSelectionEditTexts() {
    setSelectedDeviceText(mAddPumpEditTextPtr, sSelectedPumpDeviceIndexes);
    setSelectedDeviceText(mAddSenserEditTextPtr, sSelectedSensorDeviceIndexes);

    if (mTextView7Ptr) {
        mTextView7Ptr->setText("");
    }
    if (mTextView6Ptr) {
        mTextView6Ptr->setText("");
    }
}

static void clearGroupBindSelections() {
    sSelectedPumpDeviceIndexes.clear();
    sSelectedSensorDeviceIndexes.clear();
    updateGroupBindSelectionEditTexts();
}

static void syncGroupBindSelectionsFromCurrentGroup() {
    clearGroupBindSelections();

    if (!isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return;
    }

    const int total = DeviceDataStore::getDeviceCount();
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        const bool matchesScope = data && (sGroupBindAllGroups
                ? std::strcmp(data->arre, "*") == 0
                : isDeviceBoundToIrrGroup(data, sSelectedIrrGroupNo));
        if (!matchesScope) {
            continue;
        }

        if (isGroupBindPumpDevice(data)) {
            addSelectedDevice(sSelectedPumpDeviceIndexes, i);
        } else if (isSensorBindDevice(data)) {
            addSelectedDevice(sSelectedSensorDeviceIndexes, i);
        }
    }

    updateGroupBindSelectionEditTexts();
}

static void refreshGroupBindListViews() {
    if (mSelectPumpListViewPtr) {
        mSelectPumpListViewPtr->refreshListView();
    }
    if (mSelectSenserListViewPtr) {
        mSelectSenserListViewPtr->refreshListView();
    }
}

static void updateGroupBindEditTexts() {
    char line[256] = {0};
    if (mGroupNumEditTextPtr) {
        if (sGroupBindAllGroups) {
            snprintf(line, sizeof(line), "\xE5\x85\xA8\xE9\x83\xA8");
            mGroupNumEditTextPtr->setText(line);
        } else if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            snprintf(line, sizeof(line), "%d", sSelectedIrrGroupNo);
            mGroupNumEditTextPtr->setText(line);
        } else {
            mGroupNumEditTextPtr->setText("");
        }
    }

    if (mTextView5Ptr) {
        mTextView5Ptr->setText("");
    }
    if (mGroupBindNameEditTextPtr) {
        mGroupBindNameEditTextPtr->setText(sGroupBindAllGroups
                ? "\xE6\x89\x80\xE6\x9C\x89\xE9\x98\x80\xE7\xBB\x84"
                : getIrrGroupName(sSelectedIrrGroupNo));
    }
}

static void openGroupBindSelectionWindow() {
    if (!requireSelectedIrrGroup()) {
        return;
    }
    updateGroupBindEditTexts();
    syncGroupBindSelectionsFromCurrentGroup();
    updatePumpAdvanceSecEditText();
    refreshGroupBindListViews();

    hideAllPageWindows();
    if (mGroupBindValueWindowPtr) {
        mGroupBindValueWindowPtr->showWnd();
    }
}

static void openGroupBindWindow() {
    openGroupBindScopeWindow();
}

static void chooseCurrentIrrGroupFromDialog() {
    if (!sClearIrrWindowVisible) {
        return;
    }
    const int mode = sW2ChoiceDialogMode;
    const int groupNo = normalizeW2TransferTargetGroupNo(
            sW2ChoiceDialogGroupNo);
    LOGD("[Window9] confirm mode=%d group=%d selected=%d\n",
         mode, groupNo, sSelectedIrrGroupNo);

    // Deletion uses the value displayed in Window9 directly. Do not route it
    // through the overview selection state: that state can be reset while the
    // dialog is open, which made Confirm leave the group untouched.
    if (mode == 3) {
        closeClearIrrWindow();
        sW2ChoiceDialogMode = 0;
        deleteSelectedIrrGroupFromOverview(groupNo);
        return;
    }

    sSelectedIrrGroupNo = groupNo;
    sW2EditorPinnedGroupNo = groupNo;
    ensureIrrGroupNumberVisible(groupNo);
    updateClearIrrButtonText();
    closeClearIrrWindow();
    sW2ChoiceDialogMode = 0;
    if (mode == 2) {
        sGroupBindAllGroups = false;
        openGroupBindSelectionWindow();
    } else if (mode == 1) {
        clearSelectedIrrGroup();
    } else if (mode == 4) {
        openGroupRenameWindow();
    }
}

static void chooseAllIrrGroupsFromDialog() {
    if (!sClearIrrWindowVisible) {
        return;
    }
    const int mode = sW2ChoiceDialogMode;
    closeClearIrrWindow();
    sW2ChoiceDialogMode = 0;
    if (mode == 2) {
        sGroupBindAllGroups = true;
        openGroupBindSelectionWindow();
    } else if (mode == 1) {
        for (size_t i = 0; i < sIrrGroupNumbers.size(); ++i) {
            DeviceDataStore::clearIrrGroup(sIrrGroupNumbers[i]);
        }
        refreshDeviceListViews();
        resetIrrGroupSelection();
    }
}

static void hideGroupBindWindowOnly() {
    if (mGroupBindValueWindowPtr) {
        mGroupBindValueWindowPtr->hideWnd();
    }
    clearGroupBindSelections();
    refreshGroupBindListViews();
}

static void closeGroupBindWindow() {
    hideGroupBindWindowOnly();
    if (mw2set_windowPtr) {
        mw2set_windowPtr->showWnd();
        sW2SetWindowOpen = true;
    }
}

static void deleteSelectedIrrGroup() {
    if (!isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        closeGroupBindWindow();
        return;
    }

    DeviceDataStore::removeIrrGroup(sSelectedIrrGroupNo);
    std::vector<int>::iterator groupIt = std::find(
            sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), sSelectedIrrGroupNo);
    if (groupIt != sIrrGroupNumbers.end()) {
        sIrrGroupNumbers.erase(groupIt);
    }
    refreshDeviceListViews();
    resetIrrGroupSelection();
    closeGroupBindWindow();
}

static bool expandAllGroupDevicesWithoutDeletedGroup(int deletedGroupNo) {
    // The '*' marker means that a pump/sensor is associated with every visible
    // group. Before removing one group, turn that marker into the remaining
    // explicit group numbers; otherwise the next list refresh would infer the
    // deleted group again from '*'.
    const std::vector<int> visibleGroups = sIrrGroupNumbers;
    bool changed = false;
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (!data || std::strcmp(data->arre, "*") != 0) {
            continue;
        }

        changed = DeviceDataStore::unbindDeviceFromIrrGroup(i) || changed;
        for (size_t groupIndex = 0; groupIndex < visibleGroups.size(); ++groupIndex) {
            const int groupNo = visibleGroups[groupIndex];
            if (isValidIrrGroupNo(groupNo) && groupNo != deletedGroupNo) {
                changed = DeviceDataStore::bindDeviceToIrrGroup(i, groupNo) || changed;
            }
        }
    }
    return changed;
}

static void deleteSelectedIrrGroupFromOverview(int groupNo) {
    // The caller passes the number shown in Window9. Keep this function
    // independent from the overview row-selection state.
    groupNo = normalizeW2TransferTargetGroupNo(groupNo);
    if (!isValidIrrGroupNo(groupNo)) {
        LOGD("[Window9] delete ignored invalid group=%d selected=%d\n",
             groupNo, sSelectedIrrGroupNo);
        return;
    }

    const bool listedBefore = std::find(
            sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), groupNo)
            != sIrrGroupNumbers.end();
    const bool wildcardChanged = expandAllGroupDevicesWithoutDeletedGroup(groupNo);
    const bool memberChanged = DeviceDataStore::removeIrrGroup(groupNo);

    sIrrGroupNumbers.erase(
            std::remove(sIrrGroupNumbers.begin(), sIrrGroupNumbers.end(), groupNo),
            sIrrGroupNumbers.end());
    sIrrGroupCapacity[groupNo] = 0;
    sIrrGroupPumpAdvanceSeconds[groupNo - 1] = 0;
    sIrrGroupPumpAdvanceSecondsSet[groupNo - 1] = false;
    if (sIrrCapacityGroupNo == groupNo) {
        sIrrCapacityGroupNo = sIrrGroupNumbers.empty() ? 1 : sIrrGroupNumbers[0];
        sIrrCapacityPending = getIrrGroupCapacity(sIrrCapacityGroupNo);
    }
    if (sW2EditorPinnedGroupNo == groupNo) {
        sW2EditorPinnedGroupNo = -1;
    }
    if (sW2PendingGroupSelectionNo == groupNo) {
        sW2PendingGroupSelectionNo = -1;
        sW2PendingGroupSelectionAtMs = 0;
    }

    LOGD("[Window9] delete group=%d listed=%d wildcard_changed=%d member_changed=%d remaining=%u\n",
         groupNo, listedBefore ? 1 : 0, wildcardChanged ? 1 : 0,
         memberChanged ? 1 : 0, static_cast<UINT>(sIrrGroupNumbers.size()));
    refreshDeviceListViews();
    refreshChangeIrrListView();
    resetIrrGroupSelection();
}

static void bindSelectedDevicesToIrrGroup() {
    if (!isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return;
    }

    savePumpAdvanceSecondsFromEditText();

    bool changed = false;
    for (int i = 0; i < DeviceDataStore::getDeviceCount(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && (sGroupBindAllGroups
                    ? std::strcmp(data->arre, "*") == 0
                    : isDeviceBoundToIrrGroup(data, sSelectedIrrGroupNo))
                && (isGroupBindPumpDevice(data) || isSensorTextDevice(data))) {
            changed = (sGroupBindAllGroups
                    ? DeviceDataStore::unbindDeviceFromIrrGroup(i)
                    : DeviceDataStore::unbindDeviceFromIrrGroup(i, sSelectedIrrGroupNo))
                    || changed;
        }
    }
    for (size_t i = 0; i < sSelectedPumpDeviceIndexes.size(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(sSelectedPumpDeviceIndexes[i]);
        if (!sGroupBindAllGroups && data && std::strcmp(data->arre, "*") == 0) {
            continue;
        }
        changed = (sGroupBindAllGroups
                    ? DeviceDataStore::bindDeviceToAllIrrGroups(sSelectedPumpDeviceIndexes[i])
                    : DeviceDataStore::bindDeviceToIrrGroup(sSelectedPumpDeviceIndexes[i], sSelectedIrrGroupNo)) || changed;
    }
    for (size_t i = 0; i < sSelectedSensorDeviceIndexes.size(); ++i) {
        const SDATA* data = DeviceDataStore::getDevice(sSelectedSensorDeviceIndexes[i]);
        if (!sGroupBindAllGroups && data && std::strcmp(data->arre, "*") == 0) {
            continue;
        }
        changed = (sGroupBindAllGroups
                    ? DeviceDataStore::bindDeviceToAllIrrGroups(sSelectedSensorDeviceIndexes[i])
                    : DeviceDataStore::bindDeviceToIrrGroup(sSelectedSensorDeviceIndexes[i], sSelectedIrrGroupNo)) || changed;
    }

    if (changed) {
        refreshDeviceListViews();
        refreshChangeIrrListView();
    }
    closeGroupBindWindow();
}

static int getGroupBindDeviceListItemCount(bool pump) {
    return getFilteredDeviceCount(pump);
}

static void obtainGroupBindDeviceListItemData(ZKListView::ZKListItem *pListItem, int index, bool pump) {
    const int deviceIndex = getFilteredDeviceIndex(pump, index);
    const SDATA* data = DeviceDataStore::getDevice(deviceIndex);
    if (!pListItem || !data) {
        return;
    }

    pListItem->setText(data->name);
    pListItem->setSelected(
            pump
                    ? containsSelectedDevice(sSelectedPumpDeviceIndexes, deviceIndex)
                    : containsSelectedDevice(sSelectedSensorDeviceIndexes, deviceIndex));
}

static void onGroupBindDeviceListItemClick(int index, bool pump) {
    const int deviceIndex = getFilteredDeviceIndex(pump, index);
    if (deviceIndex < 0) {
        return;
    }

    if (pump) {
        if (containsSelectedDevice(sSelectedPumpDeviceIndexes, deviceIndex)) {
            removeSelectedDevice(sSelectedPumpDeviceIndexes, deviceIndex);
        } else {
            addSelectedDevice(sSelectedPumpDeviceIndexes, deviceIndex);
        }
    } else {
        if (containsSelectedDevice(sSelectedSensorDeviceIndexes, deviceIndex)) {
            removeSelectedDevice(sSelectedSensorDeviceIndexes, deviceIndex);
        } else {
            addSelectedDevice(sSelectedSensorDeviceIndexes, deviceIndex);
        }
    }
    updateGroupBindSelectionEditTexts();
    refreshGroupBindListViews();
}

static const char* getW2SelectedDeviceType() {
    return sW2SelectedTypeIndex == 0 ? W2_DEVICE_TYPE_VALVE : W2_DEVICE_TYPE_SENSOR;
}



static int getW2DefaultAddress() {
    return CUSTOM_DEVICE_START_ID + DeviceDataStore::getCustomDeviceCount();
}

static int normalizeW2TransferTargetGroupNo(int groupNo) {
    if (groupNo < 1) {
        return 1;
    }
    if (groupNo > 128) {
        return 128;
    }
    return groupNo;
}

static int readW2Window11TargetGroupNo() {
    std::string digits;
    if (mWindow11TargetGroupEditTextPtr) {
        const std::string text = mWindow11TargetGroupEditTextPtr->getText();
        digits.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] >= '0' && text[i] <= '9') {
                digits.push_back(text[i]);
            }
        }
    }
    if (!digits.empty()) {
        return normalizeW2TransferTargetGroupNo(atoi(digits.c_str()));
    }
    return normalizeW2TransferTargetGroupNo(sW2TransferTargetGroupNo);
}

static void updateW2Window11TargetGroupDisplay(int sourceGroupNo) {
    if (!isValidIrrGroupNo(sourceGroupNo)) {
        return;
    }
    sW2TransferTargetGroupNo = normalizeW2TransferTargetGroupNo(
            sW2TransferTargetGroupNo);

    if (mWindow11PromptTextPtr) {
        char text[160] = {0};
        snprintf(text, sizeof(text),
                "\xE5\xBD\x93\xE5\x89\x8D\xE8\xAE\xBE\xE5\xA4\x87\xE5\xB7\xB2\xE5\x9C\xA8\xE9\x98\x80\xE7\xBB\x84[%d]\n"
                "\xE8\xAF\xB7\xE9\x80\x89\xE6\x8B\xA9\xE7\xA7\xBB\xE9\x99\xA4\xE6\x88\x96\xE8\xBD\xAC\xE7\xA7\xBB\xE8\x87\xB3\xE9\x98\x80\xE7\xBB\x84[%d]",
                sourceGroupNo, sW2TransferTargetGroupNo);
        mWindow11PromptTextPtr->setText(text);
    }

    if (mWindow11TargetGroupEditTextPtr && !sW2Window11TargetUpdating) {
        char targetText[16] = {0};
        snprintf(targetText, sizeof(targetText), "%d",
                sW2TransferTargetGroupNo);
        sW2Window11TargetUpdating = true;
        mWindow11TargetGroupEditTextPtr->setText(targetText);
        sW2Window11TargetUpdating = false;
    }
}

static void onW2Window11TargetGroupTextChanged(const std::string &text) {
    if (sW2Window11TargetUpdating) {
        return;
    }

    std::string digits;
    digits.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (ch >= '0' && ch <= '9') {
            digits.push_back(ch);
        }
    }

    if (mWindow11TargetGroupEditTextPtr && digits != text) {
        sW2Window11TargetUpdating = true;
        mWindow11TargetGroupEditTextPtr->setText(digits.c_str());
        sW2Window11TargetUpdating = false;
    }

    if (digits.empty()) {
        return;
    }

    sW2TransferTargetGroupNo = normalizeW2TransferTargetGroupNo(atoi(digits.c_str()));
    updateW2Window11TargetGroupDisplay(sSelectedIrrGroupNo);
}

static void incrementW2Window11TargetGroup() {
    if (!sW2Window11Visible || !isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return;
    }
    sW2TransferTargetGroupNo = normalizeW2TransferTargetGroupNo(
            sW2TransferTargetGroupNo + 1);
    updateW2Window11TargetGroupDisplay(sSelectedIrrGroupNo);
}

static void hideW2Window11Only() {
    if (mWindow11Ptr) {
        mWindow11Ptr->hideWnd();
    }
    sW2Window11Visible = false;
}

static bool hideW2Window11IfTouchedOutside(const MotionEvent &ev) {
    if (!sW2Window11Visible || !mWindow11Ptr ||
            ev.mActionStatus != MotionEvent::E_ACTION_DOWN) {
        return false;
    }

    const LayoutPosition position(236, 192, 550, 212);
    if (position.isHit(ev.mX, ev.mY)) {
        // Keep Window11 controls clickable. Only a touch outside the dialog
        // is treated as cancel.
        return false;
    }

    hideW2Window11Only();
    return true;
}

static void openW2Window11(int groupNo) {
    if (!isValidIrrGroupNo(groupNo) || !sW2SetWindowOpen) {
        return;
    }
    sW2TransferSourceGroupNo = groupNo;
    sW2EditorPinnedGroupNo = groupNo;
    sW2TransferTargetGroupNo = normalizeW2TransferTargetGroupNo(groupNo + 1);
    updateW2Window11TargetGroupDisplay(groupNo);
    if (mWindow11Ptr) {
        mWindow11Ptr->showWnd();
        sW2Window11Visible = true;
    }
}

static void removeCurrentW2DeviceFromGroup() {
    if (!sW2Window11Visible || !isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return;
    }
    const int groupNo = sSelectedIrrGroupNo;
    bool changed = false;
    if (sW2AddingDevice) {
        changed = true;
    } else {
        const int index = getCurrentW2DeviceIndex();
        if (index >= 0) {
            changed = DeviceDataStore::unbindDeviceFromIrrGroup(index, groupNo);
        }
    }
    if (changed) {
        sSelectedIrrGroupNo = -1;
        refreshDeviceListViews();
        refreshChangeIrrListView();
        updateClearIrrButtonText();
    }
    sW2TransferMode = false;
    sW2TransferSourceGroupNo = -1;
    hideW2Window11Only();
}

static void beginW2DeviceTransfer() {
    if (!sW2Window11Visible || !isValidIrrGroupNo(sSelectedIrrGroupNo)) {
        return;
    }

    const int sourceGroupNo = sSelectedIrrGroupNo;
    // Read the visible edit box at click time as well as from its change
    // callback. This keeps the transfer target correct on firmware builds
    // where the final keystroke callback is delivered after button handling.
    sW2TransferTargetGroupNo = readW2Window11TargetGroupNo();
    const int targetGroupNo = normalizeW2TransferTargetGroupNo(
            sW2TransferTargetGroupNo);
    sW2TransferTargetGroupNo = targetGroupNo;
    if (targetGroupNo == sourceGroupNo) {
        hideW2Window11Only();
        return;
    }
    if (!canAssignCurrentW2ValveToIrrGroup(targetGroupNo, true)) {
        return;
    }

    const int index = getCurrentW2DeviceIndex();
    bool changed = false;
    if (sW2AddingDevice) {
        sSelectedIrrGroupNo = targetGroupNo;
        changed = true;
    } else if (index >= 0) {
        // Bind first so a multi-group device is never left without a target.
        changed = DeviceDataStore::bindDeviceToIrrGroup(index, targetGroupNo);
        changed = DeviceDataStore::unbindDeviceFromIrrGroup(index, sourceGroupNo) || changed;
    }

    if (changed) {
        // The target is a real numeric group, even when it was not one of the
        // initial rows 1..4. Keep it in the table before refreshing the list.
        sW2EditorPinnedGroupNo = targetGroupNo;
        ensureIrrGroupNumberVisible(targetGroupNo);
        syncIrrGroupNumbersFromDeviceData();
        // Pin once more after the data refresh so a target such as 28 cannot
        // be lost between the adapter refresh and the selection update.
        ensureIrrGroupNumberVisible(targetGroupNo);
        sSelectedIrrGroupNo = targetGroupNo;
        requestW2GroupSelection(targetGroupNo);
        refreshDeviceListViews();
        refreshChangeIrrListView();
        requestW2GroupSelection(targetGroupNo);

        const int currentIndexAfterTransfer = getCurrentW2DeviceIndex();
        const SDATA* afterData = currentIndexAfterTransfer >= 0
                ? DeviceDataStore::getDevice(currentIndexAfterTransfer) : NULL;
        LOGD("[Window2] transfer source=%d target=%d index=%d changed=%d arre=%s group_rows=%u selected=%d\n",
             sourceGroupNo, targetGroupNo, currentIndexAfterTransfer,
             changed ? 1 : 0, afterData ? afterData->arre : "-",
             static_cast<UINT>(sIrrGroupNumbers.size()),
             sSelectedIrrGroupNo);
    }
    sW2TransferMode = false;
    sW2TransferSourceGroupNo = -1;
    hideW2Window11Only();
}

static void hideW2SetWindowOnly() {
    const bool restoreEditorViews =
            sW2EditorSessionActive || sW2AutoAssignPreviewPending;

    hideW2AddDeviceWindowOnly();
    hideW2Window11Only();
    sW2TransferMode = false;
    sW2TransferSourceGroupNo = -1;
    sW2TransferTargetGroupNo = -1;
    closeIrrCapacityWindow(false);
    closeClearIrrWindow();

    // Close the editor and publish the closed state before restoring data.
    // The old order refreshed Window2 ListViews from inside the visible editor
    // callback, which could re-enter FTU layout/click handling and freeze the
    // board after a cancel or page switch.
    if (mw2set_windowPtr) {
        mw2set_windowPtr->hideWnd();
    }
    sW2SetWindowOpen = false;
    finishW2AutoAssignPreview(false);
    restoreW2EditorSession();

    sW2EditorPinnedGroupNo = -1;
    sW2PendingGroupSelectionNo = -1;
    sW2PendingGroupSelectionAtMs = 0;
    sW2EditingIndex = -1;
    sW2AddingDevice = false;
    sW2CurrentAddress = 0;
    sSelectedIrrGroupNo = -1;
    if (mGroupNameEditTextPtr) {
        mGroupNameEditTextPtr->setText("");
        mGroupNameEditTextPtr->setTextColor(static_cast<int>(0x00FFFFFFU));
    }
    updateClearIrrButtonText();

    sW2EditorRefreshAfterClose = restoreEditorViews && sPage2Active;
}

static void closeW2SetWindow() {
    hideW2SetWindowOnly();
    if (mButton1Ptr) mButton1Ptr->setSelected(false);
    if (mButton2Ptr) mButton2Ptr->setSelected(true);
    if (mWindow2Ptr) {
        mWindow2Ptr->showWnd();
    }
    if (sW2EditorRefreshAfterClose) {
        sW2EditorRefreshAfterClose = false;
        refreshDeviceListViews();
        refreshChangeIrrListView();
    }
}

static void openW2SetWindow(int index) {
    sW2AddingDevice = DeviceDataStore::isEmptyRow(index);
    sW2EditingIndex = sW2AddingDevice ? -1 : index;
    sSelectedIrrGroupNo = -1;
    sW2EditorPinnedGroupNo = -1;

    int address = getW2DefaultAddress();
    const char* name = "";
    const char* type = W2_DEVICE_TYPE_VALVE;

    if (!sW2AddingDevice) {
        const SDATA* data = DeviceDataStore::getDevice(index);
        if (!data || !DeviceDataStore::isCustomDevice(index)) {
            return;
        }
        address = data->address;
        name = data->name;
        type = data->type;
        const int boundGroupNo = atoi(data->arre);
        if (isValidIrrGroupNo(boundGroupNo)) {
            sSelectedIrrGroupNo = boundGroupNo;
            sW2EditorPinnedGroupNo = boundGroupNo;
        }
    }

    beginW2EditorSession();
    sW2SelectedTypeIndex = ((std::strcmp(type, W2_DEVICE_TYPE_SENSOR) == 0)
            || (std::strcmp(type, "传感器") == 0)) ? 1 : 0;

    updateW2AddressDisplay(address);
    if (mW2_NameEditTextPtr) {
        mW2_NameEditTextPtr->setText(sW2AddingDevice ? getW2SelectedDeviceType() : name);
    }
    hideAllPageWindows();
    if (mw2set_windowPtr) {
        mw2set_windowPtr->showWnd();
    }
    sW2SetWindowOpen = true;
    updateClearIrrButtonText();
    refreshChangeIrrListView();

}

static void saveW2SetWindow() {
    int address = 0;
    std::string nameText;

    address = sW2CurrentAddress;
    if (mW2_NameEditTextPtr) {
        nameText = mW2_NameEditTextPtr->getText();
    }

    if (isValidIrrGroupNo(sSelectedIrrGroupNo)
            && !canAssignCurrentW2ValveToIrrGroup(sSelectedIrrGroupNo, true)) {
        return;
    }

    bool changed = false;
    if (sW2AddingDevice) {
        changed = DeviceDataStore::addDevice(address, nameText.c_str(), getW2SelectedDeviceType());
        if (changed && isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            const int addedIndex = DeviceDataStore::getDeviceCount() - 1;
            DeviceDataStore::bindDeviceToIrrGroup(addedIndex, sSelectedIrrGroupNo);
        }
    } else {
        // Resolve the actual device again from the address before committing.
        // Window2 list rows can be rebuilt while the editor is open, so the
        // stored row index is not the only reliable identity.
        const int editingIndex = getCurrentW2DeviceIndex();
        if (editingIndex >= 0 && DeviceDataStore::isCustomDevice(editingIndex)) {
            // The selected valve group is an independent edit. Address 32 must
            // still bind to group 2 even when the text fields are unchanged.
            const bool deviceChanged = DeviceDataStore::updateDevice(
                    editingIndex, address, nameText.c_str(),
                    getW2SelectedDeviceType());
            const bool groupChanged = isValidIrrGroupNo(sSelectedIrrGroupNo)
                    && DeviceDataStore::bindDeviceToIrrGroup(
                            editingIndex, sSelectedIrrGroupNo);
            changed = deviceChanged || groupChanged;
        } else if (isValidIrrGroupNo(sSelectedIrrGroupNo)) {
            changed = DeviceDataStore::bindDeviceAddressToIrrGroup(
                    address, sSelectedIrrGroupNo);
        }
    }

    const bool addedDevice = changed && sW2AddingDevice;
    if (changed) {
        refreshDeviceListViews();
        refreshChangeIrrListView();
    }
    finishW2AutoAssignPreview(true);
    commitW2EditorSession();
    closeW2SetWindow();
    if (addedDevice) {
        showDeviceListEmptyRow();
    }
}

static void deleteW2SetWindowDevice() {
    if (!sW2AddingDevice && DeviceDataStore::deleteDevice(sW2EditingIndex)) {
        refreshDeviceListViews();
    }
    // Device deletion is an explicit editor operation, so keep it when the
    // editor closes instead of treating it as a cancel rollback.
    finishW2AutoAssignPreview(true);
    commitW2EditorSession();
    closeW2SetWindow();
}

static void setListSubItemText(ZKListView::ZKListItem *pListItem, int id, const char* text) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* subItem = pListItem->findSubItemByID(id);
    if (subItem) {
        subItem->setText(text ? text : "");
    }
}

static void setListSubItemAlignment(ZKListView::ZKListItem *pListItem, int id, ZKTextView::EAlignH h, ZKTextView::EAlignV v) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* subItem = pListItem->findSubItemByID(id);
    if (subItem) {
        subItem->setAlignment(h, v);
    }
}

static void setListSubItemVisible(ZKListView::ZKListItem *pListItem, int id, bool visible) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* subItem = pListItem->findSubItemByID(id);
    if (subItem) {
        subItem->setVisible(visible);
    }
}

static void setListSubItemPosition(ZKListView::ZKListItem *pListItem, int id, const LayoutPosition &position) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* subItem = pListItem->findSubItemByID(id);
    if (subItem) {
        subItem->setPosition(position);
    }
}

static int getPage2DeviceListItemCount(const ZKListView *pListView) {
    return DeviceDataStore::getDeviceListRowCount();
}

static void obtainPage2DeviceListItemData(ZKListView *pListView,
                                          ZKListView::ZKListItem *pListItem,
                                          int index) {
    if (!pListItem) {
        return;
    }

    ZKListView::ZKListSubItem* addressItem = pListItem->findSubItemByID(ID_MAIN_AddressSubItem);
    ZKListView::ZKListSubItem* nameItem = pListItem->findSubItemByID(ID_MAIN_NameSubItem);
    ZKListView::ZKListSubItem* typeItem = pListItem->findSubItemByID(ID_MAIN_TypeSubItem);
    ZKListView::ZKListSubItem* arreItem = pListItem->findSubItemByID(ID_MAIN_ArreSubItem);
    ZKListView::ZKListSubItem* statusItem = pListItem->findSubItemByID(ID_MAIN_StatusSubItem);
    ZKListView::ZKListSubItem* operationItem = pListItem->findSubItemByID(ID_MAIN_OperationSubItem);
    const bool isEmptyRow = DeviceDataStore::isEmptyRow(index);
    const bool isEditableDevice = DeviceDataStore::isCustomDevice(index);

    if (addressItem) addressItem->setTouchable(isEmptyRow || isEditableDevice);
    if (nameItem) nameItem->setTouchable(isEmptyRow || isEditableDevice);
    if (typeItem) typeItem->setTouchable(isEditableDevice);
    if (arreItem) arreItem->setTouchable(isEditableDevice);
    if (statusItem) statusItem->setTouchable(isEmptyRow || isEditableDevice);
    if (operationItem) operationItem->setTouchable(false);
    if (operationItem) operationItem->setVisible(false);

    if (isEmptyRow) {
        const bool discoveryRunning = sPage2CachedDiscoveryRunning;
        if (addressItem) {
            addressItem->setTextColor(static_cast<int>(0xFF168BFFU));
            addressItem->setText("点击添加");
        }
        if (nameItem) nameItem->setText("");
        if (typeItem) typeItem->setText("");
        if (arreItem) arreItem->setText("");
        if (statusItem) statusItem->setText("");
        if (operationItem) {
            operationItem->setTextColor(static_cast<int>(0xFF168BFFU));
            operationItem->setText(discoveryRunning ? "同步中" : "同步");
            operationItem->setTouchable(!discoveryRunning);
            operationItem->setVisible(true);
        }
        return;
    }

    const SDATA* data = DeviceDataStore::getDevice(index);
    if (!data) {
        return;
    }

    char addressBuf[16] = {0};
    snprintf(addressBuf, sizeof(addressBuf), "%d", data->address);

    if (addressItem) addressItem->setText(addressBuf);
    if (nameItem) {
        nameItem->setTextColor(static_cast<int>(0xFF000000U));
        nameItem->setText(data->name);
    }
    if (typeItem) typeItem->setText(data->type);
    if (arreItem) arreItem->setText(data->arre);
    if (statusItem) {
        statusItem->setText(data->status);
        statusItem->setTextColor(
            data->connected ? static_cast<int>(0xFF248A3DU)
                            : static_cast<int>(0xFF737A84U));
    }
    const bool canDelete = isEditableDevice && data->address >= 20;
    if (operationItem) {
        operationItem->setTextColor(static_cast<int>(0xFF168BFFU));
        operationItem->setText(canDelete ? "删除" : "");
        operationItem->setTouchable(canDelete);
        operationItem->setVisible(canDelete);
    }
}

static void onPage2DeviceListItemClick(ZKListView *pListView, int index, int id) {
    if (isWindow5DeviceDiscoveryRunning()) {
        return;
    }

    if (DeviceDataStore::isEmptyRow(index)) {
        if (id == ID_MAIN_OperationSubItem) {
            if (requestWindow5DeviceDiscovery()) {
                startPage2DeviceDiscoveryTip();
            }
            return;
        }

        if (id == ID_MAIN_AddressSubItem) {
            openW2AddDeviceWindow();
        }
        return;
    }

    const SDATA* data = DeviceDataStore::getDevice(index);
    if (id == ID_MAIN_OperationSubItem && data && data->address >= 20) {
        const int deletedAddress = data->address;
        if (DeviceDataStore::deleteDevice(index)) {
            clearDeletedDeviceSelection(index);
            if (sW2CurrentAddress == deletedAddress) {
                sW2CurrentAddress = 0;
            }
            refreshDeviceListViews();
            refreshChangeIrrListView();
        }
        return;
    }

    if (DeviceDataStore::isCustomDevice(index)) {
        openW2SetWindow(index);
    }
}

static int getPage2DeviceTipListItemCount(const ZKListView *pListView) {
    return 1;
}

static void obtainPage2DeviceTipListItemData(ZKListView *pListView,
                                             ZKListView::ZKListItem *pListItem,
                                             int index) {
    setListSubItemText(pListItem, ID_MAIN_AddressTipSubItem, "地址");
    setListSubItemText(pListItem, ID_MAIN_NameTipSubItem, "名称");
    setListSubItemText(pListItem, ID_MAIN_TypeTipSubItem, "类型");
    setListSubItemText(pListItem, ID_MAIN_ArreTipSubItem, "阀组编号");
    setListSubItemText(pListItem, ID_MAIN_StatusTipSubItem, "状态");
    setListSubItemText(pListItem, ID_MAIN_OperationTipSubItem, "操作");
}

static void onPage2DeviceTipListItemClick(ZKListView *pListView, int index, int id) {
}

static void onPage2Show() {
    sPage2Active = true;
    sW2TipCheckedThisVisit = false;
    hideW2TipWindowOnly();
    syncIrrGroupNumbersFromDeviceData();
    refreshDeviceListViews();
    refreshChangeIrrListView();
}

static void onPage2Hide() {
    sPage2Active = false;
    hideW2Window11Only();
    sW2TransferMode = false;
    sW2TransferSourceGroupNo = -1;
    hideGroupBindWindowOnly();
    hideW2AddDeviceWindowOnly();
    hideW2SetWindowOnly();
    hideW2TipWindowOnly();
    hideW2ActionTipWindowOnly();
    closeGroupRenameWindow();
}
