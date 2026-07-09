#pragma once
#include "uart/ProtocolSender.h"
#include "DisplayPowerManager.h"

#define DISPLAY_POWER_TIMER_ID 100
/*
*此文件由GUI工具生成
*文件功能：用于处理用户的逻辑相应代码
*功能说明：
*========================onButtonClick_XXXX
当页面中的按键按下后系统会调用对应的函数，XXX代表GUI工具里面的[ID值]名称，
如Button1,当返回值为false的时候系统将不再处理这个按键，返回true的时候系统将会继续处理此按键。比如SYS_BACK.
*========================onSlideWindowItemClick_XXXX(int index) 
当页面中存在滑动窗口并且用户点击了滑动窗口的图标后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
如slideWindow1;index 代表按下图标的偏移值
*========================onSeekBarChange_XXXX(int progress) 
当页面中存在滑动条并且用户改变了进度后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
如SeekBar1;progress 代表当前的进度值
*========================ogetListItemCount_XXXX() 
当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表的总数目,XXX代表GUI工具里面的[ID值]名称，
如List1;返回值为当前列表的总条数
*========================oobtainListItemData_XXXX(ZKListView::ZKListItem *pListItem, int index)
 当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表当前条目下的内容信息,XXX代表GUI工具里面的[ID值]名称，
如List1;pListItem 是贴图中的单条目对象，index是列表总目的偏移量。具体见函数说明
*========================常用接口===============
*LOGD(...)  打印调试信息的接口
*mTextXXXPtr->setText("****") 在控件TextXXX上显示文字****
*mButton1Ptr->setSelected(true); 将控件mButton1设置为选中模式，图片会切换成选中图片，按钮文字会切换为选中后的颜色
*mSeekBarPtr->setProgress(12) 在控件mSeekBar上将进度调整到12
*mListView1Ptr->refreshListView() 让mListView1 重新刷新，当列表数据变化后调用
*mDashbroadView1Ptr->setTargetAngle(120) 在控件mDashbroadView1上指针显示角度调整到120度
*
* 在Eclipse编辑器中  使用 “alt + /”  快捷键可以打开智能提示
*/
#include "control/ZKTextView.h"
#include <string>
#include "net/NetManager.h"
#include "utils/Log.h"

#define ETHERNETMANAGER		NETMANAGER->getEthernetManager()

using namespace std;

//地址总长
#define ADDRLEN 15

//定义字符串数组保存IP地址、掩码地址、网关地址、首选DNS服务器地址以及备选DNS服务器地址
static string addr[5];

/**
 * 字符长度是否正确
 * */
const bool isLengthCorrect(const string addr)
{
	return addr.size() <= ADDRLEN;
}

/**
 * 检查是不是最多只有3个.字符
 * */
const bool noMoreThan3charactor(const string addr)
{
	//.字符个数
	int count = 0;
	for(unsigned int level1 = 0; level1 < addr.size(); ++level1)
	{
		if('.' == addr[level1])
		{
			++count;
		}
	}
	return 3 >= count;
}

/*
 * 判断所有字符是不是由0~9和.组成
 * */
const bool allCharacterIsLegal(const string addr)
{
	for(unsigned int level1 = 0; level1 < addr.size(); ++level1)
	{
		if(false == (('0' <= addr[level1] && addr[level1] <= '9') || '.' == addr[level1]))
		{
			return false;
		}
	}
	return true;
}

/**
 * 判断开头是否是1`9
 * */
