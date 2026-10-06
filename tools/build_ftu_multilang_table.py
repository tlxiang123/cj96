#!/usr/bin/env python3
"""Build a first-pass multilingual translation table from exported FTU text."""

from __future__ import annotations

import csv
from pathlib import Path

from openpyxl import Workbook
from openpyxl.styles import Font, PatternFill
from openpyxl.utils import get_column_letter


ROOT = Path(__file__).resolve().parents[1]
I18N_DIR = ROOT / "i18n"
SOURCE = I18N_DIR / "ftu_texts_zh_CN.csv"
CSV_OUTPUT = I18N_DIR / "ftu_texts_multi_language.csv"
XLSX_OUTPUT = I18N_DIR / "ftu_texts_multi_language.xlsx"


TRANSLATIONS: dict[str, tuple[str, str, str, str, str]] = {
    "水压表": ("Pressure", "Druck", "Pression", "圧力", "압력"),
    "流量表": ("Flow", "Fluss", "Débit", "流量", "유량"),
    "运行状态": ("Status", "Status", "État", "状態", "상태"),
    "待机": ("Standby", "Standby", "Veille", "待機", "대기"),
    "水压": ("Pressure", "Druck", "Pression", "水圧", "압력"),
    "无": ("-", "-", "-", "-", "없음"),
    "流量": ("Flow", "Fluss", "Débit", "流量", "유량"),
    "水泵1": ("Pump 1", "Pumpe 1", "Pompe 1", "ポンプ1", "펌프 1"),
    "水泵2": ("Pump 2", "Pumpe 2", "Pompe 2", "ポンプ2", "펌프 2"),
    "水泵3": ("Pump 3", "Pumpe 3", "Pompe 3", "ポンプ3", "펌프 3"),
    "水泵4": ("Pump 4", "Pumpe 4", "Pompe 4", "ポンプ4", "펌프 4"),
    "水泵5": ("Pump 5", "Pumpe 5", "Pompe 5", "ポンプ5", "펌프 5"),
    "开始时间": ("Start", "Start", "Début", "開始", "시작"),
    "程序1": ("Program 1", "Programm 1", "Programme 1", "プログラム1", "프로그램 1"),
    "星期模式": ("Weekly", "Wöchentlich", "Hebdo", "週間", "주간"),
    "隔天模式": ("Interval Mode", "Intervall", "Intervalle", "間隔", "간격"),
    "星期三": ("Wednesday", "Mittwoch", "Mercredi", "水曜日", "수요일"),
    "星期四": ("Thursday", "Donnerstag", "Jeudi", "木曜日", "목요일"),
    "星期五": ("Friday", "Freitag", "Vendredi", "金曜日", "금요일"),
    "星期六": ("Saturday", "Samstag", "Samedi", "土曜日", "토요일"),
    "每天": ("Every Day", "Täglich", "Quotidien", "毎日", "매일"),
    "星期日": ("Sunday", "Sonntag", "Dimanche", "日曜日", "일요일"),
    "星期一": ("Monday", "Montag", "Lundi", "月曜日", "월요일"),
    "星期二": ("Tuesday", "Dienstag", "Mardi", "火曜日", "화요일"),
    "间隔": ("Interval", "Interval", "Interval", "Interval", "Interval"),
    "延后": ("Delay", "Verzög.", "Délai", "遅延", "지연"),
    "启用": ("Enable", "Aktivieren", "Activer", "有効", "사용"),
    "阀组[]": ("Group[]", "Grp[]", "Groupe[]", "Grp[]", "그룹[]"),
    "时长 00小时 00分 00秒": ("Duration 00h 00m 00s", "Dauer 00h 00m 00s", "Durée 00h 00m 00s", "時間 00時 00分 00秒", "시간 00시 00분 00초"),
    "设置运行时间": ("Set Run Time", "Laufzeit", "Durée", "運転時間", "운전 시간"),
    "时": ("Hour", "Std.", "h", "時", "시"),
    "分": ("Min", "Min.", "min", "分", "분"),
    "秒": ("Sec", "Sek.", "s", "秒", "초"),
    "清除": ("Clear", "Löschen", "Effacer", "クリア", "지우기"),
    "提示用户": ("User Prompt", "Benutzerhinweis", "Message utilisateur", "ユーザー通知", "사용자 알림"),
    "喷雾程序": ("Spray", "Sprühen", "Brumis.", "噴霧", "분무"),
    "设备测试": ("Device Test", "Gerätetest", "Test appareil", "機器テスト", "장치 테스트"),
    "名称": ("Name", "Name", "Nom", "名前", "이름"),
    "操作": ("Action", "Aktion", "Action", "操作", "작업"),
    "编号": ("No.", "Nr.", "N°", "番号", "번호"),
    "阀组[1]": ("Group[1]", "Grp[1]", "Groupe[1]", "Grp[1]", "그룹[1]"),
    "阀组测试": ("Valve Group Test", "Ventilgruppentest", "Test groupe de vannes", "バルブグループテスト", "밸브 그룹 테스트"),
    "数据": ("Data", "Daten", "Données", "データ", "데이터"),
    "传感器数据": ("Sensor Data", "Sensordaten", "Données capteur", "センサーデータ", "센서 데이터"),
    "测试地址": ("Test Addr", "Testaddr.", "Addr. test", "TestAddr", "테스트 주소"),
    "修改地址": ("Change Addr", "Addr. ändern", "Addr.", "Addr.変更", "주소 변경"),
    "传感器": ("Sensor", "Sensor", "Capteur", "センサー", "센서"),
    "电磁阀": ("Solenoid Valve", "Magnetventil", "Électrovanne", "電磁弁", "전자 밸브"),
    "解码器类型": ("Decoder Type", "Decodertyp", "Décodeur", "Decodertyp", "디코더"),
    "灌溉时间": ("Irrigation Time", "Bewässerungszeit", "Durée d'irrigation", "灌水時間", "관수 시간"),
    "浸泡时间": ("Soak Time", "Einweichen", "Trempage", "浸透", "침투"),
    "喷雾间隔": ("Interval", "Intervall", "Intervalle", "間隔", "간격"),
    "天": ("Day", "Tag", "Day", "Day", "Day"),
    "设备地址": ("Address", "Adresse", "Adresse", "アドレス", "주소"),
    "设备名称": ("Device Name", "Gerätename", "Name", "機器名", "이름"),
    "删除": ("Delete", "Löschen", "Supprimer", "削除", "삭제"),
    "清空阀组": ("Clear", "Leeren", "Vider", "消去", "비우기"),
    "关联传感器": ("Link Sensor", "Verknüpfen", "Lier capteur", "関連付け", "연결"),
    "阀组编号": ("Group No.", "Grp.-Nr.", "N° groupe", "Grp.番号", "그룹 번호"),
    "阀组名称": ("Group Name", "Gruppenname", "Nom groupe", "グループ名", "그룹 이름"),
    "删除阀组": ("Delete", "Löschen", "Suppr.", "削除", "삭제"),
    "关联": ("Link", "Verknüpfen", "Associer", "関連付け", "연결"),
    "已关联水泵": ("Linked", "Verknüpft", "Liées", "関連済み", "연결됨"),
    "已关联传感器": ("Linked", "Verknüpft", "Liés", "関連済み", "연결됨"),
    "选择水泵": ("Select Pump", "Pumpe wählen", "Choisir pompe", "ポンプ選択", "펌프 선택"),
    "选择传感器": ("Select Sensor", "Sensor wählen", "Choisir capteur", "センサー選択", "센서 선택"),
    "雨量": ("Rainfall", "Regenmenge", "Pluie", "雨量", "강우량"),
    "保存": ("Save", "Speichern", "Enregistrer", "保存", "저장"),
    "连接类型": ("Connection Type", "Verbindungstyp", "Type de connexion", "接続タイプ", "연결 유형"),
    "自动获取IP": ("Auto IP", "IP automatisch", "IP automatique", "IP自動取得", "IP 자동 받기"),
    "静态IP": ("Static IP", "Statische IP", "IP statique", "固定IP", "고정 IP"),
    "IP地址": ("IP Address", "IP-Adresse", "Adresse IP", "IPアドレス", "IP 주소"),
    "子网掩码": ("Subnet Mask", "Subnetzmaske", "Masque sous-réseau", "サブネットマスク", "서브넷 마스크"),
    "默认网关": ("Default Gateway", "Standard-Gateway", "Passerelle par défaut", "デフォルトゲートウェイ", "기본 게이트웨이"),
    "首选DNS服务器": ("Preferred DNS", "Bevorzugter DNS", "DNS préféré", "優先DNS", "기본 DNS"),
    "备用DNS服务器": ("Alternate DNS", "Alternativer DNS", "DNS secondaire", "代替DNS", "보조 DNS"),
    "IP地址：": ("IP Address:", "IP-Adresse:", "Adresse IP :", "IPアドレス：", "IP 주소:"),
    "MAC地址：": ("MAC Address:", "MAC-Adresse:", "Adresse MAC :", "MACアドレス：", "MAC 주소:"),
    "当前状态：": ("Current Status:", "Aktueller Status:", "État actuel :", "現在の状態：", "현재 상태:"),
    "保存成功": ("Saved", "Gespeichert", "Enregistré", "保存しました", "저장됨"),
    "湿度未连接": ("Humidity offline", "Feuchte offline", "Humidité off", "湿度なし", "습도 없음"),
    "雨感未连接": ("Rain offline", "Regen offline", "Pluie hors ligne", "雨センサーなし", "우천 없음"),
    "● 灌溉中": ("● Irrigating", "● Bewässerung", "● Irrigation", "● 灌水中", "● 관수 중"),
    "● 等待": ("● Waiting", "● Warten", "● En attente", "● 待機", "● 대기"),
    "● 完成": ("● Complete", "● Fertig", "● Terminé", "● 完了", "● 완료"),
    "雨雪延后设置": ("Rain/Snow Delay", "Regen/Schnee-Verzögerung", "Délai pluie/neige", "雨雪遅延設定", "비/눈 지연 설정"),
    "取消": ("Cancel", "Abbr.", "Annul.", "取消", "취소"),
    "确定": ("OK", "OK", "OK", "OK", "확인"),
    "湿度触发设置": ("Humidity Trigger", "Feuchteauslösung", "Déclenchement humidité", "湿度トリガー設定", "습도 트리거 설정"),
    "地址": ("Address", "Adresse", "Adresse", "アドレス", "주소"),
    "类型": ("Type", "Typ", "Type", "タイプ", "유형"),
    "状态": ("Status", "Status", "État", "状態", "상태"),
    "添加设备": ("Add Device", "Gerät hinzufügen", "Ajouter appareil", "機器追加", "장치 추가"),
    "添加设备地址": ("Add Addr", "Adresse +", "Addr. +", "追加", "주소 추가"),
    "电磁阀[]未添加到阀组": ("Solenoid valve[] not in a group", "Magnetventil[] keiner Gruppe zugeordnet", "Électrovanne[] hors groupe", "電磁弁[]は未グループ", "전자 밸브[] 그룹 없음"),
    "提示": ("Prompt", "Hinweis", "Invite", "通知", "알림"),
    "关阀": ("Close Valve", "Schließen", "Fermer", "閉弁", "닫기"),
    "开阀": ("Open Valve", "Ventil öffnen", "Ouvrir vanne", "弁を開く", "밸브 열기"),
    "解码器类型：电磁阀": ("Decoder Type: Solenoid Valve", "Decodertyp: Magnetventil", "Type décodeur : électrovanne", "デコーダー種別：電磁弁", "디코더 유형: 전자 밸브"),
    "源地址": ("Source Addr", "Quelladdr.", "Addr. src.", "送信元", "소스 주소"),
    "目标地址": ("Target Addr", "Zieladdr.", "Addr. cible", "宛先", "대상 주소"),
    "强制修改": ("Force", "Erzwingen", "Forcer", "強制", "강제"),
    "选择类型": ("Select Type", "Typ wählen", "Choisir type", "タイプ選択", "유형 선택"),
    "湿度": ("Humidity", "Feuchte", "Humidité", "湿度", "습도"),
    "AC电磁阀": ("AC Valve", "AC-Ventil", "Vanne CA", "AC弁", "AC 밸브"),
    "DC电磁阀": ("DC Valve", "DC-Ventil", "Vanne CC", "DC弁", "DC 밸브"),
    "启动时间": ("Start Time", "Startzeit", "Heure de démarrage", "起動時刻", "시작 시간"),
    "关闭时间": ("Stop Time", "Stoppzeit", "Heure d'arrêt", "停止時刻", "종료 시간"),
    "天 每天": ("Day Daily", "Tag tägl.", "Jour/jour", "日 毎日", "일 매일"),
    "喷雾次数": ("Count", "Anzahl", "Nombre", "回数", "횟수"),
    "次": ("Times", "Mal", "Times", "Times", "Times"),
    "确认": ("OK", "OK", "OK", "OK", "OK"),
    "一键添加": ("Quick Add", "Schnell+", "Ajout rap.", "クイック追加", "빠른 추가"),
    "已选阀组": ("Selected", "Ausgewählt", "Sélectionné", "選択済み", "선택됨"),
    "修改名称": ("Rename", "Umbenennen", "Renommer", "名前変更", "이름 변경"),
    "设置阀组容量": ("Set Capacity", "Kapazität", "Capacité", "容量設定", "용량 설정"),
    "单个阀组": ("Single", "Einzel", "Unique", "単一", "단일"),
    "所有阀组": ("All", "Alle", "Tous", "全て", "전체"),
    "阀组": ("Group", "Grp", "Groupe", "グループ", "그룹"),
    "容量": ("Capacity", "Kapazität", "Capacity", "容量", "용량"),
    "全部": ("All", "Alle", "Tout", "すべて", "전체"),
    "关联传感器     阀组[1]": ("Link Sensor     Group[1]", "Verknüpfen     Grp.[1]", "Lier capteur     Grp.[1]", "関連付け     Grp.[1]", "센서 연결     그룹[1]"),
    "当前设备已在阀组[1]\n请选择移除或转移至阀组[1]               ": ("Current device is in Group[1]\nRemove or transfer to Group[1]", "Aktuelles Gerät ist in Gruppe[1]\nEntfernen oder zu Gruppe[1] verschieben", "L'appareil actuel est dans Groupe[1]\nSupprimer ou transférer vers Groupe[1]", "現在の機器はグループ[1]にあります\n削除またはグループ[1]へ移動", "현재 장치는 그룹[1]에 있습니다\n제거하거나 그룹[1]로 이동"),
    "移除": ("Remove", "Entfernen", "Retirer", "削除", "제거"),
    "转移至": ("Move To", "Verschieben", "Transférer", "移動先", "이동"),
    "关联水泵": ("Link Pump", "Verknüpfen", "Lier pompe", "関連付け", "펌프 연결"),
    "当前阀组[x] 设置运行时间": ("Current Group[x] Run Time", "Aktuelle Gruppe[x] Laufzeit", "Durée du groupe actuel[x]", "現在グループ[x] 運転時間", "현재 그룹[x] 운전 시간"),
    "编辑当前阀组的运行时间，\n还是编辑所有阀组？": ("Edit the current group's run time,\nor edit all groups?", "Laufzeit der aktuellen Gruppe bearbeiten\noder alle Gruppen?", "Modifier la durée du groupe actuel\nou de tous les groupes ?", "現在のグループの運転時間を編集しますか、\nそれとも全グループを編集しますか？", "현재 그룹의 운전 시간을 편집할까요,\n아니면 모든 그룹을 편집할까요?"),
    "当前": ("Current", "Aktuell", "Actuel", "現在", "현재"),
    "周\n日": ("Sun", "So.", "Dim.", "日", "일"),
    "正在检查网络": ("Checking network", "Netzwerk wird geprüft", "Vérification réseau", "ネットワーク確認中", "네트워크 확인 중"),
    "灌溉日志": ("Irrigation Log", "Bewässerungsprotokoll", "Journal d'irrigation", "灌水ログ", "관수 로그"),
    "暂无灌溉日志": ("No irrigation logs", "Keine Bewässerungsprotokolle", "Aucun journal d'irrigation", "灌水ログなし", "관수 로그 없음"),
    "总览": ("Overview", "Übersicht", "Overview", "概要", "개요"),
    "设备": ("Devices", "Geräte", "Devices", "機器", "기기"),
    "计划": ("Schedule", "Plan", "Schedule", "Schedule", "Schedule"),
    "测试": ("Test", "Test", "Test", "テスト", "테스트"),
    "日志": ("Log", "Log", "Journal", "ログ", "로그"),
    "系统设置": ("System", "System", "Système", "設定", "설정"),
    "以太网": ("Ethernet", "Ethernet", "Ethernet", "Ethernet", "Ethernet"),
    "时间": ("Time", "Zeit", "Heure", "時刻", "시간"),
    "语言": ("Language", "Sprache", "Language", "言語", "言語"),
    "调试": ("Debug", "Debug", "Debug", "Debug", "Debug"),
    "显示": ("Display", "Anzeige", "Écran", "表示", "표시"),
    "版本号": ("Version", "Version", "Version", "Version", "Version"),
    "系统版本": ("System Version", "Systemversion", "Version système", "システムバージョン", "시스템 버전"),
    "软件版本：1.0.68": ("Software Version: 1.0.68", "Softwareversion: 1.0.68", "Version logicielle : 1.0.68", "ソフトウェアバージョン：1.0.68", "소프트웨어 버전: 1.0.68"),
    "熄屏时间": ("Screen Timeout", "Timeout", "Veille", "消灯時間", "화면 시간"),
    "显示亮度": ("Brightness", "Helligkeit", "Luminosité", "画面輝度", "화면 밝기"),
    "同步时间": ("Sync Time", "Zeit synchronisieren", "Synchroniser l'heure", "時刻同期", "시간 동기화"),
    "使用24小时制": ("Use 24-hour format", "24-Stunden-Format verwenden", "Utiliser format 24 h", "24時間制を使用", "24시간 형식 사용"),
    "设置日期": ("Set Date", "Datum einstellen", "Définir date", "日付設定", "날짜 설정"),
    "2026年6月17日": ("June 17, 2026", "17. Juni 2026", "17 juin 2026", "2026年6月17日", "2026년 6월 17일"),
    "设置时间": ("Set Time", "Uhrzeit einstellen", "Définir heure", "時刻設定", "시간 설정"),
    "日": ("Day", "Tag", "Day", "日", "Day"),
    "年": ("Year", "Jahr", "Année", "年", "년"),
    "月": ("Month", "Monat", "Mois", "月", "월"),
    "设置时区": ("Time Zone", "Zeitzone", "Fuseau", "タイムゾーン", "시간대"),
    "UTC+8 北京": ("UTC+8", "UTC+8", "UTC+8", "UTC+8", "UTC+8"),
    "UTC-8 洛杉矶": ("UTC-8", "UTC-8", "UTC-8", "UTC-8", "UTC-8"),
    "UTC-5 纽约": ("UTC-5", "UTC-5", "UTC-5", "UTC-5", "UTC-5"),
    "UTC+0 伦敦": ("UTC+0", "UTC+0", "UTC+0", "UTC+0", "UTC+0"),
    "UTC+1 巴黎": ("UTC+1", "UTC+1", "UTC+1", "UTC+1", "UTC+1"),
    "UTC+3 莫斯科": ("UTC+3", "UTC+3", "UTC+3", "UTC+3", "UTC+3"),
    "UTC+5:30 印度": ("UTC+5:30", "UTC+5:30", "UTC+5:30", "UTC+5:30", "UTC+5:30"),
    "UTC+7 曼谷": ("UTC+7", "UTC+7", "UTC+7", "UTC+7", "UTC+7"),
    "UTC+9 东京": ("UTC+9", "UTC+9", "UTC+9", "UTC+9", "UTC+9"),
    "UTC+10 悉尼": ("UTC+10", "UTC+10", "UTC+10", "UTC+10", "UTC+10"),
    "时间同步失败": ("Time sync failed", "Zeitsync fehlgeschlagen", "Échec synchro heure", "時刻同期失敗", "시간 동기화 실패"),
    "网络故障，同步失败": ("Network error, sync failed", "Netzwerkfehler, Sync fehlgeschlagen", "Erreur réseau, synchro échouée", "ネットワーク障害、同期失敗", "네트워크 오류, 동기화 실패"),
    "连接": ("Connect", "Verbinden", "Connecter", "接続", "연결"),
    "名称：": ("Name:", "Name:", "Nom :", "名前：", "이름:"),
    "加密方式：": ("Security:", "Sicherheit:", "Sécurité :", "暗号方式：", "암호화:"),
    "密码：": ("Password:", "Passwort:", "Mot de passe :", "パスワード：", "비밀번호:"),
    "显示密码": ("Show Password", "Passwort anzeigen", "Afficher mot de passe", "パスワード表示", "비밀번호 표시"),
    "断开连接": ("Disconnect", "Trennen", "Déconnecter", "切断", "연결 해제"),
    "连接状态：": ("Status:", "Status:", "État :", "状態：", "상태:"),
    "已连接": ("Connected", "Verbunden", "Connecté", "接続済み", "연결됨"),
    "密码错误": ("Wrong Password", "Falsches Passwort", "Mot de passe incorrect", "パスワードエラー", "비밀번호 오류"),
    "中文": ("Chinese", "Chinesisch", "Chinois", "中国語", "중국어"),
    "英文": ("English", "Englisch", "Anglais", "英語", "영어"),
    "德文": ("German", "Deutsch", "Allemand", "ドイツ語", "독일어"),
    "法文": ("French", "Französisch", "Français", "フランス語", "프랑스어"),
    "日文": ("Japanese", "Japanisch", "Japonais", "日本語", "일본어"),
    "韩文": ("Korean", "Koreanisch", "Coréen", "韓国語", "한국어"),
}


