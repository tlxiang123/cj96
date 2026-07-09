#pragma once
#include "uart/ProtocolSender.h"
//#include "feature.h"
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
#include "control/ZKBase.h"
#include "control/ZKSlideText.h"
#include "manager/ConfigManager.h"
#include "pinyin/pinyinime.h"
#include "pinyin/ConvertUTF.h"
#include "ime/IMEContext.h"
#include <stdio.h>
//#include "net/NetManager.h"
//#define WIFIMANAGER NETMANAGER->getWifiManager()


#define ID_IME_BUTTON_SPACE    20033

using namespace std;
using namespace ime_pinyin;

#define MAX_SEARCH_HANZI		256
#define MAX_PINYIN_LEN			64

static bool toUTF8(UTF8 *dest, size_t destLen, const char16 *src, size_t srcLen) {
	UTF16 *utf16Start = (UTF16 *) src;
	UTF16 *utf16End = (UTF16 *) (src + srcLen);

	return ConvertUTF16toUTF8((const UTF16 **) &utf16Start, utf16End, &dest,
			dest + destLen - 1, strictConversion) == conversionOK;
}

static std::string sContentStr;

enum {
	E_KEYPAD_CHAR,
	E_KEYPAD_UPERCHAR,
	E_KEYPAD_NUMCHAR,
	E_KEYPAD_SYMBOL
} sKeypadType;


enum {
	E_LANG_ENGLISH,
	E_LANG_CHINESE
} mLangType;

bool mIsIMOpenDecoderOK;

std::string mPinyinStr;

static bool isFileReadable(const std::string &path) {
	if (path.empty()) {
		return false;
	}

	FILE *fp = fopen(path.c_str(), "rb");
	if (!fp) {
		return false;
	}

	fclose(fp);
	return true;
}

static bool openPinyinDecoder() {
	im_close_decoder();

	std::string dictPath = CONFIGMANAGER->getResFilePath("ime/dict_pinyin.dat");
	if (isFileReadable(dictPath)) {
		bool ok = im_open_decoder(dictPath.c_str(), "/data/user.dic");
		LOGD("[UserIme] open pinyin dict=%s ok=%d\n", dictPath.c_str(), ok);
		if (ok) {
			return true;
		}
	}

	const char *fallbackPaths[] = {
		CONFIGMANAGER->getDictPinyinPath().c_str(),
		"/data/easyui/resources/ime/dict_pinyin.dat",
		"/data/resources/ime/dict_pinyin.dat",
		"/usr/share/easyui/resources/ime/dict_pinyin.dat",
		"/usr/share/easyui/ime/dict_pinyin.dat",
		NULL
	};

	for (int i = 0; fallbackPaths[i] != NULL; ++i) {
		const char *path = fallbackPaths[i];
		if (!path || !path[0] || !isFileReadable(path)) {
			continue;
		}

		bool ok = im_open_decoder(path, "/data/user.dic");
		LOGD("[UserIme] open pinyin fallback dict=%s ok=%d\n", path, ok);
		if (ok) {
			return true;
		}
	}

	LOGD("[UserIme] open pinyin decoder failed, resPath=%s cfgPath=%s\n",
			dictPath.c_str(), CONFIGMANAGER->getDictPinyinPath().c_str());
	return false;
}

#if defined(FLYTHINGS_API_1_0)
static IMEContext::SIMETextInfo pinfo;
#else
static IMEContext::SIMETextInfo *pinfo;
#endif

typedef struct {
	int id;
	const char *ch;
	const char *upch;
	const char *numch;
	const char *symb;
} SKeypadInfo;

typedef struct {
	int id;
	char ch;
} SNumKeypadInfo;

