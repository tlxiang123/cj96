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
    const char* status;
    bool state;
};

const DefaultDeviceConfig kDefaultDevices[] = {
    {1,  "水泵1",      "水泵",   "-", "在线", false},
    {2,  "水泵2",      "水泵",   "-", "离线", false},
    {3,  "水泵3",      "水泵",   "-", "在线", false},
    {4,  "水泵4",      "水泵",   "-", "离线", false},
    {5,  "水泵5",      "水泵",   "-", "在线", false},
    {6,  "湿度传感器", "传感器", "-", "在线", false},
    {8,  "雨量传感器", "传感器", "-", "离线", false},
    {9,  "水压表",     "传感器", "-", "在线", false},
    {10, "流量表",     "传感器", "-", "在线", false},
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
    copyText(data.status, sizeof(data.status), "在线");
    data.state = false;
}

void resetIrrGroupNames() {
    for (int i = 0; i < 128; ++i) {
        snprintf(sIrrGroupNames[i], sizeof(sIrrGroupNames[i]), "阀组[%d]", i + 1);
    }
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
        copyText(data.status, sizeof(data.status), kDefaultDevices[i].status);
        data.state = kDefaultDevices[i].state;
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
    copyText(data.status, sizeof(data.status), "在线");
    data.state = false;

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

    if (address > 0) {
        data->address = address;
    }
    if (name && name[0] != '\0') {
        copyText(data->name, sizeof(data->name), name);
    }
    if (type && type[0] != '\0') {
        copyText(data->type, sizeof(data->type), type);
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

bool toggleDeviceState(int index) {
    if (index < 0 || index >= getDeviceCount()) {
        return false;
    }

    SDATA* data = getMutableDevice(index);
    if (!data) {
        return false;
    }

    data->state = !data->state;
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
        } else if (dataGroupNo > groupNo) {
            char groupText[sizeof(it->arre)] = {0};
            snprintf(groupText, sizeof(groupText), "%d", dataGroupNo - 1);
            copyText(it->arre, sizeof(it->arre), groupText);
            changed = true;
        }
    }
    for (int i = groupNo - 1; i < 127; ++i) {
        copyText(sIrrGroupNames[i], sizeof(sIrrGroupNames[i]), sIrrGroupNames[i + 1]);
    }
    snprintf(sIrrGroupNames[127], sizeof(sIrrGroupNames[127]), "阀组[%d]", 128);
    return changed;
}

}  // namespace DeviceDataStore
