import { GlobalConfig } from '@ray-js/types';

export const tuya = {
  themeLocation: 'theme.json',
  darkmode: 'auto',
  window: {
    pageOrientation: 'landscape',
    hideMenuButton: true,
    backgroundColor: '#000000',
    navigationBarTitleText: '',
    navigationBarBackgroundColor: '--app-B2',
    navigationBarTextStyle: 'black',
  },
  functionalPages: {
    // 设备详情功能页，若未自定义实现设备详情界面，该项为必填配置，不可删除。
    settings: {
      appid: 'tycryc71qaug8at6yt',
      entryCode: 'entrye0n05idydmmfv',
    },
  },
};

const globalConfig: GlobalConfig = {
  basename: '',
};

export default globalConfig;