// 英文键盘
static const SKeypadInfo sEnglishKeypadInfoTab[] = {
		ID_USERIME_BUTTON_Q, "q", "Q", "1", "[",
		ID_USERIME_BUTTON_W, "w", "W", "2", "]",
		ID_USERIME_BUTTON_E, "e", "E", "3", "{",
		ID_USERIME_BUTTON_R, "r", "R", "4", "}",
		ID_USERIME_BUTTON_T, "t", "T", "5", "#",
		ID_USERIME_BUTTON_Y, "y", "Y", "6", "%",
		ID_USERIME_BUTTON_U, "u", "U", "7", "^",
		ID_USERIME_BUTTON_I, "i", "I", "8", "*",
		ID_USERIME_BUTTON_O, "o", "O", "9", "+",
		ID_USERIME_BUTTON_P, "p", "P", "0", "=",

		ID_USERIME_BUTTON_A, "a", "A", "-", "_",
		ID_USERIME_BUTTON_S, "s", "S", "/", "\\",
		ID_USERIME_BUTTON_D, "d", "D", ":", "|",
		ID_USERIME_BUTTON_F, "f", "F", ";", "~",
		ID_USERIME_BUTTON_G, "g", "G", "(", "<",
		ID_USERIME_BUTTON_H, "h", "H", ")", ">",
		ID_USERIME_BUTTON_J, "j", "J", "$", "$",
		ID_USERIME_BUTTON_K, "k", "K", "&", "&",
		ID_USERIME_BUTTON_L, "l", "L", "@", "@",

		ID_USERIME_BUTTON_Z, "z", "Z", ".", ".",
		ID_USERIME_BUTTON_X, "x", "X", ",", ",",
		ID_USERIME_BUTTON_C, "c", "C", "?", "?",
		ID_USERIME_BUTTON_V, "v", "V", "!", "!",
		ID_USERIME_BUTTON_B, "b", "B", "\'", "\'",
		ID_USERIME_BUTTON_N, "n", "N", "\"", "\"",
		ID_USERIME_BUTTON_M, "m", "M", "`", "`",

		ID_USERIME_BUTTON_COMMA, ",", "!", NULL, NULL,
		ID_USERIME_BUTTON_DOT, ".", "?", NULL, NULL,

		ID_USERIME_BUTTON_SPACE, " ", " ", " ", " ",
		ID_USERIME_BUTTON_ENTER, "\r", "\r", "\r", "\r",
		0, NULL, NULL, NULL, NULL
};

// 中文键盘 (本样例暂不支持中文键盘)
static const SKeypadInfo sChineseKeypadInfoTab[] = {
	ID_USERIME_BUTTON_Q, "q", "Q", "1", "【",
	ID_USERIME_BUTTON_W, "w", "W", "2", "】",
	ID_USERIME_BUTTON_E, "e", "E", "3", "{",
	ID_USERIME_BUTTON_R, "r", "R", "4", "}",
	ID_USERIME_BUTTON_T, "t", "T", "5", "#",
	ID_USERIME_BUTTON_Y, "y", "Y", "6", "%",
	ID_USERIME_BUTTON_U, "u", "U", "7", "-",
	ID_USERIME_BUTTON_I, "i", "I", "8", "*",
	ID_USERIME_BUTTON_O, "o", "O", "9", "+",
	ID_USERIME_BUTTON_P, "p", "P", "0", "=",

	ID_USERIME_BUTTON_A, "a", "A", "-", "_",
	ID_USERIME_BUTTON_S, "s", "S", "/", "——",
	ID_USERIME_BUTTON_D, "d", "D", "：", "\\",
	ID_USERIME_BUTTON_F, "f", "F", "；", "|",
	ID_USERIME_BUTTON_G, "g", "G", "（", "～",
	ID_USERIME_BUTTON_H, "h", "H", "）", "《",
	ID_USERIME_BUTTON_J, "j", "J", "￥", "》",
	ID_USERIME_BUTTON_K, "k", "K", "@", "&",
	ID_USERIME_BUTTON_L, "l", "L", ".", ".",

	ID_USERIME_BUTTON_Z, "z", "Z", "。", "。",
	ID_USERIME_BUTTON_X, "x", "X", "，", "，",
	ID_USERIME_BUTTON_C, "c", "C", "、", "、",
	ID_USERIME_BUTTON_V, "v", "V", "？", "？",
	ID_USERIME_BUTTON_B, "b", "B", "！", "！",
	ID_USERIME_BUTTON_N, "n", "N", "“", "“",
	ID_USERIME_BUTTON_M, "m", "M", "”", "”",

	ID_USERIME_BUTTON_COMMA, "，", "！", NULL, NULL,
	ID_USERIME_BUTTON_DOT, "。", "？", NULL, NULL,

	ID_USERIME_BUTTON_SPACE, " ", " ", " ", " ",
	ID_USERIME_BUTTON_ENTER, "\r", "\r", "\r", "\r",
	0, NULL, NULL, NULL, NULL
};

