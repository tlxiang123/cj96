#ifndef LOGIC_DEVICEDATASTORE_H_
#define LOGIC_DEVICEDATASTORE_H_

#include "mainLogic.h"

namespace DeviceDataStore {

void initDefaultDevices();

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
bool syncDiscoveredDevice(int address, int decoderType, bool stateKnown, bool state,
                          bool *pAdded);
bool bindDeviceToIrrGroup(int index, int groupNo);
bool bindDeviceAddressToIrrGroup(int address, int groupNo);
bool clearIrrGroup(int groupNo);
bool removeIrrGroup(int groupNo);

}  // namespace DeviceDataStore

#endif /* LOGIC_DEVICEDATASTORE_H_ */
