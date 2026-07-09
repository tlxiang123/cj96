

/*
 * mainLogic.h
 *
 *  Created on: 2026年6月15日
 *      Author: Administrator
 */
#ifndef LOGIC_MAINLOGIC_H_
#define LOGIC_MAINLOGIC_H_

#include <vector>

#define BACK_GROUND_BTN_1    1
#define BACK_GROUND_BTN_2    2
#define BACK_GROUND_BTN_3    3
#define BACK_GROUND_BTN_4    4
#define BACK_GROUND_BTN_5    5
#define BACK_GROUND_BTN_6    6
#define BACK_GROUND_BTN_7    7
#define BACK_GROUND_BTN_8    8


typedef struct data{
	int address;
	char type[20];
	char name[20];
	char arre[20];
	char status[20];
	bool state;
}SDATA;

// 出厂设备数量（地址1-6、8-10固定设备）
#define DEFAULT_DEVICE_COUNT 9

// 手动添加设备的起始ID
#define CUSTOM_DEVICE_START_ID 20

// 最大设备数量限制
#define MAX_DEVICE_COUNT 120


extern std::vector<SDATA> w2_DeviceDataList;


#endif /* LOGIC_MAINLOGIC_H_ */
