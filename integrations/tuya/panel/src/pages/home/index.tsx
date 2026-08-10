import React, { useState } from 'react';
import { Text, View, device, showToast } from '@ray-js/ray';
import { NavBar, Button } from '@ray-js/smart-ui';
import { hooks, useDevInfo } from '@ray-js/panel-sdk';
import styles from './index.module.less';

const CJ96_DP_ID = 101;
const SCREEN_SLEEP_FRAME = 'AA55F00155AA';
const SCREEN_WAKE_FRAME = 'AA55F00255AA';
const SCREEN_SLEEP_STATE_FRAME = 'AA55F01155AA';
const SCREEN_WAKE_STATE_FRAME = 'AA55F01255AA';
const ROUND_IRRIGATION_ON_FRAME = 'AA55F10155AA';
const ROUND_IRRIGATION_OFF_FRAME = 'AA55F10255AA';
const TEST_VERSION = '测试版 1.0.6';

function formatCallbackValue(value: any): string {
  if (typeof value === 'string') return value;
  try {
    const json = JSON.stringify(value);
    if (json && json !== '{}') return json;
  } catch (error) {
    console.log('format callback value failed', error);
  }
  return String(value?.errorMsg || value?.message || value?.errorCode || value?.code || value);
}

export function Home() {
  const { publishDpsBase } = device;
  const devInfo = useDevInfo();
  const reportedFrame = hooks.useDpValue<string>('cj96_raw');
  const deviceId = devInfo?.devId;
  const [sending, setSending] = useState(false);
  const [callbackResult, setCallbackResult] = useState('尚未下发');
  const normalizedReportedFrame = String(reportedFrame || '').toUpperCase();
  const screenSleeping = normalizedReportedFrame === SCREEN_SLEEP_STATE_FRAME;
  const screenAwake = normalizedReportedFrame === SCREEN_WAKE_STATE_FRAME;

  const publishCommandFrame = (value: string, actionName: string) => {
    if (!deviceId) {
      console.error('publishDpsBase failed: missing deviceId');
      setCallbackResult('fail: 设备ID无效');
      showToast({ title: '设备ID无效', icon: 'none' });
      return;
    }

    setSending(true);
    setCallbackResult(`正在下发${actionName}命令...`);

    try {
      publishDpsBase({
        deviceId,
        dps: { [CJ96_DP_ID]: value },
        mode: 1,
        pipelines: [0, 1, 2, 3, 4, 5, 6],
        options: {},
        success: (result: any) => {
          console.log('publishDpsBase success', result);
          setSending(false);
          setCallbackResult(`success: ${formatCallbackValue(result)}`);
          showToast({ title: `${actionName}下发成功`, icon: 'success' });
        },
        fail: (error: any) => {
          console.log('publishDpsBase fail', error);
          setSending(false);
          setCallbackResult(`fail: ${formatCallbackValue(error)}`);
          showToast({ title: error?.errorMsg || '下发失败', icon: 'none' });
        },
      });
    } catch (error) {
      console.error('publishDpsBase throw', error);
      setSending(false);
      setCallbackResult(`throw: ${formatCallbackValue(error)}`);
      showToast({ title: '下发失败', icon: 'none' });
    }
  };

  return (
    <>
      <NavBar leftText="CJ96调试" leftTextType="home" />
      <View className={styles.view}>
        <View className={styles.panel}>
          <View className={styles.headerRow}>
            <Text className={styles.label}>CJ96协议帧</Text>
            <Text className={styles.version}>{TEST_VERSION}</Text>
          </View>
          <Text className={styles.label}>设备ID</Text>
          <Text className={styles.reportedValue}>{deviceId || '无'}</Text>

          <Text className={styles.label}>屏幕控制</Text>
          <View className={styles.actions}>
            <Button
              type={screenSleeping ? 'primary' : 'default'}
              loading={sending}
              disabled={sending}
              onClick={() => publishCommandFrame(SCREEN_SLEEP_FRAME, '屏幕休眠')}
            >
              屏幕休眠
            </Button>
            <Button
              type={screenAwake ? 'primary' : 'default'}
              disabled={sending}
              onClick={() => publishCommandFrame(SCREEN_WAKE_FRAME, '屏幕唤醒')}
            >
              屏幕唤醒
            </Button>
          </View>

          <Text className={styles.label}>轮灌控制</Text>
          <View className={styles.actions}>
            <Button
              type="primary"
              loading={sending}
              disabled={sending}
              onClick={() => publishCommandFrame(ROUND_IRRIGATION_ON_FRAME, '轮灌开启')}
            >
              轮灌开启
            </Button>
            <Button
              disabled={sending}
              onClick={() => publishCommandFrame(ROUND_IRRIGATION_OFF_FRAME, '轮灌关闭')}
            >
              轮灌关闭
            </Button>
          </View>

          <Text className={styles.label}>回调结果</Text>
          <Text className={styles.tip}>{callbackResult}</Text>
        </View>
      </View>
    </>
  );
}

export default Home;
