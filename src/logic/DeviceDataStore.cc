#include "DeviceDataStore.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

std::vector<SDATA> w2_DeviceDataList;

namespace {

struct DefaultDeviceConfig {
    int address;
    const char* name;
    const char* type;
    const char* arre;
};

const DefaultDeviceConfig kDefaultDevices[] = {
    {1,  "水泵1",      "水泵",   "-"},
    {2,  "水泵2",      "水泵",   "-"},
    {3,  "水泵3",      "水泵",   "-"},
    {4,  "水泵4",      "水泵",   "-"},
    {5,  "水泵5",      "水泵",   "-"},
    {6,  "湿度传感器", "传感器", "-"},
    {8,  "雨量传感器", "传感器", "-"},
    {9,  "水压表",     "传感器", "-"},
    {10, "流量表",     "传感器", "-"},
};

const char* kSensorNames[] = {
    "流量表",
    "雨量传感器",
    "温度传感器",
    "压力传感器",
    "电磁阀",
    "电动阀",
    "气动阀",
    "调节阀",
};

int sNameIndex = 0;
int sSelectedEditIndex = -1;
char sIrrGroupNames[128][32];

bool isValidIrrGroupNo(int groupNo) {
    return groupNo > 0 && groupNo <= 128;
}

void copyText(char* dst, size_t dstSize, const char* src) {
    if (!dst || dstSize == 0) {
        return;
    }

    std::strncpy(dst, src ? src : "", dstSize - 1);
    dst[dstSize - 1] = '\0';
}

void fillCustomDevice(SDATA& data, int address, const char* name, const char* type) {
    const char* deviceType = (type && type[0] != '\0') ? type : "传感器";
    const char* deviceName = (name && name[0] != '\0') ? name : deviceType;

    data.address = address;
    copyText(data.name, sizeof(data.name), deviceName);
    copyText(data.type, sizeof(data.type), deviceType);
    copyText(data.arre, sizeof(data.arre), "-");
    copyText(data.status, sizeof(data.status), "未连接");
    data.state = false;
    data.stateKnown = false;
    data.connected = false;
}

void resetIrrGroupNames() {
    for (int i = 0; i < 128; ++i) {
        snprintf(sIrrGroupNames[i], sizeof(sIrrGroupNames[i]), "阀组[%d]", i + 1);
    }
}

int getNextAutoDeviceNameIndex(const char* prefix) {
    if (!prefix || prefix[0] == '\0') {
        return 1;
    }

    int maxIndex = 0;
    const size_t prefixLen = std::strlen(prefix);
    for (std::vector<SDATA>::const_iterator it = w2_DeviceDataList.begin();
         it != w2_DeviceDataList.end(); ++it) {
        if (std::strncmp(it->name, prefix, prefixLen) != 0) {
            continue;
        }

        const char* suffix = it->name + prefixLen;
        char* pEnd = NULL;
        const long value = std::strtol(suffix, &pEnd, 10);
        if ((pEnd != suffix) && pEnd && (*pEnd == '\0') &&
            (value > maxIndex) && (value <= MAX_DEVICE_COUNT)) {
            maxIndex = static_cast<int>(value);
        }
    }
    return maxIndex + 1;
}

}  // namespace

namespace DeviceDataStore {

void initDefaultDevices() {
    w2_DeviceDataList.clear();
    sNameIndex = 0;
    sSelectedEditIndex = -1;
    resetIrrGroupNames();

    for (int i = 0; i < DEFAULT_DEVICE_COUNT; ++i) {
        SDATA data;
        data.address = kDefaultDevices[i].address;
        copyText(data.name, sizeof(data.name), kDefaultDevices[i].name);
        copyText(data.type, sizeof(data.type), kDefaultDevices[i].type);
        copyText(data.arre, sizeof(data.arre), kDefaultDevices[i].arre);
        copyText(data.status, sizeof(data.status), "未连接");
        data.state = false;
        data.stateKnown = false;
        data.connected = false;
        w2_DeviceDataList.push_back(data);
    }
}

bool isDefaultDevice(int index) {
    return index >= 0 && index < DEFAULT_DEVICE_COUNT;
}

bool isCustomDevice(int index) {
    return index >= DEFAULT_DEVICE_COUNT && index < getDeviceCount();
}

bool isEmptyRow(int index) {
    return index == getDeviceCount();
}

int getDeviceCount() {
    return static_cast<int>(w2_DeviceDataList.size());
}

int getDeviceListRowCount() {
    return getDeviceCount() + 1;
}

int getCustomDeviceCount() {
    const int total = getDeviceCount();
    return total > DEFAULT_DEVICE_COUNT ? total - DEFAULT_DEVICE_COUNT : 0;
}

const char* getIrrGroupName(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return "";
    }
    return sIrrGroupNames[groupNo - 1];
}