static const SNumKeypadInfo sNumKeypadInfoTab[] = {
	ID_USERIME_BUTTON_NUMBER_1, '1',
	ID_USERIME_BUTTON_NUMBER_2, '2',
	ID_USERIME_BUTTON_NUMBER_3, '3',
	ID_USERIME_BUTTON_NUMBER_4, '4',
	ID_USERIME_BUTTON_NUMBER_5, '5',
	ID_USERIME_BUTTON_NUMBER_6, '6',
	ID_USERIME_BUTTON_NUMBER_7, '7',
	ID_USERIME_BUTTON_NUMBER_8, '8',
	ID_USERIME_BUTTON_NUMBER_9, '9',
	ID_USERIME_BUTTON_NUMBER_0, '0',
	ID_USERIME_BUTTON_NUMBER_PLUS, '+',
	ID_USERIME_BUTTON_NUMBER_MINUS, '-',
	ID_USERIME_BUTTON_NUMBER_DOT, '.',
	ID_USERIME_BUTTON_NUMBER_PERCENT, '%',
	ID_USERIME_BUTTON_NUMBER_MULT, '*',
	ID_USERIME_BUTTON_NUMBER_DIV, '/',
	ID_USERIME_BUTTON_NUMBER_SPACE, ' ',
	0, 0
};

ZKSlideText *pSlideText;


static void addOneChar(char ch) {
	sContentStr += ch;
	mTEXTVIEW_CONTENTPtr->setText(sContentStr);
}