const bool headIsCorrect(string addr, unsigned int pos = 0)
{
	//查找到达末尾，直接返回true
	if(addr.size() == pos)
	{
		return true;
	}
	//查找.字符
	if(string::npos == addr.find('.', pos))
	{
		if('0' < addr[pos] && addr[pos] <= '9')
		{
			return true;
		}
		//如果开头为字符'0'且其后没有字符，那么返回true
		else if(addr.size()-1 == pos)
		{
			return true;
		}
		return false;
	}
	else
	{
		//开头为0~9
		if('0' > addr[pos] || addr[pos] > '9')
		{
			return false;
		}
		//开头为0，后面没有字符返回false
		if('0' == addr[pos] && addr.size()-1 <= pos)
		{
			return false;
		}
		//跳过.字符继续查找
		return headIsCorrect(addr, addr.find('.', pos)+1);
	}
	return true; //默认返回true
}

/**
 * 检查地址每一小段的长度，最长为3个字符
 * */
const bool partCheractorCorrect(const string addr, unsigned int pos = 0)
{
	//查找到达末尾，直接返回true
	if(addr.size() == pos)
	{
		return true;
	}
	//查找.字符
	if(string::npos == addr.find('.', pos))
	{
		if(3 >= addr.size()-pos)
		{
			return true;
		}
		return false;
	}
	else
	{
		//由于跳过了.字符进行查找，因此判断条件应该是>3
		if(3 < addr.find('.', pos) - pos)
		{
			return false;
		}
		//跳过.字符继续查找
		return partCheractorCorrect(addr, addr.find('.', pos)+1);
	}
	return true; //默认返回true
}

/**
 * 检测地址是否有误
 * */
const bool checkAddr(string addr)
{
	return  (true == headIsCorrect(addr))?			//地址每小段开头是否为0~9
			(true == partCheractorCorrect(addr))?	//每小段字符串最大长度是否为3
			(true == noMoreThan3charactor(addr))?	//最多是否只有3个.字符
			(true == isLengthCorrect(addr))?		//地址最大长度是否为15
			(true == allCharacterIsLegal(addr))?	//所有的字符是否都由0~9和.组成
			true: false: false: false: false: false;

}

/**
 * 同步地址
 * */
void synAddr(const string &text, string& addr, ZKTextView* ctrPtr)
{
	//内容没发生更改直接break
	if(text == addr){return;}
	if(true == checkAddr(text))
	{
		//检测输入正确，同步地址
		addr = text;
		return;
	}
	//当前输入的地址有误，回退至先前输入的地址
	ctrPtr->setText(addr);
}

/**
 * @brief 文本内容改变监听接口
 */
class textChangeListener : public ZKTextView::ITextChangeListener {
public:
	virtual ~textChangeListener() { }
	virtual void onTextChanged(ZKTextView *pTextView, const std::string &text)
	{
		switch(pTextView->getID())
		{
		case ID_ETHERNETSETTING_ipAddrInput: //IP地址输入框
			synAddr(text, addr[0], mipAddrInputPtr);
			break;
		case ID_ETHERNETSETTING_maskInput: //掩码地址输入框
			synAddr(text, addr[1], mmaskInputPtr);
			break;
		case ID_ETHERNETSETTING_gateWayInput: //网关输入框
			synAddr(text, addr[2], mgateWayInputPtr);
			break;
		case ID_ETHERNETSETTING_firstDNSInput: //首选DNS服务器输入框
			synAddr(text, addr[3], mfirstDNSInputPtr);
			break;
		case ID_ETHERNETSETTING_secondDNSInput: //备用DNS服务器输入框
			synAddr(text, addr[4], msecondDNSInputPtr);
			break;
		}
	}
};
//实例化文本内容改变监听接口
static textChangeListener myTextChangeListener;

/**
 * 注册/注销按键监听事件
 * */
void loginListenEvent(const bool isLogin)
{
	mipAddrInputPtr->setTextChangeListener(((isLogin)? &myTextChangeListener:NULL));
	mmaskInputPtr->setTextChangeListener(((isLogin)? &myTextChangeListener:NULL));
	mgateWayInputPtr->setTextChangeListener(((isLogin)? &myTextChangeListener:NULL));
	mfirstDNSInputPtr->setTextChangeListener(((isLogin)? &myTextChangeListener:NULL));
	msecondDNSInputPtr->setTextChangeListener(((isLogin)? &myTextChangeListener:NULL));
}

