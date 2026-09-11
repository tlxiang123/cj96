#ifndef LOGIC_DEVICEDATASTORE_H_
#define LOGIC_DEVICEDATASTORE_H_

#include "mainLogic.h"
#include <string>

namespace DeviceDataStore {

void initDefaultDevices();
void appendPersistentConfigText(std::string& text);
bool loadPersistentConfigText(const std::string& text);

bool isDefaultDevice(int index);
bool isCustomDevice(int index);
bool isEmptyRow(int index);

int getDeviceCount();
int getDeviceListRowCount();
int getCustomDeviceCount();
const char* getIrrGroupName(int groupNo);
bool setIrrGroupName(int groupNo, const char* name);

const SDATA* getDevice(int index);
SDATA* getMutableDevice(int index);

bool addDevice();
bool addDevice(int address, const char* name, const char* type);
bool editDevice(int index);
bool updateDevice(int index, int address, const char* name, const char* type);
bool deleteDevice(int index);
bool updateRuntimeStateByAddress(int address, bool connected, int decoderType,
                                 bool stateKnown, bool state);
bool updateRuntimeSensorStatusByAddress(int address, bool connected,
                                        const char* status);
bool syncDiscoveredDevice(int address, int decoderType, bool stateKnown, bool state,
                           bool *pAdded);
bool isDeviceBoundToIrrGroup(const SDATA* data, int groupNo);
bool isDeviceBoundToAnyIrrGroup(const SDATA* data);
bool bindDeviceToIrrGroup(int index, int groupNo);
bool bindDeviceToAllIrrGroups(int index);
bool bindDeviceAddressToIrrGroup(int address, int groupNo);
bool setDeviceIrrGroupText(int index, const char* groupText);
bool unbindDeviceFromIrrGroup(int index);
bool unbindDeviceFromIrrGroup(int index, int groupNo);
bool clearIrrGroup(int groupNo);
bool removeIrrGroup(int groupNo);

}  // namespace DeviceDataStore

#endif /* LOGIC_DEVICEDATASTORE_H_ */
