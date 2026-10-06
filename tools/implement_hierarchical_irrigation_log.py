#!/usr/bin/env python3
from __future__ import annotations

import shutil
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIAG = ROOT / 'diagnostics'

CPP_BLOCK = r'''struct SIrrigationActionLog {
    time_t timestamp;
    int address;
    bool open;
    std::string resultText;
};

struct SIrrigationGroupRunLog {
    int groupNo;
    time_t startedAt;
    time_t endedAt;
    int plannedDurationSeconds;
    std::string statusText;
    std::vector<SIrrigationActionLog> actions;
};

struct SIrrigationRunLog {
    int runId;
    time_t startedAt;
    time_t endedAt;
    std::string modeText;
    std::string planName;
    std::string statusText;
    std::vector<SIrrigationGroupRunLog> groups;
};

enum ELogDetailLevel {
    LOG_DETAIL_NONE = 0,
    LOG_DETAIL_GROUPS,
    LOG_DETAIL_ACTIONS,
};

static std::vector<SIrrigationRunLog> sIrrigationRunLogs;
static bool sIrrigationRunLogsDirty = false;
static int sIrrigationRunNextId = 1;
static int sLogDetailLevel = LOG_DETAIL_NONE;
static int sLogDetailRunIndex = -1;
static int sLogDetailGroupIndex = -1;

static void appendMainInt(std::string& text, int value) {
    char buffer[32] = {0};
    snprintf(buffer, sizeof(buffer), "%d", value);
    text += buffer;
}

static const char* irrigationLogText(const char* text) {
    return Cj96I18n::translateRuntimeText(text, Cj96I18n::getLanguage());
}

static std::string formatIrrigationLogTime(time_t value, bool seconds) {
    if (value <= 0) return "--";
    struct tm timeInfo;
    localtime_r(&value, &timeInfo);
    char buffer[32] = {0};
    snprintf(buffer, sizeof(buffer), seconds ? "%02d/%02d %02d:%02d:%02d" : "%02d/%02d %02d:%02d",
            timeInfo.tm_mon + 1, timeInfo.tm_mday, timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
    return buffer;
}

static std::string formatIrrigationClockTime(time_t value) {
    if (value <= 0) return "--";
    struct tm timeInfo;
    localtime_r(&value, &timeInfo);
    char buffer[16] = {0};
    snprintf(buffer, sizeof(buffer), "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
    return buffer;
}

static std::string formatIrrigationDuration(int seconds) {
    if (seconds < 0) seconds = 0;
    char buffer[64] = {0};
    if (seconds >= 3600) {
        snprintf(buffer, sizeof(buffer), "%dh %dm", seconds / 3600, (seconds % 3600) / 60);
    } else {
        snprintf(buffer, sizeof(buffer), "%d%s", (seconds + 59) / 60, irrigationLogText("分钟"));
    }
    return buffer;
}

static void setIrrigationLogListSubItem(ZKListView::ZKListItem *pListItem,
        int id, const char *text, int color) {
    if (!pListItem) return;
    ZKListView::ZKListSubItem *subItem = pListItem->findSubItemByID(id);
    if (subItem) {
        subItem->setText(text ? text : "");
        subItem->setTextColor(color);
    }
}

static void refreshValveOperationLogWindow() {
    if (mLogListViewPtr) mLogListViewPtr->refreshListView();
}

static void refreshIrrigationLogDetailWindow() {
    if (mLogDetailListViewPtr) mLogDetailListViewPtr->refreshListView();
}

static void hideIrrigationLogDetail() {
    sLogDetailLevel = LOG_DETAIL_NONE;
    sLogDetailRunIndex = -1;
    sLogDetailGroupIndex = -1;
    if (mLogDetailWindowPtr) mLogDetailWindowPtr->hideWnd();
}

static bool isIrrigationRunIndexValid(int index) {
    return index >= 0 && index < static_cast<int>(sIrrigationRunLogs.size());
}

static void setIrrigationLogDetailTexts(const std::string& title,
        const std::string& line1, const std::string& line2,
        const std::string& line3, const std::string& hint) {
    if (mLogDetailTitleTextPtr) mLogDetailTitleTextPtr->setText(title.c_str());
    if (mLogDetailInfo1TextPtr) mLogDetailInfo1TextPtr->setText(line1.c_str());
    if (mLogDetailInfo2TextPtr) mLogDetailInfo2TextPtr->setText(line2.c_str());
    if (mLogDetailInfo3TextPtr) mLogDetailInfo3TextPtr->setText(line3.c_str());
    if (mLogDetailHintTextPtr) mLogDetailHintTextPtr->setText(hint.c_str());
}

static void showIrrigationRunDetail(int runIndex) {
    if (!isIrrigationRunIndexValid(runIndex)) return;
    const SIrrigationRunLog& run = sIrrigationRunLogs[runIndex];
    sLogDetailLevel = LOG_DETAIL_GROUPS;
    sLogDetailRunIndex = runIndex;
    sLogDetailGroupIndex = -1;
    char countText[64] = {0};
    snprintf(countText, sizeof(countText), "%s%d%s", irrigationLogText("执行："),
             static_cast<int>(run.groups.size()), irrigationLogText("个阀组"));
    const std::string line1 = std::string(irrigationLogText("计划：")) + run.planName;
    const std::string line3 = std::string(irrigationLogText("开始：")) +
            formatIrrigationClockTime(run.startedAt) + "    " + irrigationLogText("结束：") +
            formatIrrigationClockTime(run.endedAt) + "    " + irrigationLogText(run.statusText.c_str());
    setIrrigationLogDetailTexts(irrigationLogText("灌溉执行明细"), line1, countText, line3,
            irrigationLogText("点击阀组查看地址动作"));
    if (mLogDetailWindowPtr) mLogDetailWindowPtr->showWnd();
    refreshIrrigationLogDetailWindow();
}

static void showIrrigationGroupActionDetail(int groupIndex) {
    if (!isIrrigationRunIndexValid(sLogDetailRunIndex)) return;
    SIrrigationRunLog& run = sIrrigationRunLogs[sLogDetailRunIndex];
    if (groupIndex < 0 || groupIndex >= static_cast<int>(run.groups.size())) return;
    const SIrrigationGroupRunLog& group = run.groups[groupIndex];
    sLogDetailLevel = LOG_DETAIL_ACTIONS;
    sLogDetailGroupIndex = groupIndex;
    char title[96] = {0};
    snprintf(title, sizeof(title), "%s%02d%s", irrigationLogText("阀组"), group.groupNo,
             irrigationLogText("执行明细"));
    const std::string line1 = std::string(irrigationLogText("计划：")) + run.planName;
    const std::string line2 = std::string(irrigationLogText("开始：")) +
            formatIrrigationClockTime(group.startedAt) + "    " + irrigationLogText("结束：") +
            formatIrrigationClockTime(group.endedAt);
    setIrrigationLogDetailTexts(title, line1, line2, irrigationLogText(group.statusText.c_str()),
            irrigationLogText("地址级命令流水"));
    refreshIrrigationLogDetailWindow();
}

static int getListItemCount_LogListView(const ZKListView *pListView) {
    return sIrrigationRunLogs.empty() ? 1 : static_cast<int>(sIrrigationRunLogs.size());
}

static void obtainListItemData_LogListView(ZKListView *pListView,
        ZKListView::ZKListItem *pListItem, int index) {
    const bool valid = isIrrigationRunIndexValid(index);
    const SIrrigationRunLog *run = valid ? &sIrrigationRunLogs[index] : NULL;
    const std::string timeText = run ? formatIrrigationLogTime(run->startedAt, false) :
            irrigationLogText("暂无灌溉日志");
    const std::string modeText = run ? irrigationLogText(run->modeText.c_str()) : "";
    const std::string planText = run ? run->planName : "";
    const std::string stateText = run ? irrigationLogText(run->statusText.c_str()) : "";
    std::string summary;
    if (run) {
        char buffer[48] = {0};
        snprintf(buffer, sizeof(buffer), "%d%s", static_cast<int>(run->groups.size()), irrigationLogText("组"));
        summary = buffer;
    }
    setIrrigationLogListSubItem(pListItem, ID_MAIN_LogTimeSubItem, timeText.c_str(), 0x005BBB);
    setIrrigationLogListSubItem(pListItem, ID_MAIN_LogWeekSubItem, planText.c_str(), 0x005BBB);
    setIrrigationLogListSubItem(pListItem, ID_MAIN_LogModeSubItem, modeText.c_str(), 0xFF6B00);
    setIrrigationLogListSubItem(pListItem, ID_MAIN_LogActionSubItem, stateText.c_str(), 0x00A651);
    setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem, summary.c_str(), 0x536475);
}

static void onListItemClick_LogListView(ZKListView *pListView, int index, int id) {
    if (isIrrigationRunIndexValid(index)) showIrrigationRunDetail(index);
}

static int getListItemCount_LogDetailListView(const ZKListView *pListView) {
    if (!isIrrigationRunIndexValid(sLogDetailRunIndex)) return 1;
    const SIrrigationRunLog& run = sIrrigationRunLogs[sLogDetailRunIndex];
    if (sLogDetailLevel == LOG_DETAIL_GROUPS) {
        return run.groups.empty() ? 1 : static_cast<int>(run.groups.size());
    }
    if (sLogDetailLevel == LOG_DETAIL_ACTIONS && sLogDetailGroupIndex >= 0 &&
            sLogDetailGroupIndex < static_cast<int>(run.groups.size())) {
        return run.groups[sLogDetailGroupIndex].actions.empty() ? 1 :
                static_cast<int>(run.groups[sLogDetailGroupIndex].actions.size());
    }
    return 1;
}

static void obtainListItemData_LogDetailListView(ZKListView *pListView,
        ZKListView::ZKListItem *pListItem, int index) {
    const int color = 0x005BBB;
    if (!isIrrigationRunIndexValid(sLogDetailRunIndex)) {
        setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem1, irrigationLogText("暂无记录"), color);
        return;
    }
    const SIrrigationRunLog& run = sIrrigationRunLogs[sLogDetailRunIndex];
    if (sLogDetailLevel == LOG_DETAIL_GROUPS && index >= 0 && index < static_cast<int>(run.groups.size())) {
        const SIrrigationGroupRunLog& group = run.groups[index];
        char groupText[64] = {0};
        snprintf(groupText, sizeof(groupText), "%s%02d", irrigationLogText("阀组"), group.groupNo);
        char valveCount[64] = {0};
        snprintf(valveCount, sizeof(valveCount), "%d%s", static_cast<int>(group.actions.size() / 2),
                 irrigationLogText("个阀门"));
        const int seconds = group.endedAt > group.startedAt ?
                static_cast<int>(group.endedAt - group.startedAt) : group.plannedDurationSeconds;
        const std::string duration = formatIrrigationDuration(seconds);
        setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem1, groupText, color);
        setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem2, duration.c_str(), color);
        setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem3, valveCount, 0x536475);
        setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem4,
                irrigationLogText(group.statusText.c_str()), 0x00A651);
        return;
    }
    if (sLogDetailLevel == LOG_DETAIL_ACTIONS && sLogDetailGroupIndex >= 0 &&
            sLogDetailGroupIndex < static_cast<int>(run.groups.size())) {
        const SIrrigationGroupRunLog& group = run.groups[sLogDetailGroupIndex];
        if (index >= 0 && index < static_cast<int>(group.actions.size())) {
            const SIrrigationActionLog& action = group.actions[index];
            char addressText[48] = {0};
            snprintf(addressText, sizeof(addressText), "%s%02d", irrigationLogText("地址"), action.address);
            setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem1,
                    formatIrrigationLogTime(action.timestamp, true).c_str(), color);
            setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem2, addressText, color);
            setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem3,
                    irrigationLogText(action.open ? "开阀" : "关阀"), 0xFF6B00);
            setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem4,
                    irrigationLogText(action.resultText.c_str()), 0x00A651);
            return;
        }
    }
    setIrrigationLogListSubItem(pListItem, ID_MAIN_LogDetailSubItem1, irrigationLogText("暂无记录"), color);
}

static void onListItemClick_LogDetailListView(ZKListView *pListView, int index, int id) {
    if (sLogDetailLevel == LOG_DETAIL_GROUPS) showIrrigationGroupActionDetail(index);
}

static bool onButtonClick_LogDetailBackButton(ZKButton *pButton) {
    if (sLogDetailLevel == LOG_DETAIL_ACTIONS) {
        showIrrigationRunDetail(sLogDetailRunIndex);
    } else {
        hideIrrigationLogDetail();
    }
    return true;
}

static SIrrigationRunLog* getActiveIrrigationRunLog() {
    if (sIrrigationRunLogs.empty()) return NULL;
    SIrrigationRunLog& run = sIrrigationRunLogs.back();
    return run.endedAt == 0 ? &run : NULL;
}

static void trimIrrigationRunLogs(time_t now) {
    const time_t expireBefore = now - static_cast<time_t>(30LL * 24LL * 60LL * 60LL);
    while (!sIrrigationRunLogs.empty() && sIrrigationRunLogs.front().startedAt < expireBefore) {
        sIrrigationRunLogs.erase(sIrrigationRunLogs.begin());
    }
}

static void saveValveOperationLogs() {
    std::string text("version\\t2\\n");
    for (size_t i = 0; i < sIrrigationRunLogs.size(); ++i) {
        const SIrrigationRunLog& run = sIrrigationRunLogs[i];
        text += "run\\t"; appendMainInt(text, run.runId); text += "\\t";
        appendMainInt(text, static_cast<int>(run.startedAt)); text += "\\t";
        appendMainInt(text, static_cast<int>(run.endedAt)); text += "\\t";
        text += cj96_persist::escapeField(run.modeText.c_str()); text += "\\t";
        text += cj96_persist::escapeField(run.planName.c_str()); text += "\\t";
        text += cj96_persist::escapeField(run.statusText.c_str()); text += "\\n";
        for (size_t g = 0; g < run.groups.size(); ++g) {
            const SIrrigationGroupRunLog& group = run.groups[g];
            text += "group\\t"; appendMainInt(text, run.runId); text += "\\t";
            appendMainInt(text, group.groupNo); text += "\\t";
            appendMainInt(text, static_cast<int>(group.startedAt)); text += "\\t";
            appendMainInt(text, static_cast<int>(group.endedAt)); text += "\\t";
            appendMainInt(text, group.plannedDurationSeconds); text += "\\t";
            text += cj96_persist::escapeField(group.statusText.c_str()); text += "\\n";
            for (size_t a = 0; a < group.actions.size(); ++a) {
                const SIrrigationActionLog& action = group.actions[a];
                text += "action\\t"; appendMainInt(text, run.runId); text += "\\t";
                appendMainInt(text, group.groupNo); text += "\\t";
                appendMainInt(text, static_cast<int>(action.timestamp)); text += "\\t";
                appendMainInt(text, action.address); text += "\\t";
                appendMainInt(text, action.open ? 1 : 0); text += "\\t";
                text += cj96_persist::escapeField(action.resultText.c_str()); text += "\\n";
            }
        }
    }
    if (cj96_persist::queueTextWrite(cj96_persist::WRITE_TARGET_VALVE_LOG, text)) sIrrigationRunLogsDirty = false;
}

static SIrrigationRunLog* findIrrigationRunById(int runId) {
    for (size_t i = 0; i < sIrrigationRunLogs.size(); ++i)
        if (sIrrigationRunLogs[i].runId == runId) return &sIrrigationRunLogs[i];
    return NULL;
}

static SIrrigationGroupRunLog* findIrrigationGroupLog(SIrrigationRunLog* run, int groupNo) {
    if (!run) return NULL;
    for (size_t i = 0; i < run->groups.size(); ++i)
        if (run->groups[i].groupNo == groupNo) return &run->groups[i];
    return NULL;
}

static void loadValveOperationLogs() {
    sIrrigationRunLogs.clear();
    sIrrigationRunLogsDirty = false;
    sIrrigationRunNextId = 1;
    std::string text;
    if (!cj96_persist::readTextFile(cj96_persist::logPath("valve_operations.tsv"), text)) return;
    const time_t now = time(NULL);
    const time_t expireBefore = now - static_cast<time_t>(30LL * 24LL * 60LL * 60LL);
    size_t start = 0;
    while (start <= text.size()) {
        const size_t end = text.find('\\n', start);
        std::string line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!line.empty() && line[line.size() - 1] == '\\r') line.resize(line.size() - 1);
        const std::vector<std::string> fields = cj96_persist::splitTabLine(line);
        if (fields.size() >= 7 && fields[0] == "run") {
            SIrrigationRunLog run;
            run.runId = cj96_persist::parseInt(fields[1], sIrrigationRunNextId, 1, 2147483647);
            run.startedAt = static_cast<time_t>(cj96_persist::parseInt(fields[2], 0, 0, 2147483647));
            run.endedAt = static_cast<time_t>(cj96_persist::parseInt(fields[3], 0, 0, 2147483647));
            run.modeText = cj96_persist::unescapeField(fields[4]);
            run.planName = cj96_persist::unescapeField(fields[5]);
            run.statusText = cj96_persist::unescapeField(fields[6]);
            if (run.startedAt >= expireBefore) {
                sIrrigationRunLogs.push_back(run);
                if (run.runId >= sIrrigationRunNextId) sIrrigationRunNextId = run.runId + 1;
            }
        } else if (fields.size() >= 7 && fields[0] == "group") {
            SIrrigationRunLog* run = findIrrigationRunById(cj96_persist::parseInt(fields[1], 0, 0, 2147483647));
            if (run) {
                SIrrigationGroupRunLog group;
                group.groupNo = cj96_persist::parseInt(fields[2], 0, 0, 128);
                group.startedAt = static_cast<time_t>(cj96_persist::parseInt(fields[3], 0, 0, 2147483647));
                group.endedAt = static_cast<time_t>(cj96_persist::parseInt(fields[4], 0, 0, 2147483647));
                group.plannedDurationSeconds = cj96_persist::parseInt(fields[5], 0, 0, 86400);
                group.statusText = cj96_persist::unescapeField(fields[6]);
                run->groups.push_back(group);
            }
        } else if (fields.size() >= 7 && fields[0] == "action") {
            SIrrigationRunLog* run = findIrrigationRunById(cj96_persist::parseInt(fields[1], 0, 0, 2147483647));
            SIrrigationGroupRunLog* group = run ? findIrrigationGroupLog(run,
                    cj96_persist::parseInt(fields[2], 0, 0, 128)) : NULL;
            if (group) {
                SIrrigationActionLog action;
                action.timestamp = static_cast<time_t>(cj96_persist::parseInt(fields[3], 0, 0, 2147483647));
                action.address = cj96_persist::parseInt(fields[4], 0, 0, 65535);
                action.open = cj96_persist::parseBool(fields[5], false);
                action.resultText = cj96_persist::unescapeField(fields[6]);
                group->actions.push_back(action);
            }
        } else if (fields.size() >= 7 && fields[0] == "log") {
            const time_t timestamp = static_cast<time_t>(cj96_persist::parseInt(fields[1], 0, 0, 2147483647));
            if (timestamp >= expireBefore) {
                SIrrigationRunLog run;
                run.runId = sIrrigationRunNextId++;
                run.startedAt = timestamp;
                run.endedAt = timestamp;
                run.modeText = cj96_persist::unescapeField(fields[4]);
                run.planName = cj96_persist::unescapeField(fields[6]);
                run.statusText = cj96_persist::unescapeField(fields[5]);
                sIrrigationRunLogs.push_back(run);
                sIrrigationRunLogsDirty = true;
            }
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    trimIrrigationRunLogs(now);
}

static SIrrigationRunLog& beginIrrigationRun(const char* mode, const char* planName, time_t startedAt) {
    SIrrigationRunLog run;
    run.runId = sIrrigationRunNextId++;
    run.startedAt = startedAt > 0 ? startedAt : time(NULL);
    run.endedAt = 0;
    run.modeText = mode ? mode : "自动灌溉";
    run.planName = planName ? planName : "";
    run.statusText = "执行中";
    sIrrigationRunLogs.push_back(run);
    trimIrrigationRunLogs(time(NULL));
    sIrrigationRunLogsDirty = true;
    refreshValveOperationLogWindow();
    return sIrrigationRunLogs.back();
}

static void beginScheduledIrrigationRun(const char* planName, time_t startedAt) {
    SIrrigationRunLog* active = getActiveIrrigationRunLog();
    if (!active) (void)beginIrrigationRun("自动灌溉", planName, startedAt);
}

static void appendBoundValveActions(SIrrigationGroupRunLog& group, bool open) {
    const int total = DeviceDataStore::getDeviceCount();
    const time_t now = time(NULL);
    for (int i = 0; i < total; ++i) {
        const SDATA* data = DeviceDataStore::getDevice(i);
        if (data && strcmp(data->type, "电磁阀") == 0 &&
                DeviceDataStore::isDeviceBoundToIrrGroup(data, group.groupNo)) {
            SIrrigationActionLog action;
            action.timestamp = now;
            action.address = data->address;
            action.open = open;
            action.resultText = "命令已发送";
            group.actions.push_back(action);
        }
    }
}

static void logScheduledIrrigationGroupOpen(int groupNo, int durationSeconds) {
    SIrrigationRunLog* run = getActiveIrrigationRunLog();
    if (!run) return;
    SIrrigationGroupRunLog group;
    group.groupNo = groupNo;
    group.startedAt = time(NULL);
    group.endedAt = 0;
    group.plannedDurationSeconds = durationSeconds;
    group.statusText = "执行中";
    appendBoundValveActions(group, true);
    run->groups.push_back(group);
    sIrrigationRunLogsDirty = true;
    refreshValveOperationLogWindow();
}

static void logScheduledIrrigationGroupClose(int groupNo) {
    SIrrigationRunLog* run = getActiveIrrigationRunLog();
    SIrrigationGroupRunLog* group = findIrrigationGroupLog(run, groupNo);
    if (!group) return;
    appendBoundValveActions(*group, false);
    sIrrigationRunLogsDirty = true;
}

static void finishScheduledIrrigationGroup(int groupNo, const char* status) {
    SIrrigationRunLog* run = getActiveIrrigationRunLog();
    SIrrigationGroupRunLog* group = findIrrigationGroupLog(run, groupNo);
    if (!group) return;
    group->endedAt = time(NULL);
    group->statusText = status ? status : "调度完成";
    sIrrigationRunLogsDirty = true;
    refreshIrrigationLogDetailWindow();
}

static void finishScheduledIrrigationRun(const char* status) {
    SIrrigationRunLog* run = getActiveIrrigationRunLog();
    if (!run) return;
    run->endedAt = time(NULL);
    run->statusText = status ? status : "调度完成";
    sIrrigationRunLogsDirty = true;
    refreshValveOperationLogWindow();
    refreshIrrigationLogDetailWindow();
}

static void appendValveAddressOperationLog(bool open, int address) {
    SIrrigationRunLog& run = beginIrrigationRun("手动灌溉", irrigationLogText("手动操作"), time(NULL));
    SIrrigationGroupRunLog group;
    group.groupNo = 0;
    group.startedAt = run.startedAt;
    group.endedAt = run.startedAt;
    group.plannedDurationSeconds = 0;
    group.statusText = "调度完成";
    SIrrigationActionLog action;
    action.timestamp = run.startedAt;
    action.address = address;
    action.open = open;
    action.resultText = "命令已发送";
    group.actions.push_back(action);
    run.groups.push_back(group);
    run.statusText = "调度完成";
    run.endedAt = run.startedAt;
    sIrrigationRunLogsDirty = true;
    refreshValveOperationLogWindow();
}


static void appendValveGroupOperationLog(const char* mode, int groupNo, bool open) {
    SIrrigationRunLog& run = beginIrrigationRun(
            mode && strstr(mode, "自动") ? "自动灌溉" : "手动灌溉",
            irrigationLogText("手动操作"), time(NULL));
    SIrrigationGroupRunLog group;
    group.groupNo = groupNo;
    group.startedAt = run.startedAt;
    group.endedAt = run.startedAt;
    group.plannedDurationSeconds = 0;
    group.statusText = "调度完成";
    appendBoundValveActions(group, open);
    run.groups.push_back(group);
    run.statusText = "调度完成";
    run.endedAt = run.startedAt;
    sIrrigationRunLogsDirty = true;
    refreshValveOperationLogWindow();
}

static void appendValveOperationLogEntry(const char *modeText,
        const char *actionText, const char *detailText) {
    (void)actionText;
    appendValveAddressOperationLog(false, 0);
}
'''