bool setIrrGroupName(int groupNo, const char* name) {
    if (!isValidIrrGroupNo(groupNo)) {
        return false;
    }

    char targetName[sizeof(sIrrGroupNames[0])] = {0};
    if (name && name[0] != '\0') {
        copyText(targetName, sizeof(targetName), name);
    } else {
        snprintf(targetName, sizeof(targetName), "阀组[%d]", groupNo);
    }

    char* currentName = sIrrGroupNames[groupNo - 1];
    if (std::strcmp(currentName, targetName) == 0) {
        return false;
    }

    copyText(currentName, sizeof(sIrrGroupNames[0]), targetName);
    return true;
}

const SDATA* getDevice(int index) {
    if (index < 0 || index >= getDeviceCount()) {
        return NULL;
    }
    return &w2_DeviceDataList[index];
}

SDATA* getMutableDevice(int index) {
    if (index < 0 || index >= getDeviceCount()) {
        return NULL;
    }
    return &w2_DeviceDataList[index];
}

bool addDevice() {
    if (getDeviceCount() >= MAX_DEVICE_COUNT) {
        return false;
    }

    const int customCount = getCustomDeviceCount();
    const int nameIndex = sNameIndex % (sizeof(kSensorNames) / sizeof(kSensorNames[0]));
    ++sNameIndex;

    SDATA data;
    data.address = CUSTOM_DEVICE_START_ID + customCount;
    copyText(data.name, sizeof(data.name), kSensorNames[nameIndex]);
    copyText(data.type, sizeof(data.type), "传感器");
    copyText(data.arre, sizeof(data.arre), "地址3");
    copyText(data.status, sizeof(data.status), "未连接");
    data.state = false;
    data.stateKnown = false;
    data.connected = false;

    w2_DeviceDataList.push_back(data);
    return true;
}

bool addDevice(int address, const char* name, const char* type) {
    if (getDeviceCount() >= MAX_DEVICE_COUNT) {
        return false;
    }

    if (address <= 0) {
        address = CUSTOM_DEVICE_START_ID + getCustomDeviceCount();
    }

    SDATA data;
    fillCustomDevice(data, address, name, type);
    w2_DeviceDataList.push_back(data);
    return true;
}

bool editDevice(int index) {
    if (!isCustomDevice(index)) {
        return false;
    }

    SDATA* data = getMutableDevice(index);
    if (!data) {
        return false;
    }

    sSelectedEditIndex = index;
    const int nameIndex = sNameIndex % (sizeof(kSensorNames) / sizeof(kSensorNames[0]));
    ++sNameIndex;
    copyText(data->name, sizeof(data->name), kSensorNames[nameIndex]);
    return true;
}

bool updateDevice(int index, int address, const char* name, const char* type) {
    if (!isCustomDevice(index)) {
        return false;
    }

    SDATA* data = getMutableDevice(index);
    if (!data) {
        return false;
    }

    bool identityChanged = false;
    if ((address > 0) && (data->address != address)) {
        data->address = address;
        identityChanged = true;
    }
    if (name && name[0] != '\0' && std::strcmp(data->name, name) != 0) {
        copyText(data->name, sizeof(data->name), name);
    }
    if (type && type[0] != '\0' && std::strcmp(data->type, type) != 0) {
        copyText(data->type, sizeof(data->type), type);
        identityChanged = true;
    }
    if (identityChanged) {
        copyText(data->status, sizeof(data->status), "未连接");
        data->state = false;
        data->stateKnown = false;
        data->connected = false;
    }
    return true;
}

bool deleteDevice(int index) {
    if (!isCustomDevice(index)) {
        return false;
    }

    w2_DeviceDataList.erase(w2_DeviceDataList.begin() + index);
    if (sSelectedEditIndex == index) {
        sSelectedEditIndex = -1;
    } else if (sSelectedEditIndex > index) {
        --sSelectedEditIndex;
    }
    return true;
}

