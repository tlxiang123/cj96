#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include "mqtt_client_interface.h"
#include "system_interface.h"
#include "tuya_cacert.h"
#include "tuya_error_code.h"
#include "tuya_log.h"
#include "tuyalink_core.h"
#include "cJSON.h"

#define CJ96_DEFAULT_HOST "m1.tuyacn.com"
#define CJ96_DEFAULT_PORT 8883
#define CJ96_DEFAULT_KEEPALIVE 60
#define CJ96_DEFAULT_TIMEOUT_MS 5000
#define CJ96_VALUE_MAX 512
#define CJ96_BOOT_REPORT_VALUE "AA55000355AA"
#define CJ96_SCREEN_SLEEP_VALUE "AA55F00155AA"
#define CJ96_SCREEN_WAKE_VALUE "AA55F00255AA"
#define CJ96_SCREEN_SLEEP_STATE_VALUE "AA55F01155AA"
#define CJ96_SCREEN_WAKE_STATE_VALUE "AA55F01255AA"
#define CJ96_ROUND_IRRIGATION_ON_VALUE "AA55F10155AA"
#define CJ96_ROUND_IRRIGATION_OFF_VALUE "AA55F10255AA"
#define CJ96_SCREEN_POWER_CMD_PATH "/mnt/extsd/tuya_demo/screen_power_cmd"
#define CJ96_SCREEN_POWER_STATE_PATH "/mnt/extsd/tuya_demo/screen_power_state"
#define CJ96_ROUND_IRRIGATION_CMD_PATH "/mnt/extsd/tuya_demo/round_irrigation_cmd"
#define CJ96_LOCK_PATH "/mnt/extsd/tuya_demo/cj96_tuya_demo.lock"

typedef struct {
    char product_id[64];
    char device_id[96];
    char device_secret[128];
    char host[128];
    int port;
    int keepalive;
    int timeout_ms;
    int dp_id;
    int daemonize;
    char dp_code[64];
} cj96_config_t;

static tuya_mqtt_context_t s_client;
static cj96_config_t s_config;
static volatile int s_connected = 0;
static int s_lock_fd = -1;
static uint8_t s_periodic_report_seq = 0x03;
static char s_last_reported_screen_state[16] = "";

static void trace_event(const char *fmt, ...)
{
    FILE *fp;
    va_list args;

    fp = fopen("/mnt/extsd/tuya_demo/cj96_tuya_demo.run.log", "ab");
    if (!fp) {
        return;
    }

    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fputc('\n', fp);
    fclose(fp);
}

int tuya_mqtt_subscribe_message_callback_register(tuya_mqtt_context_t *context,
                                                  const char *topic,
                                                  mqtt_subscribe_message_cb_t cb,
                                                  void *userdata);

static char *trim(char *s)
{
    char *end;
    while (*s && isspace((unsigned char)*s)) {
        ++s;
    }
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        --end;
    }
    *end = '\0';
    return s;
}

static void config_defaults(cj96_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->host, sizeof(cfg->host), "%s", CJ96_DEFAULT_HOST);
    snprintf(cfg->dp_code, sizeof(cfg->dp_code), "%s", "cj96_raw");
    cfg->port = CJ96_DEFAULT_PORT;
    cfg->keepalive = CJ96_DEFAULT_KEEPALIVE;
    cfg->timeout_ms = CJ96_DEFAULT_TIMEOUT_MS;
    cfg->dp_id = 101;
    cfg->daemonize = 0;
}

static void set_string(char *dst, size_t dst_size, const char *value)
{
    if (dst_size == 0) {
        return;
    }
    snprintf(dst, dst_size, "%s", value ? value : "");
}