/**
 * 注册定时器
 * 填充数组用于注册定时器
 * 注意：id不能重复
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{DISPLAY_POWER_TIMER_ID,  1000},
};

/**
 * 当界面构造时触发
 */
static void onUI_init(){
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	DisplayPowerManager::syncFromContext();
	/**
	 * 注册/注销按键监听事件
	 * */
	loginListenEvent(true);
}

/**
 * 当切换到该界面时触发
 */
static void onUI_intent(const Intent *intentPtr) {
    if (intentPtr != NULL) {
        //TODO
    }
}

/*
 * 当界面显示时触发
 */
static void onUI_show() {
	//获取网络连接模式
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
	bool connectMode = ETHERNETMANAGER->isAutoMode();
	if(true == connectMode) //自动获取IP模式
	{
		mDYNAMIC_IP_BUTTONPtr->setSelected(true); //将自动获取IP按钮设为选中状态
		mSTATIC_IP_BUTTONPtr->setSelected(false); //将静态IP按钮设为非选中状态

	    mipAddressPtr->setText(ETHERNETMANAGER->getIp()); //设置IP地址
	    mmacAddressPtr->setText(ETHERNETMANAGER->getMacAddr()); //设置MAC地址
	    mmenuTipWinPtr->showWnd(); //显示提示窗口
	    mstaticIPSetUpWinPtr->hideWnd(); //隐藏静态IP地址配置窗口
	    if (ETHERNETMANAGER->isConnected()) {
	    	mNetStatusPtr->setText("已连接");
	    } else {
	    	mNetStatusPtr->setText("未连接");
	    }


	}
	else if(false == connectMode) //静态IP模式
	{
		mSTATIC_IP_BUTTONPtr->setSelected(true); //将静态IP按钮设为选中状态
		mDYNAMIC_IP_BUTTONPtr->setSelected(false); //将自动获取IP按钮设为非选中状态
		mstaticIPSetUpWinPtr->showWnd(); //显示静态IP地址配置窗口
	}
	LOGD("connectMode = %d", connectMode);
#endif
}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {

}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {
	/**
	 * 注册/注销按键监听事件
	 * */
	loginListenEvent(false);
}

/**
 * 串口数据回调接口
 */
static void onProtocolDataUpdate(const SProtocolData &data) {

}

/**
 * 定时器触发函数
 * 不建议在此函数中写耗时操作，否则将影响UI刷新
 * 参数： id
 *         当前所触发定时器的id，与注册时的id相同
 * 返回值: true
 *             继续运行当前定时器
 *         false
 *             停止运行当前定时器
 */
static bool onUI_Timer(int id){
	switch (id) {
		case DISPLAY_POWER_TIMER_ID:
			return DisplayPowerManager::onOneSecondTimer();

		default:
			break;
	}
    return true;
}

/**
 * 有新的触摸事件时触发
 * 参数：ev
 *         新的触摸事件
 * 返回值：true
 *            表示该触摸事件在此被拦截，系统不再将此触摸事件传递到控件上
 *         false
 *            触摸事件将继续传递到控件上
 */