def require_replace(text: str, old: str, new: str, label: str, count: int = 1) -> str:
    actual = text.count(old)
    if actual != count:
        raise RuntimeError(f'{label}: expected {count} occurrence(s), found {actual}')
    return text.replace(old, new)


def backup(paths: list[Path]) -> None:
    DIAG.mkdir(exist_ok=True)
    stamp = datetime.now().strftime('%Y%m%d-%H%M%S')
    dest = DIAG / f'hierarchical_irrigation_log_backup_{stamp}'
    dest.mkdir()
    for path in paths:
        target = dest / path.relative_to(ROOT)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    print(f'Backup: {dest}')


def control(caption: str, ident: int, left: int, top: int, width: int, height: int,
            text: str = '', font: int = 22, color: int = 0x005BBB, alignment: int = 33,
            kind: str = 'textview') -> dict:
    return {
        'alignment': alignment, 'caption': caption, 'colorTab': {'color0': color},
        'family': 'Alibaba-PuHuiTi-Regular', 'fontSize': font, 'id': ident,
        **({'text': text} if text else {}), 'touchable': False,
        'position': {'height': height, 'left': left, 'top': top, 'width': width},
    }


def list_view(caption: str, ident: int, ids: list[int], captions: list[str],
              columns: list[tuple[int, int, int, int]], rows: int) -> dict:
    subs = []
    for i, sub_id in enumerate(ids):
        left, top, width, height = columns[i]
        subs.append({
            'alignment': 33, 'caption': captions[i], 'colorTab': {'color0': 0x005BBB},
            'family': 'Alibaba-PuHuiTi-Regular', 'fontSize': 21, 'id': sub_id,
            'touchable': False, 'position': {'height': height, 'left': left, 'top': top, 'width': width},
        })
    return {
        'beepEnable': True, 'caption': caption, 'cols': 1, 'id': ident, 'rows': rows,
        'position': {'height': rows * 61 + 4, 'left': 70, 'top': 100, 'width': 867},
        'colSpacing': 0, 'rowSpacing': 1,
        'item': {'alignment': 37, 'backgroundColor': 0xCFE3FA, 'caption': 'item',
                 'family': 'Alibaba-PuHuiTi-Regular', 'fontSize': 21,
                 'iconPosition': {'height': 0, 'left': 0, 'top': 0, 'width': 0},
                 'position': {'height': 60, 'left': 0, 'top': 0, 'width': 867}, 'subItem': subs},
    }