static int load_config(const char *path, cj96_config_t *cfg)
{
    FILE *fp;
    char line[512];

    config_defaults(cfg);

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "open config failed: %s\n", path);
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *key;
        char *value;
        char *eq;

        key = trim(line);
        if (key[0] == '\0' || key[0] == '#') {
            continue;
        }
        eq = strchr(key, '=');
        if (!eq) {
            continue;
        }
        *eq = '\0';
        value = trim(eq + 1);
        key = trim(key);

        if (strcmp(key, "product_id") == 0) {
            set_string(cfg->product_id, sizeof(cfg->product_id), value);
        } else if (strcmp(key, "device_id") == 0 || strcmp(key, "uuid") == 0) {
            set_string(cfg->device_id, sizeof(cfg->device_id), value);
        } else if (strcmp(key, "device_secret") == 0 || strcmp(key, "auth_key") == 0) {
            set_string(cfg->device_secret, sizeof(cfg->device_secret), value);
        } else if (strcmp(key, "host") == 0) {
            set_string(cfg->host, sizeof(cfg->host), value);
        } else if (strcmp(key, "dp_code") == 0) {
            set_string(cfg->dp_code, sizeof(cfg->dp_code), value);
        } else if (strcmp(key, "port") == 0) {
            cfg->port = atoi(value);
        } else if (strcmp(key, "keepalive") == 0) {
            cfg->keepalive = atoi(value);
        } else if (strcmp(key, "timeout_ms") == 0) {
            cfg->timeout_ms = atoi(value);
        } else if (strcmp(key, "dp_id") == 0) {
            cfg->dp_id = atoi(value);
        } else if (strcmp(key, "daemon") == 0 || strcmp(key, "daemonize") == 0) {
            cfg->daemonize = atoi(value);
        }
    }

    fclose(fp);

    if (cfg->device_id[0] == '\0' || cfg->device_secret[0] == '\0') {
        fprintf(stderr, "config missing device_id/device_secret\n");
        return -1;
    }
    return 0;
}

static int daemonize_process(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        return -1;
    }
    if (pid > 0) {
        exit(0);
    }

    if (setsid() < 0) {
        return -1;
    }

    freopen("/dev/null", "rb", stdin);
    freopen("/mnt/extsd/tuya_demo/cj96_tuya_demo.stdout.log", "ab", stdout);
    freopen("/mnt/extsd/tuya_demo/cj96_tuya_demo.stderr.log", "ab", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    trace_event("daemonized pid=%ld", (long)getpid());
    return 0;
}

static void hex_dump(const uint8_t *data, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i) {
        printf("%02X", data[i]);
    }
}

static void ensure_command_directory(void)
{
    if (mkdir("/mnt/extsd/tuya_demo", 0755) != 0 && errno != EEXIST) {
        trace_event("mkdir tuya_demo failed errno=%d", errno);
    }
}

static int acquire_single_instance_lock(void)
{
    char pid_buf[32];
    int n;

    s_lock_fd = open(CJ96_LOCK_PATH, O_RDWR | O_CREAT, 0644);
    if (s_lock_fd < 0) {
        trace_event("lock open failed errno=%d", errno);
        return 0;
    }

    if (flock(s_lock_fd, LOCK_EX | LOCK_NB) != 0) {
        trace_event("another instance already running");
        close(s_lock_fd);
        s_lock_fd = -1;
        return 0;
    }

    ftruncate(s_lock_fd, 0);
    n = snprintf(pid_buf, sizeof(pid_buf), "%ld\n", (long)getpid());
    if (n > 0) {
        (void)write(s_lock_fd, pid_buf, (size_t)n);
    }
    return 1;
}

static int report_cj96_value(tuya_mqtt_context_t *context, const char *hex_value)
{
    char payload[CJ96_VALUE_MAX];
    unsigned long long timestamp_ms;
    int n;
    int ret;

    timestamp_ms = (unsigned long long)system_timestamp() * 1000ULL;
    n = snprintf(payload, sizeof(payload),
                 "{\"%s\":{\"value\":\"%s\",\"time\":%llu}}",
                 s_config.dp_code,
                 hex_value,
                 timestamp_ms);
    if (n <= 0 || n >= (int)sizeof(payload)) {
        fprintf(stderr, "report payload too large\n");
        return -1;
    }

    printf("CJ96 report property: %s\n", payload);
    trace_event("report property: %s", payload);
    ret = tuyalink_thing_property_report_with_ack(context, NULL, payload);
    trace_event("report property ret=%d", ret);
    return ret;
}

static void build_periodic_report_value(char *dst, size_t dst_size)
{
    snprintf(dst, dst_size, "AA5500%02X55AA", (unsigned int)s_periodic_report_seq);
    ++s_periodic_report_seq;
}

static int write_screen_power_command(const char *command)
{
    ensure_command_directory();
    FILE *fp = fopen(CJ96_SCREEN_POWER_CMD_PATH, "wb");
    if (!fp) {
        trace_event("screen power command open failed: %s", command);
        return -1;
    }
    fprintf(fp, "%s\n", command);
    fclose(fp);
    trace_event("screen power command=%s", command);
    return 0;
}