static bool onethernetsettingActivityTouchEvent(const MotionEvent &ev) {
    if (DisplayPowerManager::handleTouchEvent()) {
        return true;
    }

    switch (ev.mActionStatus) {
		case MotionEvent::E_ACTION_DOWN://触摸按下
			//LOGD("时刻 = %ld 坐标  x = %d, y = %d", ev.mEventTime, ev.mX, ev.mY);
			break;
		case MotionEvent::E_ACTION_MOVE://触摸滑动
			break;
		case MotionEvent::E_ACTION_UP:  //触摸抬起
			break;
		default:
			break;
	}
	return false;
}
static bool onButtonClick_SAVE_BUTTON(ZKButton *pButton) {
    LOGD(" ButtonClick SAVE_BUTTON !!!\n");
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
	bool ret = false;
	if (mDYNAMIC_IP_BUTTONPtr->isSelected()) {
		ret = ETHERNETMANAGER->setAutoMode(true);
	} else {
//		LOGD("IP：%s \n子网掩码：%s\n默认网关：%s\n首选DNS服务器：%s\n备用DNS服务器：%s", addr[0].c_str(), addr[1].c_str(),
//				addr[2].c_str(), addr[3].c_str(), addr[4].c_str());
		ret = ETHERNETMANAGER->configure(addr[0].c_str(), addr[1].c_str(), addr[2].c_str(), addr[3].c_str(), addr[4].c_str());
	}
	msaveTipPtr->setText(ret ? "设置成功" : "设置失败");
	msaveTipWinPtr->showWnd();
#endif
    return false;
}

static bool onButtonClick_DYNAMIC_IP_BUTTON(ZKButton *pButton) {
    LOGD(" ButtonClick DYNAMIC_IP_BUTTON !!!\n");
    pButton->setSelected(true); //将自动获取IP按钮设为选中状态
    mSTATIC_IP_BUTTONPtr->setSelected(!pButton->isSelected()); //将静态IP按钮设为非选中状态

    mipAddressPtr->setText(ETHERNETMANAGER->getIp()); //设置IP地址
    mmacAddressPtr->setText(ETHERNETMANAGER->getMacAddr()); //设置MAC地址
    mmenuTipWinPtr->showWnd(); //显示提示窗口
    mstaticIPSetUpWinPtr->hideWnd(); //隐藏静态IP地址配置窗口
    return false;
}

static bool onButtonClick_STATIC_IP_BUTTON(ZKButton *pButton) {
    LOGD(" ButtonClick STATIC_IP_BUTTON !!!\n");
#if !(__PLATFORM_Z6S__ || __PLATFORM_A33NOR__)
    pButton->setSelected(true); //将静态IP按钮设为选中状态
    mDYNAMIC_IP_BUTTONPtr->setSelected(!pButton->isSelected()); //将自动获取IP按钮设为非选中状态

    char ipAddr[32] = {0};
    char mask[32] = {0};
    char gateway[32] = {0};
    char dns1[32] = {0};
    char dns2[32] = {0};
    ETHERNETMANAGER->getStaticConfigureInfo(ipAddr, mask, gateway, dns1, dns2); //获取静态IP配置信息

	mipAddrInputPtr->setText(ipAddr);
	mmaskInputPtr->setText(mask);
	mgateWayInputPtr->setText(gateway);
	mfirstDNSInputPtr->setText(dns1);
	msecondDNSInputPtr->setText(dns2);

    mstaticIPSetUpWinPtr->showWnd(); //显示静态IP地址配置窗口
    mmenuTipWinPtr->hideWnd();
#endif
    return false;
}

static bool onButtonClick_sys_back(ZKButton *pButton) {
    LOGD(" ButtonClick sys_back !!!\n");
    return false;
}

static void onEditTextChanged_ipAddrInput(const std::string &text) {
    //LOGD(" onEditTextChanged_ ipAddrInput %s !!!\n", text.c_str());
}

static void onEditTextChanged_maskInput(const std::string &text) {
    //LOGD(" onEditTextChanged_ maskInput %s !!!\n", text.c_str());
}

static void onEditTextChanged_gateWayInput(const std::string &text) {
    //LOGD(" onEditTextChanged_ gateWayInput %s !!!\n", text.c_str());
}

static void onEditTextChanged_firstDNSInput(const std::string &text) {
    //LOGD(" onEditTextChanged_ firstDNSInput %s !!!\n", text.c_str());
}

static void onEditTextChanged_secondDNSInput(const std::string &text) {
    //LOGD(" onEditTextChanged_ secondDNSInput %s !!!\n", text.c_str());
}