void addStr(int pos, const std::string &str) {
	if (str.empty() || (pos < 0) || (pos > sContentStr.length())) {
		return;
	}
	sContentStr.insert(pos, str);

#if defined(FLYTHINGS_API_1_0)
	if (pinfo.isPassword) {
		std::string passwordStr = std::string(sContentStr.length(), pinfo.passwordChar);
#else
	if (pinfo->isPassword) {
		std::string passwordStr = std::string(sContentStr.length(), pinfo->passwordChar);
#endif
		mTEXTVIEW_CONTENTPtr->setText(passwordStr);
	}
	else {
		mTEXTVIEW_CONTENTPtr->setText(sContentStr);
	}
}

static void delOneChar(uint32_t pos) {
//	if (!sContentStr.empty()) {
//		sContentStr.erase(sContentStr.length() - 1, 1);
//		mTEXTVIEW_CONTENTPtr->setText(sContentStr);
//	}

	if (sContentStr.empty() || (pos <= 0) || (pos > sContentStr.size())) {
		return;
	}

	int size = 1;
	pos--;
	if (sContentStr[pos] & 0x80) {
		while ((pos > 0) && !(sContentStr[pos] & 0x40)) {
			size++;
			pos--;
		}
	}
	sContentStr.erase(pos, size);

	mTEXTVIEW_CONTENTPtr->setText(sContentStr);
}

void clearPinyin() {
	if (! mPinyinStr.empty()) {
		mPinyinStr.clear();
		mTEXTVIEW_PINYINPtr->setText(mPinyinStr);
	}
	mSLIDETEXT_HANZIPtr->clearTextList();
}

namespace {
class MyLongClick : public ZKBase::ILongClickListener {
public:
	virtual void onLongClick(ZKBase *pBase) {
		clearPinyin();

		if ( !sContentStr.empty()) {
			sContentStr.clear();
			mTEXTVIEW_CONTENTPtr->setText("");
		}

	}
};
}
static MyLongClick sMyLongClick;


class ZKTextView;
namespace {
class MyTextUnitClickListener : public ZKSlideText::ITextUnitClickListener  {
public:
		virtual void onTextUnitClick(ZKSlideText *pSlideText, const std::string &text) {
			addStr(sContentStr.length(), text);
			clearPinyin();
		}
};
}

static MyTextUnitClickListener sMyTextUnitClickListener;

static const uint32_t kChineseLetterTextSize = 40;

static void refreshLetterButtonTextSize() {
	static const int letterButtonIds[] = {
		ID_USERIME_BUTTON_Q, ID_USERIME_BUTTON_W, ID_USERIME_BUTTON_E, ID_USERIME_BUTTON_R,
		ID_USERIME_BUTTON_T, ID_USERIME_BUTTON_Y, ID_USERIME_BUTTON_U, ID_USERIME_BUTTON_I,
		ID_USERIME_BUTTON_O, ID_USERIME_BUTTON_P,
		ID_USERIME_BUTTON_A, ID_USERIME_BUTTON_S, ID_USERIME_BUTTON_D, ID_USERIME_BUTTON_F,
		ID_USERIME_BUTTON_G, ID_USERIME_BUTTON_H, ID_USERIME_BUTTON_J, ID_USERIME_BUTTON_K,
		ID_USERIME_BUTTON_L,
		ID_USERIME_BUTTON_Z, ID_USERIME_BUTTON_X, ID_USERIME_BUTTON_C, ID_USERIME_BUTTON_V,
		ID_USERIME_BUTTON_B, ID_USERIME_BUTTON_N, ID_USERIME_BUTTON_M,
		0
	};
	static uint32_t defaultTextSize = 0;

	if ((defaultTextSize == 0) && mBUTTON_APtr) {
		defaultTextSize = mBUTTON_APtr->getTextSize();
	}

	const uint32_t baseSize = defaultTextSize ? defaultTextSize : 30;
	uint32_t targetSize = baseSize;

	if ((sKeypadType == E_KEYPAD_CHAR) && (mLangType == E_LANG_CHINESE)) {
		targetSize = kChineseLetterTextSize;
	}

	for (int i = 0; letterButtonIds[i] != 0; ++i) {
		ZKTextView *pTextView = (ZKTextView *) mActivityPtr->findControlByID(letterButtonIds[i]);
		if (pTextView) {
			pTextView->setTextSize(targetSize);
		}
	}
}


static void reshowAllKey() {
	const SKeypadInfo *tab = (mLangType == E_LANG_CHINESE) ? sChineseKeypadInfoTab : sEnglishKeypadInfoTab;
	for (int i = 0; tab[i].id != 0; ++i) {
		if ((tab[i].id == ID_USERIME_BUTTON_ENTER) ||
			(tab[i].id == ID_USERIME_BUTTON_SPACE)) {
			continue;
		}

		ZKTextView *pTextView = (ZKTextView *) mActivityPtr->findControlByID(tab[i].id);
		if (!pTextView) {
			continue;
		}

		const char *str = NULL;

		switch (sKeypadType) {
		case E_KEYPAD_CHAR: str = tab[i].ch; break;
		case E_KEYPAD_UPERCHAR: str = tab[i].upch; break;
		case E_KEYPAD_NUMCHAR: str = tab[i].numch; break;
		case E_KEYPAD_SYMBOL: str = tab[i].symb; break;
		}

		if (str) {
			pTextView->setText(str);
		}
		pTextView->setVisible(str != NULL);
	}

	mBUTTON_LSHIFTPtr->setSelected(sKeypadType == E_KEYPAD_UPERCHAR);
	mBUTTON_RSHIFTPtr->setSelected(sKeypadType == E_KEYPAD_UPERCHAR);
	mBUTTON_LANGPtr->setInvalid(false);
	mBUTTON_LANGPtr->setTouchable(true);
	mBUTTON_LANGPtr->setSelected(mLangType == E_LANG_CHINESE);
	mBUTTON_SPACEPtr->setText((mLangType == E_LANG_CHINESE) ? "空格" : "space");
	mBUTTON_ENTERPtr->setText((mLangType == E_LANG_CHINESE) ? "确认" : "return");
	refreshLetterButtonTextSize();

	switch (sKeypadType) {
	case E_KEYPAD_CHAR:
	case E_KEYPAD_UPERCHAR:
		mBUTTON_LSHIFTPtr->setText("Shift");
		mBUTTON_RSHIFTPtr->setText("Shift");
		mBUTTON_LSYMBOLPtr->setText(".?123");
		mBUTTON_RSYMBOLPtr->setText(".?123");
		break;
	case E_KEYPAD_NUMCHAR:
		mBUTTON_LSHIFTPtr->setText("#+=");
		mBUTTON_RSHIFTPtr->setText("#+=");
		mBUTTON_LSYMBOLPtr->setText("ABC");
		mBUTTON_RSYMBOLPtr->setText("ABC");
		break;
	case E_KEYPAD_SYMBOL:
		mBUTTON_LSHIFTPtr->setText("?123");
		mBUTTON_RSHIFTPtr->setText("?123");
		mBUTTON_LSYMBOLPtr->setText("ABC");
		mBUTTON_RSYMBOLPtr->setText("ABC");
		break;
	}
}

/**
 * 注册定时器
 * 填充数组用于注册定时器
 * 注意：id不能重复
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	//{0,  6000}, //定时器id=0, 时间间隔6秒
	//{1,  1000},
};

/**
 * 当界面构造时触发
 */
static void onUI_init() {
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	mLangType = E_LANG_CHINESE;
	sKeypadType = E_KEYPAD_CHAR;
	mIsIMOpenDecoderOK = false;
	//注册按键长按监听;
	mBUTTON_DELPtr->setLongClickListener(&sMyLongClick);
	mBUTTON_NUMBER_DELPtr->setLongClickListener(&sMyLongClick);
	mSLIDETEXT_HANZIPtr->setTextUnitClickListener(&sMyTextUnitClickListener);
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
	im_close_decoder();
    //取消按键长按监听
    mBUTTON_DELPtr->setLongClickListener(NULL);
    mBUTTON_NUMBER_DELPtr->setLongClickListener(NULL);
	mSLIDETEXT_HANZIPtr->setTextUnitClickListener(NULL);
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

static bool onUI_Timer(int id) {
	switch (id) {
	default:
		break;
	}

    return true;
}

#if defined(FLYTHINGS_API_1_0)
/**
 * 点击编辑框跳转到输入法界面后，会回调该接口
 * 参数：info		编辑框信息
 */
static void onUI_InitIME(const IMEContext::SIMETextInfo &info) {
	// 编辑框控件传递过来的字符串内容
	pinfo = info;

	sContentStr = info.text;
	mTEXTVIEW_CONTENTPtr->setText(sContentStr);
	if (pinfo.isPassword) {
		std::string passwordStr = std::string(sContentStr.length(), pinfo.passwordChar);
		mTEXTVIEW_CONTENTPtr->setText(passwordStr);
	}
	else {
		mTEXTVIEW_CONTENTPtr->setText(sContentStr);
	}

	sKeypadType = E_KEYPAD_CHAR;

	// 数字键盘
	if (info.imeTextType == IMEContext::E_IME_TEXT_TYPE_NUMBER) {
		mWINDOW_ALLPtr->hideWnd();
		mWINDOW_NUMBERPtr->showWnd();
	} else {	// 全键盘
		mIsIMOpenDecoderOK = openPinyinDecoder();
		LOGD("-------%d------onInitIME mIsIMOpenDecoderOK: %d----\n", __LINE__,mIsIMOpenDecoderOK);

		if (!mIsIMOpenDecoderOK || info.isPassword) {
			mLangType = E_LANG_ENGLISH;
		}

		reshowAllKey();
	}
}
#else
static void onUI_InitIME(IMEContext::SIMETextInfo *info) {
	// 编辑框控件传递过来的字符串内容
	pinfo = info;

	sContentStr = info->text;
	if (pinfo->isPassword) {
		std::string passwordStr = std::string(sContentStr.length(), pinfo->passwordChar);
		mTEXTVIEW_CONTENTPtr->setText(passwordStr);
	}
	else {
		mTEXTVIEW_CONTENTPtr->setText(sContentStr);
	}

	sKeypadType = E_KEYPAD_CHAR;

	// 数字键盘
	if (info->imeTextType == IMEContext::E_IME_TEXT_TYPE_NUMBER) {
		mWINDOW_ALLPtr->hideWnd();
		mWINDOW_NUMBERPtr->showWnd();
	} else {	// 全键盘
		mIsIMOpenDecoderOK = openPinyinDecoder();
		LOGD("-------%d------onInitIME mIsIMOpenDecoderOK: %d----\n", __LINE__,mIsIMOpenDecoderOK);

		if (!mIsIMOpenDecoderOK || info->isPassword) {
			mLangType = E_LANG_ENGLISH;
		}

		reshowAllKey();
	}
}
#endif




static void searchHanZi() {
	if (!mIsIMOpenDecoderOK || mPinyinStr.empty()) {
		return;
	}

	std::vector<std::string> hanZiList;

	im_reset_search();
	unsigned int n = im_search(mPinyinStr.c_str(), mPinyinStr.length());
	if (n > MAX_SEARCH_HANZI) {
		n = MAX_SEARCH_HANZI;
	}

	char16 buffer[64];
	UTF8 utf8Buf[1024] = { 0 };

	for (unsigned int i = 0; i < n; i++) {
		im_get_candidate(i, buffer, 64);
		toUTF8(utf8Buf, 1024, buffer, 64);
		hanZiList.push_back((const char *) utf8Buf);
	}

	mSLIDETEXT_HANZIPtr->setTextList(hanZiList);
}

static void clearPinyin_and_Hanzi() {
	if (!mPinyinStr.empty()) {
		const std::string &text = mSLIDETEXT_HANZIPtr->getText(0);
		if (!text.empty()) {
			addStr(sContentStr.length(), text);
		}

		clearPinyin();
	}
}

static void oneButtonSearchHanzi(ZKButton *pButton) {
#if defined(FLYTHINGS_API_1_0)
	IMEContext::SIMETextInfo info = pinfo;
	if (info.imeTextType == IMEContext::E_IME_TEXT_TYPE_ALL)
#else
		IMEContext::SIMETextInfo *info = pinfo;
		if (info->imeTextType == IMEContext::E_IME_TEXT_TYPE_ALL)
#endif
	{
		const SKeypadInfo *tab = (mLangType == E_LANG_CHINESE) ? sChineseKeypadInfoTab : sEnglishKeypadInfoTab;
		for (int i = 0; tab[i].id != 0; i++) {
			if (tab[i].id == pButton->getID()) {
				const char *str = NULL;

				switch (sKeypadType) {
				case E_KEYPAD_CHAR: str = tab[i].ch; break;
				case E_KEYPAD_UPERCHAR: str = tab[i].upch; break;
				case E_KEYPAD_NUMCHAR: str = tab[i].numch; break;
				case E_KEYPAD_SYMBOL: str = tab[i].symb; break;
				}

				if (!str) {
					break;
				}

				if (mLangType == E_LANG_CHINESE) {
					if (((str[0] >= 'A') && (str[0] <= 'Z')) ||
						((str[0] >= 'a') && (str[0] <= 'z'))) {
						if (mPinyinStr.length() < MAX_PINYIN_LEN) {
							mPinyinStr += str[0];
							mTEXTVIEW_PINYINPtr->setText(mPinyinStr);
							searchHanZi();
						}
						break;
					} else if (!mPinyinStr.empty()) {
						const std::string &text = mSLIDETEXT_HANZIPtr->getText(0);
						if (!text.empty()) {
							addStr(sContentStr.length(), text);
						}

						clearPinyin();

						if (tab[i].id == ID_IME_BUTTON_SPACE) {
							break;
						}
					}
				}

				addStr(sContentStr.length(), str);

				break;
			}
		}
	}
//		else if (info->imeTextType == IMEContext::E_IME_TEXT_TYPE_NUMBER) {
//		for (int i = 0; sNumKeypadInfoTab[i].id != 0; i++) {
//			if (sNumKeypadInfoTab[i].id == pButton->getID()) {
//				addStr(sContentStr.length(), std::string() + sNumKeypadInfoTab[i].ch);
//				break;
//			}
//		}
//	}
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
static bool onUserImeActivityTouchEvent(const MotionEvent &ev) {
	LOGD("--%d-- --%s-- x:%d  Y:%d\n", __LINE__, __FILE__, ev.mX, ev.mY);
	//点击空白处退出界面
	if(ev.mY < 226)
	{
		LOGD("--%d-- --%s-- ev.mY:%d 点击空白处退出界面!!\n", __LINE__, __FILE__, ev.mY);
		EASYUICONTEXT->goBack();
	}
	return false;
}

static bool onButtonClick_BUTTON_A(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_A !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_B(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_B !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_C(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_C !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_D(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_D !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_E(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_E !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_F(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_F !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_G(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_G !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_H(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_H !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_I(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_I !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_J(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_J !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_K(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_K !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_O(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_O !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_L(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_L !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_M(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_M !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_N(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_N !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_P(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_P !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_Q(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_Q !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_R(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_R !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_S(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_S !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_T(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_T !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_U(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_U !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_V(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_V !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_W(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_W !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_X(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_X !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_Y(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_Y !!!\n");
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_Z(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_Z !!!\n");
	//addOneChar(pButton->getText()[0]);
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_COMMA(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_COMMA !!!\n");
	//addOneChar(pButton->getText()[0]);
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_DOT(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_DOT !!!\n");
	//addOneChar(pButton->getText()[0]);
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_DEL(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_DEL !!!\n");
	if (mPinyinStr.empty()) {
		delOneChar(sContentStr.size());
	} else {
		if (mPinyinStr.size() == 1) {
			clearPinyin();
		} else {
			mPinyinStr.erase(mPinyinStr.length() - 1);
			mTEXTVIEW_PINYINPtr->setText(mPinyinStr);
			searchHanZi();
		}
	}
    return false;
}

static bool onButtonClick_BUTTON_ENTER(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_ENTER !!!\n");
	//逻辑代码在UserImeActivity.cpp中
    return false;
}

static bool onButtonClick_BUTTON_LSHIFT(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_LSHIFT !!!\n");
	clearPinyin_and_Hanzi();
	switch (sKeypadType) {
	case E_KEYPAD_CHAR: sKeypadType = E_KEYPAD_UPERCHAR; break;
	case E_KEYPAD_UPERCHAR: sKeypadType = E_KEYPAD_CHAR; break;
	case E_KEYPAD_NUMCHAR: sKeypadType = E_KEYPAD_SYMBOL; break;
	case E_KEYPAD_SYMBOL: sKeypadType = E_KEYPAD_NUMCHAR; break;
	}
	reshowAllKey();
    return false;
}

static bool onButtonClick_BUTTON_LSYMBOL(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_LSYMBOL !!!\n");
	clearPinyin_and_Hanzi();
	switch (sKeypadType) {
	case E_KEYPAD_CHAR:
	case E_KEYPAD_UPERCHAR:
		sKeypadType = E_KEYPAD_NUMCHAR;
		break;
	case E_KEYPAD_NUMCHAR:
	case E_KEYPAD_SYMBOL:
		sKeypadType = E_KEYPAD_CHAR;
		break;
	}
	reshowAllKey();
    return false;
}

static bool onButtonClick_BUTTON_SPACE(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_SPACE !!!\n");
	//addOneChar(pButton->getText()[0]);
	oneButtonSearchHanzi(pButton);
    return false;
}

static bool onButtonClick_BUTTON_HIDE(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_HIDE !!!\n");
	if (mLangType == E_LANG_CHINESE) {
		clearPinyin();
	}

    return false;
}

static bool onButtonClick_BUTTON_RSYMBOL(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_RSYMBOL !!!\n");
	clearPinyin_and_Hanzi();
	switch (sKeypadType) {
	case E_KEYPAD_CHAR:
	case E_KEYPAD_UPERCHAR:
		sKeypadType = E_KEYPAD_NUMCHAR;
		break;
	case E_KEYPAD_NUMCHAR:
	case E_KEYPAD_SYMBOL:
		sKeypadType = E_KEYPAD_CHAR;
		break;
	}
	reshowAllKey();
    return false;
}

static bool onButtonClick_BUTTON_RSHIFT(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_RSHIFT !!!\n");
	clearPinyin_and_Hanzi();
	switch (sKeypadType) {
	case E_KEYPAD_CHAR: sKeypadType = E_KEYPAD_UPERCHAR; break;
	case E_KEYPAD_UPERCHAR: sKeypadType = E_KEYPAD_CHAR; break;
	case E_KEYPAD_NUMCHAR: sKeypadType = E_KEYPAD_SYMBOL; break;
	case E_KEYPAD_SYMBOL: sKeypadType = E_KEYPAD_NUMCHAR; break;
	}
	reshowAllKey();
    return false;
}

static bool onButtonClick_BUTTON_LANG(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_LANG !!!\n");
	clearPinyin_and_Hanzi();
	if (mLangType == E_LANG_ENGLISH && !mIsIMOpenDecoderOK) {
		mIsIMOpenDecoderOK = openPinyinDecoder();
	}

	if (mLangType == E_LANG_ENGLISH && !mIsIMOpenDecoderOK) {
		pButton->setInvalid(false);
		pButton->setTouchable(true);
		return false;
	}

	mLangType = (mLangType == E_LANG_CHINESE) ? E_LANG_ENGLISH : E_LANG_CHINESE;
	sKeypadType = E_KEYPAD_CHAR;
	reshowAllKey();
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_DEL(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_DEL !!!\n");
	if (mPinyinStr.empty()) {
		delOneChar(sContentStr.size());
	} else {
		if (mPinyinStr.size() == 1) {
			clearPinyin();
		} else {
			mPinyinStr.erase(mPinyinStr.length() - 1);
			mTEXTVIEW_PINYINPtr->setText(mPinyinStr);
			searchHanZi();
		}
	}
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_ENTER(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_ENTER !!!\n");
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_HIDE(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_HIDE !!!\n");
	clearPinyin();
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_1(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_1 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_2(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_2 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_3(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_3 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_4(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_4 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_5(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_5 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_6(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_6 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_7(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_7 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_8(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_8 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_9(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_9 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_PLUS(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_PLUS !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_0(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_0 !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_MINUS(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_MINUS !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_DOT(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_DOT !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_PERCENT(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_PERCENT !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_MULT(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_MULT !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_DIV(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_DIV !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_BUTTON_NUMBER_SPACE(ZKButton *pButton) {
    //LOGD(" ButtonClick BUTTON_NUMBER_SPACE !!!\n");
	addOneChar(pButton->getText()[0]);
    return false;
}

static bool onButtonClick_back(ZKButton *pButton) {
    LOGD(" ButtonClick back !!!\n");
    return false;
}