def load_rows() -> list[dict[str, str]]:
    with SOURCE.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def normalize_source_text(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\\n", "\n").rstrip()


def save_csv(rows: list[dict[str, str]]) -> None:
    fieldnames = [
        "key",
        "中文原文",
        "English",
        "Deutsch",
        "Français",
        "日本語",
        "한국어",
        "所在FTU",
        "控件ID",
        "控件名/caption",
        "控件类型",
        "字段名",
        "JSON路径",
        "备注",
    ]
    with CSV_OUTPUT.open("w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow({field: row.get(field, "") for field in fieldnames})


def save_xlsx(rows: list[dict[str, str]]) -> None:
    fieldnames = [
        "key",
        "中文原文",
        "English",
        "Deutsch",
        "Français",
        "日本語",
        "한국어",
        "所在FTU",
        "控件ID",
        "控件名/caption",
        "控件类型",
        "字段名",
        "JSON路径",
        "备注",
    ]
    workbook = Workbook()
    sheet = workbook.active
    sheet.title = "FTU多语言"
    sheet.append(fieldnames)
    for row in rows:
        sheet.append([row.get(field, "") for field in fieldnames])

    header_fill = PatternFill(fill_type="solid", fgColor="D9EAF7")
    for cell in sheet[1]:
        cell.font = Font(bold=True)
        cell.fill = header_fill
    widths = [30, 24, 26, 28, 30, 24, 24, 16, 12, 24, 12, 12, 42, 18]
    for index, width in enumerate(widths, 1):
        sheet.column_dimensions[get_column_letter(index)].width = width
    sheet.freeze_panes = "A2"
    workbook.save(XLSX_OUTPUT)


def main() -> None:
    rows = load_rows()
    missing: set[str] = set()
    translations = {
        normalize_source_text(source_text): values
        for source_text, values in TRANSLATIONS.items()
    }
    for row in rows:
        source_text = row["中文原文"]
        values = translations.get(normalize_source_text(source_text))
        if values is None:
            values = ("", "", "", "", "")
            missing.add(source_text)
        row["English"], row["Deutsch"], row["Français"], row["日本語"], row["한국어"] = values
    save_csv(rows)
    save_xlsx(rows)
    print(f"rows={len(rows)}")
    print(f"unique={len({row['中文原文'] for row in rows})}")
    print(f"missing={len(missing)}")
    if missing:
        for text in sorted(missing):
            print(f"missing: {text}")
    print(f"csv={CSV_OUTPUT}")
    print(f"xlsx={XLSX_OUTPUT}")


if __name__ == "__main__":
    main()