static int write_round_irrigation_command(const char *command)
{
    ensure_command_directory();
    FILE *fp = fopen(CJ96_ROUND_IRRIGATION_CMD_PATH, "wb");
    if (!fp) {
        trace_event("round irrigation command open failed: %s", command);
        return -1;
    }
    fprintf(fp, "%s\n", command);
    fclose(fp);
    trace_event("round irrigation command=%s", command);
    return 0;
}

static int read_screen_power_state(char *state, size_t state_size)
{
    FILE *fp;

    if (!state || state_size == 0) {
        return 0;
    }
    fp = fopen(CJ96_SCREEN_POWER_STATE_PATH, "rb");
    if (!fp) {
        return 0;
    }
    if (!fgets(state, (int)state_size, fp)) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    trim(state);
    return strcasecmp(state, "sleep") == 0 || strcasecmp(state, "wake") == 0;
}

static int report_screen_power_state(tuya_mqtt_context_t *context, bool force)
{
    char state[16];
    const char *value;
    int ret;

    if (!read_screen_power_state(state, sizeof(state))) {
        return 0;
    }
    if (!force && strcasecmp(state, s_last_reported_screen_state) == 0) {
        return 1;
    }

    value = strcasecmp(state, "sleep") == 0
            ? CJ96_SCREEN_SLEEP_STATE_VALUE
            : CJ96_SCREEN_WAKE_STATE_VALUE;
    ret = report_cj96_value(context, value);
    if (ret >= OPRT_OK) {
        snprintf(s_last_reported_screen_state,
                 sizeof(s_last_reported_screen_state), "%s", state);
        trace_event("screen power state reported=%s value=%s", state, value);
    }
    return 1;
}

static const char *find_cj96_raw_value(const char *json)
{
    static char value[CJ96_VALUE_MAX];
    char dp_id_key[16];
    cJSON *root;
    cJSON *node;
    cJSON *data_node;
    cJSON *value_node;
    const char *candidate;

    if (!json) {
        return NULL;
    }
    root = cJSON_Parse(json);
    if (!root) {
        trace_event("parse property set json failed: %s", json);
        return NULL;
    }
    candidate = NULL;
    snprintf(dp_id_key, sizeof(dp_id_key), "%d", s_config.dp_id);

    node = cJSON_GetObjectItemCaseSensitive(root, s_config.dp_code);
    if (!node) {
        node = cJSON_GetObjectItemCaseSensitive(root, dp_id_key);
    }
    data_node = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (!node && cJSON_IsObject(data_node)) {
        node = cJSON_GetObjectItemCaseSensitive(data_node, s_config.dp_code);
        if (!node) {
            node = cJSON_GetObjectItemCaseSensitive(data_node, dp_id_key);
        }
    }

    if (cJSON_IsString(node) && node->valuestring) {
        candidate = node->valuestring;
    } else if (cJSON_IsObject(node)) {
        value_node = cJSON_GetObjectItemCaseSensitive(node, "value");
        if (cJSON_IsString(value_node) && value_node->valuestring) {
            candidate = value_node->valuestring;
        }
    }

    if (!candidate || candidate[0] == '\0') {
        cJSON_Delete(root);
        trace_event("property set missing %s/%s in %s",
                    s_config.dp_code,
                    dp_id_key,
                    json);
        return NULL;
    }

    if (strlen(candidate) >= sizeof(value)) {
        cJSON_Delete(root);
        trace_event("property set value too long: %s", candidate);
        return NULL;
    }
    snprintf(value, sizeof(value), "%s", candidate);
    cJSON_Delete(root);
    return value;
}

static void handle_cj96_property_set_value(const char *hex_value)
{
    if (!hex_value) {
        trace_event("property set ignored: empty value");
        return;
    }
    if (strcasecmp(hex_value, CJ96_SCREEN_SLEEP_VALUE) == 0) {
        trace_event("property set command sleep=%s", hex_value);
        write_screen_power_command("sleep");
    } else if (strcasecmp(hex_value, CJ96_SCREEN_WAKE_VALUE) == 0) {
        trace_event("property set command wake=%s", hex_value);
        write_screen_power_command("wake");
    } else if (strcasecmp(hex_value, CJ96_ROUND_IRRIGATION_ON_VALUE) == 0) {
        trace_event("property set command round irrigation on=%s", hex_value);
        write_round_irrigation_command("on");
    } else if (strcasecmp(hex_value, CJ96_ROUND_IRRIGATION_OFF_VALUE) == 0) {
        trace_event("property set command round irrigation off=%s", hex_value);
        write_round_irrigation_command("off");
    } else {
        trace_event("property set ignored: %s", hex_value);
    }
}