def patch_ftu() -> None:
    sys.path.insert(0, str(ROOT / 'tools'))
    from ftu_style import decode_ftu, encode_ftu
    path = ROOT / 'ui' / 'main.ftu'
    data, header, _ = decode_ftu(path)
    found = None
    def walk(obj):
        nonlocal found
        if isinstance(obj, dict):
            if obj.get('caption') == 'LogWindow': found = obj
            for value in obj.values(): walk(value)
        elif isinstance(obj, list):
            for value in obj: walk(value)
    walk(data)
    if not found: raise RuntimeError('LogWindow not found in main.ftu')
    for key in list(found):
        cap = found[key].get('caption', '') if isinstance(found[key], dict) else ''
        if cap.startswith(('LogLine', 'LogWeek', 'LogMode', 'LogAction', 'LogDetail')):
            del found[key]
    found['listview__log_main'] = list_view('LogListView', 80013,
        [24035, 24036, 24037, 24038, 24039],
        ['LogTimeSubItem', 'LogWeekSubItem', 'LogModeSubItem', 'LogActionSubItem', 'LogDetailSubItem'],
        [(12, 3, 190, 28), (12, 31, 290, 26), (210, 3, 210, 28), (640, 3, 200, 28), (310, 31, 520, 26)], 4)
    found['textview__log_hint'] = control('LogHintText', 50340, 70, 355, 867, 30,
        '点击记录查看阀组执行明细', 19, 0x536475, 37)
    window12 = None
    for value in found.values():
        if isinstance(value, dict) and value.get('caption') == 'Window12': window12 = value
    if not window12: raise RuntimeError('Window12 not found inside LogWindow')
    window12.clear()
    window12.update({'beepEnable': True, 'backgroundColor': 0xCFE3FA, 'caption': 'Window12', 'id': 110029,
                     'visible': False, 'position': {'height': 400, 'left': 0, 'top': 0, 'width': 1007}})
    window12['button__log_back'] = {'alignment': 37, 'backgroundColor': 0x168BFF, 'caption': 'LogDetailBackButton',
        'colorTab': {'color0': 0xFFFFFF}, 'family': 'Alibaba-PuHuiTi-Regular', 'fontSize': 22, 'id': 20221,
        'text': '返回', 'position': {'height': 42, 'left': 45, 'top': 35, 'width': 112}}
    window12['textview__log_detail_title'] = control('LogDetailTitleText', 50330, 220, 34, 570, 42,
        '灌溉执行明细', 28, 0x005BBB, 37)
    window12['textview__log_detail_info1'] = control('LogDetailInfo1Text', 50331, 70, 92, 860, 28, '', 20)
    window12['textview__log_detail_info2'] = control('LogDetailInfo2Text', 50332, 70, 120, 860, 28, '', 20)
    window12['textview__log_detail_info3'] = control('LogDetailInfo3Text', 50333, 70, 148, 860, 28, '', 20)
    window12['textview__log_detail_hint'] = control('LogDetailHintText', 50334, 70, 178, 860, 26,
        '点击阀组查看地址动作', 18, 0x536475, 37)
    detail = list_view('LogDetailListView', 80014, [24040, 24041, 24042, 24043],
        ['LogDetailSubItem1', 'LogDetailSubItem2', 'LogDetailSubItem3', 'LogDetailSubItem4'],
        [(12, 12, 220, 38), (240, 12, 180, 38), (430, 12, 200, 38), (640, 12, 190, 38)], 3)
    detail['position'].update({'left': 70, 'top': 212, 'height': 183, 'width': 867})
    window12['listview__log_detail'] = detail
    ids = []
    def all_ids(obj):
        if isinstance(obj, dict):
            if 'id' in obj: ids.append(obj['id'])
            for value in obj.values(): all_ids(value)
        elif isinstance(obj, list):
            for value in obj: all_ids(value)
    all_ids(data)
    duplicates = sorted(v for v in set(ids) if ids.count(v) > 1)
    if duplicates: raise RuntimeError(f'Duplicate FTU IDs: {duplicates}')
    encoded = encode_ftu(data, header)
    path.write_bytes(encoded)
    check, _, _ = decode_ftu(path)
    if not check: raise RuntimeError('FTU round-trip validation failed')