bool updateRuntimeStateByAddress(int address, bool connected, int decoderType,
                                 bool stateKnown, bool state) {
    bool changed = false;

    for (int i = 0; i < getDeviceCount(); ++i) {
        SDATA* data = getMutableDevice(i);
        if (!data || data->address != address) {
            continue;
        }

        const bool valve = decoderType == DEVICE_DECODER_TYPE_VALVE;
        const bool sensor = decoderType == DEVICE_DECODER_TYPE_SENSOR;
        const bool nextStateKnown = connected && valve && stateKnown;
        const bool nextState = nextStateKnown && state;
        const char* nextStatus = !connected ? "未连接" :
                                 (valve ? (!nextStateKnown ? "状态未知" :
                                           (nextState ? "打开" : "关闭")) : "已连接");
        const char* nextType = valve ? "电磁阀" : (sensor ? "传感器" : data->type);

        if ((data->connected != connected) ||
            (data->stateKnown != nextStateKnown) ||
            (data->state != nextState) ||
            (std::strcmp(data->status, nextStatus) != 0) ||
            (std::strcmp(data->type, nextType) != 0)) {
            data->connected = connected;
            data->stateKnown = nextStateKnown;
            data->state = nextState;
            copyText(data->status, sizeof(data->status), nextStatus);
            copyText(data->type, sizeof(data->type), nextType);
            changed = true;
        }
    }

    return changed;
}

bool syncDiscoveredDevice(int address, int decoderType, bool stateKnown, bool state,
                          bool *pAdded) {
    if (pAdded) {
        *pAdded = false;
    }
    if ((address < CUSTOM_DEVICE_START_ID) || (address > 255) ||
        ((decoderType != DEVICE_DECODER_TYPE_VALVE) &&
         (decoderType != DEVICE_DECODER_TYPE_SENSOR))) {
        return false;
    }

    for (int i = 0; i < getDeviceCount(); ++i) {
        const SDATA* data = getDevice(i);
        if (data && data->address == address) {
            return updateRuntimeStateByAddress(address, true, decoderType,
                                               stateKnown, state);
        }
    }

    if (getDeviceCount() >= MAX_DEVICE_COUNT) {
        return false;
    }

    const char* typeText = (decoderType == DEVICE_DECODER_TYPE_VALVE) ?
                           "电磁阀" : "传感器";
    char name[sizeof(SDATA::name)] = {0};
    snprintf(name, sizeof(name), "%s%d", typeText,
             getNextAutoDeviceNameIndex(typeText));
    if (!addDevice(address, name, typeText)) {
        return false;
    }

    if (pAdded) {
        *pAdded = true;
    }
    (void)updateRuntimeStateByAddress(address, true, decoderType,
                                      stateKnown, state);
    return true;
}

bool bindDeviceToIrrGroup(int index, int groupNo) {
    if (groupNo <= 0 || groupNo > 128 || isEmptyRow(index)) {
        return false;
    }

    SDATA* data = getMutableDevice(index);
    if (!data) {
        return false;
    }

    char groupText[sizeof(data->arre)] = {0};
    snprintf(groupText, sizeof(groupText), "%d", groupNo);
    copyText(data->arre, sizeof(data->arre), groupText);
    return true;
}

bool bindDeviceAddressToIrrGroup(int address, int groupNo) {
    if (address <= 0) {
        return false;
    }

    for (int i = 0; i < getDeviceCount(); ++i) {
        SDATA* data = getMutableDevice(i);
        if (data && data->address == address) {
            return bindDeviceToIrrGroup(i, groupNo);
        }
    }
    return false;
}

bool unbindDeviceFromIrrGroup(int index) {
    SDATA* data = getMutableDevice(index);
    if (!data || std::strcmp(data->arre, "-") == 0) {
        return false;
    }
    copyText(data->arre, sizeof(data->arre), "-");
    return true;
}

bool clearIrrGroup(int groupNo) {
    if (groupNo <= 0 || groupNo > 128) {
        return false;
    }

    char groupText[16] = {0};
    snprintf(groupText, sizeof(groupText), "%d", groupNo);

    bool changed = false;
    for (std::vector<SDATA>::iterator it = w2_DeviceDataList.begin();
         it != w2_DeviceDataList.end(); ++it) {
        if (std::strcmp(it->arre, groupText) == 0) {
            copyText(it->arre, sizeof(it->arre), "-");
            changed = true;
        }
    }
    return changed;
}

bool removeIrrGroup(int groupNo) {
    if (!isValidIrrGroupNo(groupNo)) {
        return false;
    }

    bool changed = false;
    for (std::vector<SDATA>::iterator it = w2_DeviceDataList.begin();
         it != w2_DeviceDataList.end(); ++it) {
        int dataGroupNo = atoi(it->arre);
        if (dataGroupNo == groupNo) {
            copyText(it->arre, sizeof(it->arre), "-");
            changed = true;
        }
    }
    snprintf(sIrrGroupNames[groupNo - 1], sizeof(sIrrGroupNames[groupNo - 1]),
             "阀组[%d]", groupNo);
    return changed;
}

}  // namespace DeviceDataStore