static void raw_down_cb(uint16_t msgid, const mqtt_client_message_t *msg, void *userdata)
{
    tuya_mqtt_context_t *context = (tuya_mqtt_context_t *)userdata;
    char raw_hex[CJ96_VALUE_MAX];
    size_t i;
    (void)msgid;

    printf("CJ96 raw/down topic=%s len=%u payload=", msg->topic, (unsigned int)msg->length);
    hex_dump(msg->payload, msg->length);
    printf("\n");

    if (msg->length * 2 + 1 <= sizeof(raw_hex)) {
        for (i = 0; i < msg->length; ++i) {
            snprintf(raw_hex + i * 2, sizeof(raw_hex) - (size_t)(i * 2), "%02X", msg->payload[i]);
        }
        trace_event("raw/down payload=%s", raw_hex);
        handle_cj96_property_set_value(raw_hex);
    } else {
        trace_event("raw/down payload too long len=%u", (unsigned int)msg->length);
    }

    report_cj96_value(context, "AA5581010055AA");
}

static void property_set_cb(uint16_t msgid, const mqtt_client_message_t *msg, void *userdata)
{
    tuya_mqtt_context_t *context = (tuya_mqtt_context_t *)userdata;
    const char *value;

    (void)msgid;

    printf("CJ96 property/set topic=%s len=%u payload=", msg->topic, (unsigned int)msg->length);
    hex_dump(msg->payload, msg->length);
    printf("\n");
    trace_event("property/set topic=%s payload=%.*s",
                msg->topic,
                (int)msg->length,
                (const char *)msg->payload);

    value = find_cj96_raw_value((const char *)msg->payload);
    printf("CJ96 property/set parsed=%s\n", value ? value : "<null>");
    trace_event("property/set parsed=%s", value ? value : "<null>");
    handle_cj96_property_set_value(value);

    if (value) {
        report_cj96_value(context, value);
    }
}

static void on_connected(tuya_mqtt_context_t *context, void *user_data)
{
    char raw_down_topic[128];
    char property_set_topic[128];
    int ret;
    (void)user_data;

    s_connected = 1;
    trace_event("connected product_id=%s dp=%d/%s",
                s_config.product_id,
                s_config.dp_id,
                s_config.dp_code);
    printf("CJ96 Tuya connected. product_id=%s dp=%d/%s\n",
           s_config.product_id,
           s_config.dp_id,
           s_config.dp_code);

    snprintf(raw_down_topic, sizeof(raw_down_topic),
             "tylink/%s/channel/raw/down", s_config.device_id);
    ret = tuya_mqtt_subscribe_message_callback_register(context, raw_down_topic, raw_down_cb, context);
    printf("CJ96 raw/down subscribe ret=%d topic=%s\n", ret, raw_down_topic);

    snprintf(property_set_topic, sizeof(property_set_topic),
             "tylink/%s/thing/property/set", s_config.device_id);
    ret = tuya_mqtt_subscribe_message_callback_register(context, property_set_topic, property_set_cb, context);
    printf("CJ96 property/set subscribe ret=%d topic=%s\n", ret, property_set_topic);

    tuyalink_thing_data_model_get(context, NULL);
    report_cj96_value(context, CJ96_BOOT_REPORT_VALUE);
    report_screen_power_state(context, true);
}

static void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
    (void)context;
    (void)user_data;
    s_connected = 0;
    trace_event("disconnected");
    printf("CJ96 Tuya disconnected\n");
}