def patch_sources() -> None:
    main = ROOT / 'src' / 'logic' / 'mainLogic.cc'
    text = main.read_text(encoding='utf-8')
    start = text.index('struct SValveOperationLogItem {')
    end = text.index('static int collectMainWifiDnsServers', start)
    text = text[:start] + CPP_BLOCK + '\n\n' + text[end:]
    text = require_replace(text,
        '    appendValveGroupOperationLog("自动", groupNo, true);\n',
        '    logScheduledIrrigationGroupOpen(groupNo, static_cast<int>((sPage3ScheduleDurationsMs[groupIndex] + 999LL) / 1000LL));\n',
        'schedule group open', 1)
    text = require_replace(text,
        '    sPage3ScheduleValveStartAtMs = static_cast<long long>(valveStartTime) * 1000LL;\n',
        '    sPage3ScheduleValveStartAtMs = static_cast<long long>(valveStartTime) * 1000LL;\n'
        '    char logPlanName[96] = {0};\n'
        '    const int logLanguage = Cj96I18n::getLanguage();\n'
        '    if (programIndex >= 0 && programIndex < 4) {\n'
        '        snprintf(logPlanName, sizeof(logPlanName), "%s %s %d",\n'
        '                 Cj96I18n::translateRuntimeText(kPage3SeasonNames[programIndex], logLanguage),\n'
        '                 Cj96I18n::translateRuntimeText("计划", logLanguage), programIndex + 1);\n'
        '    } else {\n'
        '        snprintf(logPlanName, sizeof(logPlanName), "%s %d",\n'
        '                 Cj96I18n::translateRuntimeText("程序", logLanguage), programIndex + 1);\n'
        '    }\n'
        '    beginScheduledIrrigationRun(logPlanName, valveStartTime);\n',
        'schedule log run start', 1)
    text = require_replace(text,
        '        appendValveGroupOperationLog("自动",\n                sPage3ScheduleGroups[sPage3ScheduleGroupIndex], false);\n',
        '        logScheduledIrrigationGroupClose(sPage3ScheduleGroups[sPage3ScheduleGroupIndex]);\n'
        '        finishScheduledIrrigationGroup(sPage3ScheduleGroups[sPage3ScheduleGroupIndex], "已中断");\n'
        '        finishScheduledIrrigationRun("已中断");\n',
        'scheduled stop close', 1)
    text = require_replace(text,
        '    clearPage3ScheduleState();\n    return true;\n}\n\nstatic bool advancePage3ScheduledGroup()',
        '    finishScheduledIrrigationRun("已中断");\n    clearPage3ScheduleState();\n    return true;\n}\n\nstatic bool advancePage3ScheduledGroup()',
        'scheduled stop final', 1)
    text = require_replace(text,
        '            appendValveGroupOperationLog("自动",\n                    sPage3ScheduleGroups[sPage3ScheduleGroupIndex], false);\n            sPage3ScheduleGroupClosing = true;\n',
        '            logScheduledIrrigationGroupClose(sPage3ScheduleGroups[sPage3ScheduleGroupIndex]);\n'
        '            finishScheduledIrrigationGroup(sPage3ScheduleGroups[sPage3ScheduleGroupIndex], "调度完成");\n'
        '            sPage3ScheduleGroupClosing = true;\n',
        'advance group close', 1)
    text = require_replace(text,
        '    if (nextGroupIndex >= static_cast<int>(sPage3ScheduleGroups.size())) {\n        clearPage3ScheduleState();\n        return true;\n    }',
        '    if (nextGroupIndex >= static_cast<int>(sPage3ScheduleGroups.size())) {\n        finishScheduledIrrigationRun("调度完成");\n        clearPage3ScheduleState();\n        return true;\n    }',
        'advance final', 1)
    text = require_replace(text,
        '            sPage3ScheduleGroupClosing = true;\n        }\n\n        if (sPage3ScheduleSwitchAtMs > nowMs) {',
        '            logScheduledIrrigationGroupClose(sPage3ScheduleGroups[sPage3ScheduleGroupIndex]);\n'
        '            sPage3ScheduleGroupClosing = true;\n        }\n\n        if (sPage3ScheduleSwitchAtMs > nowMs) {',
        'active group close command', 1)
    text = require_replace(text,
        '        sPage3ScheduleGroupOpen = false;\n        sPage3ScheduleGroupClosing = false;\n        sPage3ScheduleCloseAtMs = 0;',
        '        if (sPage3ScheduleGroupIndex >= 0 && sPage3ScheduleGroupIndex < static_cast<int>(sPage3ScheduleGroups.size())) {\n'
        '            finishScheduledIrrigationGroup(sPage3ScheduleGroups[sPage3ScheduleGroupIndex], "调度完成");\n'
        '        }\n        sPage3ScheduleGroupOpen = false;\n        sPage3ScheduleGroupClosing = false;\n        sPage3ScheduleCloseAtMs = 0;',
        'active group complete', 1)
    text = require_replace(text,
        '        sPage3TodayCompletedTime = now;\n        clearPage3ScheduleState();',
        '        sPage3TodayCompletedTime = now;\n        finishScheduledIrrigationRun("调度完成");\n        clearPage3ScheduleState();',
        'active final complete', 1)
    main.write_text(text, encoding='utf-8')

    header = ROOT / 'src' / 'activity' / 'mainActivity.h'
    text = header.read_text(encoding='utf-8')
    anchor = '#define ID_MAIN_LogListView    80013\n'
    addition = anchor + '#define ID_MAIN_LogDetailWindow    110029\n#define ID_MAIN_LogDetailBackButton    20221\n#define ID_MAIN_LogDetailTitleText    50330\n#define ID_MAIN_LogDetailInfo1Text    50331\n#define ID_MAIN_LogDetailInfo2Text    50332\n#define ID_MAIN_LogDetailInfo3Text    50333\n#define ID_MAIN_LogDetailHintText    50334\n#define ID_MAIN_LogDetailListView    80014\n#define ID_MAIN_LogDetailSubItem1    24040\n#define ID_MAIN_LogDetailSubItem2    24041\n#define ID_MAIN_LogDetailSubItem3    24042\n#define ID_MAIN_LogDetailSubItem4    24043\n'
    text = require_replace(text, anchor, addition, 'header macro insertion', 1)
    header.write_text(text, encoding='utf-8')

    activity = ROOT / 'src' / 'activity' / 'mainActivity.cpp'
    text = activity.read_text(encoding='utf-8')
    text = require_replace(text, 'static ZKListView* mLogListViewPtr;\n',
        'static ZKListView* mLogListViewPtr;\nstatic ZKWindow* mLogDetailWindowPtr;\nstatic ZKListView* mLogDetailListViewPtr;\nstatic ZKTextView* mLogDetailTitleTextPtr;\nstatic ZKTextView* mLogDetailInfo1TextPtr;\nstatic ZKTextView* mLogDetailInfo2TextPtr;\nstatic ZKTextView* mLogDetailInfo3TextPtr;\nstatic ZKTextView* mLogDetailHintTextPtr;\n', 'activity pointers', 1)
    text = require_replace(text, '    ID_MAIN_LogButton, onButtonClick_LogButton,\n',
        '    ID_MAIN_LogButton, onButtonClick_LogButton,\n    ID_MAIN_LogDetailBackButton, onButtonClick_LogDetailBackButton,\n', 'back button callback', 1)
    text = require_replace(text, '    ID_MAIN_LogListView, getListItemCount_LogListView, obtainListItemData_LogListView, onListItemClick_LogListView,\n',
        '    ID_MAIN_LogListView, getListItemCount_LogListView, obtainListItemData_LogListView, onListItemClick_LogListView,\n    ID_MAIN_LogDetailListView, getListItemCount_LogDetailListView, obtainListItemData_LogDetailListView, onListItemClick_LogDetailListView,\n', 'detail list callback', 1)
    text = require_replace(text, '    mLogListViewPtr = NULL;\n',
        '    mLogListViewPtr = NULL;\n    mLogDetailWindowPtr = NULL;\n    mLogDetailListViewPtr = NULL;\n    mLogDetailTitleTextPtr = NULL;\n    mLogDetailInfo1TextPtr = NULL;\n    mLogDetailInfo2TextPtr = NULL;\n    mLogDetailInfo3TextPtr = NULL;\n    mLogDetailHintTextPtr = NULL;\n', 'detail pointer init', 1)
    text = require_replace(text, '    if (mLogListViewPtr != NULL) {\n        mLogListViewPtr->setListAdapter(this);\n        mLogListViewPtr->setItemClickListener(this);\n    }\n',
        '    if (mLogListViewPtr != NULL) {\n        mLogListViewPtr->setListAdapter(this);\n        mLogListViewPtr->setItemClickListener(this);\n    }\n'
        '    mLogDetailWindowPtr = (ZKWindow*)findControlByID(ID_MAIN_LogDetailWindow);\n'
        '    mLogDetailListViewPtr = (ZKListView*)findControlByID(ID_MAIN_LogDetailListView);\n'
        '    mLogDetailTitleTextPtr = (ZKTextView*)findControlByID(ID_MAIN_LogDetailTitleText);\n'
        '    mLogDetailInfo1TextPtr = (ZKTextView*)findControlByID(ID_MAIN_LogDetailInfo1Text);\n'
        '    mLogDetailInfo2TextPtr = (ZKTextView*)findControlByID(ID_MAIN_LogDetailInfo2Text);\n'
        '    mLogDetailInfo3TextPtr = (ZKTextView*)findControlByID(ID_MAIN_LogDetailInfo3Text);\n'
        '    mLogDetailHintTextPtr = (ZKTextView*)findControlByID(ID_MAIN_LogDetailHintText);\n'
        '    if (mLogDetailListViewPtr != NULL) {\n        mLogDetailListViewPtr->setListAdapter(this);\n        mLogDetailListViewPtr->setItemClickListener(this);\n    }\n', 'detail control binding', 1)
    activity.write_text(text, encoding='utf-8')

    navigation = ROOT / 'src' / 'logic' / 'pageNavigationLogic.cc'
    text = navigation.read_text(encoding='utf-8')
    text = require_replace(text, '    if (mLogWindowPtr) {\n        mLogWindowPtr->hideWnd();\n    }\n    if (mLogButtonPtr) {',
        '    if (mLogWindowPtr) {\n        mLogWindowPtr->hideWnd();\n    }\n    hideIrrigationLogDetail();\n    if (mLogButtonPtr) {', 'navigation main hide', 2)
    text = require_replace(text, '    if (mLogWindowPtr) {\n        mLogWindowPtr->showWnd();\n    }\n    sCurrentPageIndex = 0;',
        '    if (mLogWindowPtr) {\n        mLogWindowPtr->showWnd();\n    }\n    hideIrrigationLogDetail();\n    sCurrentPageIndex = 0;', 'navigation log show', 1)
    navigation.write_text(text, encoding='utf-8')

    i18n = ROOT / 'src' / 'logic' / 'Cj96I18n.h'
    text = i18n.read_text(encoding='utf-8')
    anchor = '    {"暂无灌溉日志", {"No irrigation logs", "Keine Bewässerungsprotokolle", "Aucun journal d\'irrigation", "灌水ログなし"}},\n'
    extra = anchor + '''    {"自动灌溉", {"Automatic irrigation", "Automatische Bewässerung", "Irrigation automatique", "自動灌水"}},
    {"手动灌溉", {"Manual irrigation", "Manuelle Bewässerung", "Irrigation manuelle", "手動灌水"}},
    {"手动操作", {"Manual operation", "Manuelle Bedienung", "Opération manuelle", "手動操作"}},
    {"调度完成", {"Scheduled complete", "Planmäßig abgeschlossen", "Planification terminée", "スケジュール完了"}},
    {"命令已发送", {"Command sent", "Befehl gesendet", "Commande envoyée", "コマンド送信済み"}},
    {"已中断", {"Interrupted", "Unterbrochen", "Interrompu", "中断"}},
    {"执行中", {"Running", "Läuft", "En cours", "実行中"}},
    {"灌溉执行明细", {"Irrigation details", "Bewässerungsdetails", "Détails d'irrigation", "灌水詳細"}},
    {"执行明细", {" details", " Details", " détails", " 詳細"}},
    {"点击阀组查看地址动作", {"Tap a group for address actions", "Gruppe antippen für Adressaktionen", "Touchez un groupe pour les actions", "グループをタップしてアドレス動作を見る"}},
    {"地址级命令流水", {"Address command history", "Adressbefehlsverlauf", "Historique des commandes", "アドレスコマンド履歴"}},
    {"计划：", {"Plan: ", "Plan: ", "Plan : ", "計画："}},
    {"执行：", {"Run: ", "Ausführung: ", "Exécution : ", "実行："}},
    {"开始：", {"Start: ", "Start: ", "Début : ", "開始："}},
    {"结束：", {"End: ", "Ende: ", "Fin : ", "終了："}},
    {"阀组", {"Group ", "Gruppe ", "Groupe ", "グループ"}},
    {"个阀组", {" groups", " Gruppen", " groupes", " グループ"}},
    {"个阀门", {" valves", " Ventile", " vannes", " バルブ"}},
    {"组", {" groups", " Gruppen", " groupes", " グループ"}},
    {"计划", {"Plan", "Plan", "Plan", "計画"}},
    {"程序", {"Program", "Programm", "Programme", "プログラム"}},
    {"分钟", {" min", " Min.", " min", "分"}},
    {"开阀", {"Open valve", "Ventil öffnen", "Ouvrir vanne", "開弁"}},
    {"关阀", {"Close valve", "Ventil schließen", "Fermer vanne", "閉弁"}},
    {"返回", {"Back", "Zurück", "Retour", "戻る"}},
    {"暂无记录", {"No records", "Keine Einträge", "Aucun enregistrement", "記録なし"}},
'''
    text = require_replace(text, anchor, extra, 'runtime translations', 1)
    i18n.write_text(text, encoding='utf-8')


def main() -> None:
    paths = [ROOT / p for p in [
        'ui/main.ftu', 'src/activity/mainActivity.h', 'src/activity/mainActivity.cpp',
        'src/logic/mainLogic.cc', 'src/logic/pageNavigationLogic.cc', 'src/logic/Cj96I18n.h']]
    backup(paths)
    patch_ftu()
    patch_sources()
    print('Hierarchical irrigation log implementation applied.')

if __name__ == '__main__':
    main()