static void on_messages(tuya_mqtt_context_t *context, void *user_data, const tuyalink_message_t *msg)
{
    (void)user_data;
    printf("CJ96 message type=%d code=%u msgid=%s data=%s\n",
           (int)msg->type,
           (unsigned int)msg->code,
           msg->msgid ? msg->msgid : "",
           msg->data_string ? msg->data_string : "");
    trace_event("message type=%d code=%u msgid=%s data=%s",
                (int)msg->type,
                (unsigned int)msg->code,
                msg->msgid ? msg->msgid : "",
                msg->data_string ? msg->data_string : "");

    if (msg->type == THING_TYPE_PROPERTY_REPORT_RSP) {
        trace_event("property report ack code=%u msgid=%s",
                    (unsigned int)msg->code,
                    msg->msgid ? msg->msgid : "");
    }

    if (msg->type == THING_TYPE_PROPERTY_SET && msg->data_string) {
        const char *value = find_cj96_raw_value(msg->data_string);
        printf("CJ96 property set raw payload=%s parsed=%s\n",
               msg->data_string,
               value ? value : "<null>");
        trace_event("property set raw=%s parsed=%s",
                    msg->data_string,
                    value ? value : "<null>");
        handle_cj96_property_set_value(value);
        if (value) {
            printf("CJ96 property set received, echo report\n");
            trace_event("property set echo report");
            tuyalink_thing_property_report_with_ack(context, NULL, msg->data_string);
        }
    }
}

int main(int argc, char **argv)
{
    const char *config_path = "/mnt/extsd/tuya_demo/cj96_tuya_demo.conf";
    int ret;
    unsigned int now_ms;
    unsigned int last_connected_ms;
    unsigned int last_report_ms;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (argc > 1) {
        config_path = argv[1];
    }

    if (load_config(config_path, &s_config) != 0) {
        return 2;
    }

    if (s_config.daemonize && daemonize_process() != 0) {
        fprintf(stderr, "daemonize failed\n");
        trace_event("daemonize failed");
        return 5;
    }

    ensure_command_directory();
    if (!acquire_single_instance_lock()) {
        fprintf(stderr, "another cj96_tuya_demo instance is already running\n");
        return 6;
    }

    printf("CJ96 Tuya demo start. host=%s port=%d config=%s\n",
           s_config.host, s_config.port, config_path);
    trace_event("start host=%s port=%d config=%s", s_config.host, s_config.port, config_path);

    ret = tuya_mqtt_init(&s_client, &(const tuya_mqtt_config_t){
        .host = s_config.host,
        .port = (uint16_t)s_config.port,
        .cacert = (const uint8_t *)tuya_cacert_pem,
        .cacert_len = sizeof(tuya_cacert_pem),
        .device_id = s_config.device_id,
        .device_secret = s_config.device_secret,
        .keepalive = (uint32_t)s_config.keepalive,
        .timeout_ms = (uint32_t)s_config.timeout_ms,
        .on_connected = on_connected,
        .on_disconnect = on_disconnect,
        .on_messages = on_messages,
    });
    if (ret != OPRT_OK) {
        fprintf(stderr, "tuya_mqtt_init failed: %d\n", ret);
        trace_event("tuya_mqtt_init failed: %d", ret);
        return 3;
    }

    trace_event("before tuya_mqtt_connect");
    ret = tuya_mqtt_connect(&s_client);
    trace_event("tuya_mqtt_connect ret=%d", ret);
    if (ret != OPRT_OK) {
        fprintf(stderr, "tuya_mqtt_connect failed: %d\n", ret);
        return 4;
    }

    last_connected_ms = system_ticks();
    last_report_ms = last_connected_ms;

    for (;;) {
        ret = tuya_mqtt_loop(&s_client);
        if (ret != OPRT_OK) {
            trace_event("tuya_mqtt_loop failed ret=%d, restarting connection", ret);
            s_connected = 0;
            (void)tuya_mqtt_disconnect(&s_client);
            last_connected_ms = system_ticks();
            system_sleep(100);
            continue;
        }
        now_ms = system_ticks();

        if (s_connected) {
            last_connected_ms = now_ms;
            report_screen_power_state(&s_client, false);
            if ((now_ms - last_report_ms) >= 60000U) {
                if (!report_screen_power_state(&s_client, true)) {
                    char periodic_value[sizeof("AA5500FF55AA")];
                    build_periodic_report_value(periodic_value, sizeof(periodic_value));
                    report_cj96_value(&s_client, periodic_value);
                }
                last_report_ms = now_ms;
            }
        } else if ((now_ms - last_connected_ms) >= 15000U) {
            trace_event("reconnect after connection lost");
            ret = tuya_mqtt_connect(&s_client);
            trace_event("reconnect tuya_mqtt_connect ret=%d", ret);
            last_connected_ms = system_ticks();
        }

        system_sleep(100);
    }

    return 0;
}
