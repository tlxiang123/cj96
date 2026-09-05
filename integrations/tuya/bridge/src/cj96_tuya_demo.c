#include <ctype.h>
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "mqtt_client_interface.h"
#include "system_interface.h"
#include "tuya_cacert.h"
#include "tuya_error_code.h"
#include "tuya_log.h"
#include "tuyalink_core.h"
#include "cJSON.h"
#include "mbedtls/md.h"
#include "mbedtls/md5.h"

#define CJ96_DEFAULT_HOST "m1.tuyacn.com"
#define CJ96_DEFAULT_PORT 8883
#define CJ96_DEFAULT_KEEPALIVE 60
#define CJ96_DEFAULT_TIMEOUT_MS 5000
#define CJ96_MQTT_LOOP_TIMEOUT_MS 200
#define CJ96_VALUE_MAX 1536
#define CJ96_HEARTBEAT_VALUE "AA55000355AA"
#define CJ96_SCREEN_SLEEP_VALUE "AA55F00155AA"
#define CJ96_SCREEN_WAKE_VALUE "AA55F00255AA"
#define CJ96_SCREEN_SLEEP_STATE_VALUE "AA55F01155AA"
#define CJ96_SCREEN_WAKE_STATE_VALUE "AA55F01255AA"
#define CJ96_ROUND_IRRIGATION_ON_VALUE "AA55F10155AA"
#define CJ96_ROUND_IRRIGATION_OFF_VALUE "AA55F10255AA"
#define CJ96_HOME_STOP_SCHEDULE_VALUE "AA55F20155AA"
#define CJ96_HOME_ADVANCE_GROUP_VALUE "AA55F20255AA"
#define CJ96_DEVICE_SYNC_VALUE "AA55F50155AA"
#define CJ96_DEVICE_TABLE_READ_VALUE "AA55F50255AA"
#define CJ96_DEVICE_TABLE_VERSION_PREFIX "AA55F503"
#define CJ96_NETWORK_NONE_STATE_VALUE "AA55E10055AA"
#define CJ96_NETWORK_ETHERNET_STATE_VALUE "AA55E10155AA"
#define CJ96_NETWORK_WIFI_STATE_VALUE "AA55E10255AA"
#define CJ96_NETWORK_4G_STATE_VALUE "AA55E10355AA"
#define CJ96_RUNTIME_DIR "/tmp/cj96_tuya_demo"
#define CJ96_PERSIST_ROOT_PRIMARY "/mnt/extsd/cj96_tuya_demo"
#define CJ96_PERSIST_ROOT_FALLBACK "/data/cj96_tuya_demo"
#define CJ96_COMMAND_INBOX_DIR CJ96_RUNTIME_DIR "/inbox"
#define CJ96_COMMAND_ACK_DIR CJ96_RUNTIME_DIR "/ack"
#define CJ96_COMMAND_FILE_PREFIX "cmd_"
#define CJ96_COMMAND_FILE_SUFFIX ".cj96"
#define CJ96_COMMAND_PROTOCOL "CJ96CMD1"
#define CJ96_ACK_PROTOCOL "CJACK1"
#define CJ96_SCREEN_POWER_CMD_PATH CJ96_RUNTIME_DIR "/screen_power_cmd"
#define CJ96_SCREEN_POWER_STATE_PATH CJ96_RUNTIME_DIR "/screen_power_state"
#define CJ96_ROUND_IRRIGATION_CMD_PATH CJ96_RUNTIME_DIR "/round_irrigation_cmd"
#define CJ96_HOME_COMMAND_PATH CJ96_RUNTIME_DIR "/home_command"
#define CJ96_DEVICE_SYNC_COMMAND_PATH CJ96_RUNTIME_DIR "/device_sync_command"
#define CJ96_DEVICE_SYNC_ACK_PATH CJ96_RUNTIME_DIR "/device_sync_ack"
#define CJ96_DEVICE_SYNC_REPORT_QUEUE_PATH CJ96_RUNTIME_DIR "/device_sync_report.queue"
#define CJ96_DEVICE_SYNC_REPORT_SENDING_PATH CJ96_RUNTIME_DIR "/device_sync_report.sending"
#define CJ96_OTA_REQUEST_PATH CJ96_RUNTIME_DIR "/ota_request"
#define CJ96_OTA_STATUS_PATH CJ96_RUNTIME_DIR "/ota_status"
#define CJ96_OTA_CHANNEL 0U
#define CJ96_OTA_IMAGE_PATH "/mnt/extsd/update.img"
#define CJ96_OTA_PART_PATH "/mnt/extsd/update.img.part"
#define CJ96_OTA_MAX_IMAGE_SIZE (24U * 1024U * 1024U)
#define CJ96_OTA_HTTP_HEADER_MAX 8192U
#define CJ96_OTA_DOWNLOAD_BUFFER_SIZE 4096U
#define CJ96_DEVICE_SYNC_REPORT_INTERVAL_MS 80U
#define CJ96_DEVICE_SYNC_ACK_TIMEOUT_MS 1800U
#define CJ96_DEVICE_SYNC_ACK_MAX_RETRIES 5U
#define CJ96_DEVICE_SYNC_FRAME_MAX 960U
#define CJ96_LOCK_PATH CJ96_RUNTIME_DIR "/cj96_tuya_demo.lock"
#define CJ96_NETWORK_CHECK_INTERVAL_MS 1000U
#define CJ96_NETWORK_SETTLE_MS 1500U
#define CJ96_CONNECT_RETRY_MS 5000U
#define CJ96_DISCONNECTED_RESET_MS 30000U

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
    char firmware_version[32];
} cj96_config_t;

static tuya_mqtt_context_t s_client;
static cj96_config_t s_config;
static volatile int s_connected = 0;
static int s_lock_fd = -1;
static char s_last_reported_screen_state[16] = "";
static volatile int s_force_connect_pending = 0;
static volatile int s_ota_check_pending = 0;
static char s_active_network[64] = "none";
static char s_persistent_root[128] = CJ96_RUNTIME_DIR;
static unsigned long s_command_sequence = 0UL;

/* A new device-sync request supersedes frames left by an earlier auto-assign
 * or delete operation. Keep the queue state at file scope so the property
 * callback can discard that stale report before the fresh discovery result. */
static FILE *s_device_sync_queue_fp = NULL;
static unsigned int s_device_sync_last_report_ms = 0U;
static int s_device_sync_waiting_sequence = 0;
static long s_device_sync_waiting_offset = 0L;
static unsigned int s_device_sync_waiting_since_ms = 0U;
static unsigned int s_device_sync_waiting_retries = 0U;

static void trace_event(const char *fmt, ...)
{
    FILE *fp;
    va_list args;

    fp = fopen(CJ96_RUNTIME_DIR "/cj96_tuya_demo.run.log", "ab");
    if (!fp) {
        return;
    }

    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fputc('\n', fp);
    fclose(fp);
}

static bool tick_deadline_reached(unsigned int now_ms, unsigned int deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static bool get_interface_ipv4(const char *interface_name, char *ip, size_t ip_size)
{
    struct ifreq ifr;
    struct sockaddr_in *address;
    int fd;

    if (!interface_name || !ip || ip_size == 0) {
        return false;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return false;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFADDR, &ifr) != 0) {
        close(fd);
        return false;
    }

    address = (struct sockaddr_in *)&ifr.ifr_addr;
    if (!inet_ntop(AF_INET, &address->sin_addr, ip, ip_size)) {
        close(fd);
        return false;
    }
    close(fd);
    return strcmp(ip, "0.0.0.0") != 0 && strcmp(ip, "127.0.0.1") != 0;
}

static bool get_default_route_metric(const char *interface_name, unsigned long *metric)
{
    FILE *fp;
    char line[256];

    if (!interface_name) {
        return false;
    }

    fp = fopen("/proc/net/route", "r");
    if (!fp) {
        return false;
    }

    (void)fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        char iface[IFNAMSIZ];
        unsigned long destination = 0;
        unsigned long gateway = 0;
        unsigned long flags = 0;
        unsigned long route_metric = 0;
        if (sscanf(line, "%15s %lx %lx %lx %*u %*u %lu",
                   iface, &destination, &gateway, &flags, &route_metric) == 5
                && strcmp(iface, interface_name) == 0
                && destination == 0 && (flags & 0x1UL) != 0) {
            if (metric) {
                *metric = route_metric;
            }
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}

static bool interface_carrier_available(const char *interface_name)
{
    char path[96];
    FILE *fp;
    int carrier = 1;

    snprintf(path, sizeof(path), "/sys/class/net/%s/carrier", interface_name);
    fp = fopen(path, "r");
    if (!fp) {
        return true;
    }
    if (fscanf(fp, "%d", &carrier) != 1) {
        carrier = 1;
    }
    fclose(fp);
    return carrier == 1;
}

static bool interface_is_quectel_rndis(const char *interface_name)
{
    char path[128];
    char line[256];
    FILE *fp;
    bool has_rndis_driver = false;
    bool has_quectel_product = false;

    if (!interface_name) {
        return false;
    }

    snprintf(path, sizeof(path), "/sys/class/net/%s/device/uevent", interface_name);
    fp = fopen(path, "r");
    if (!fp) {
        return false;
    }
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "DRIVER=rndis_host", strlen("DRIVER=rndis_host")) == 0) {
            has_rndis_driver = true;
        } else if (strncmp(line, "PRODUCT=2c7c/", 13) == 0) {
            has_quectel_product = true;
        }
    }
    fclose(fp);
    return has_rndis_driver && has_quectel_product;
}

static bool describe_network_interface(const char *interface_name,
                                       char *description,
                                       size_t description_size)
{
    char ip[INET_ADDRSTRLEN];
    unsigned long metric = 0;

    if (!interface_carrier_available(interface_name)
            || !get_interface_ipv4(interface_name, ip, sizeof(ip))
            || !get_default_route_metric(interface_name, &metric)) {
        return false;
    }

    snprintf(description, description_size, "%s:%s:%lu",
             interface_name, ip, metric);
    return true;
}

static void detect_active_network(char *description, size_t description_size)
{
	FILE *fp;
	char line[256];

	if (describe_network_interface("eth0", description, description_size)) {
		return;
	}
	if (describe_network_interface("wlan0", description, description_size)) {
		return;
	}
	if (interface_is_quectel_rndis("usb0")
			&& describe_network_interface("usb0", description, description_size)) {
		return;
	}

	/* 4G modules commonly expose a routed data link as usb0, ppp0, or wwan0.
	 * Enumerate the actual interface names so the MQTT client can use 4G
	 * without requiring an Ethernet link to be present. */
	fp = fopen("/proc/net/dev", "r");
	if (fp) {
		while (fgets(line, sizeof(line), fp)) {
			char *name = line;
			char interface_name[IFNAMSIZ];
			size_t length;

			while (*name == ' ' || *name == '\t') {
				++name;
			}
			char *colon = strchr(name, ':');
			if (!colon) {
				continue;
			}
			*colon = '\0';
			length = strlen(name);
			while (length > 0U && (name[length - 1U] == ' ' || name[length - 1U] == '\t')) {
				name[--length] = '\0';
			}
			if ((strncmp(name, "ppp", 3) != 0 && strncmp(name, "wwan", 4) != 0) ||
				length == 0U || length >= sizeof(interface_name)) {
				continue;
			}
			strncpy(interface_name, name, sizeof(interface_name) - 1U);
			interface_name[sizeof(interface_name) - 1U] = '\0';
			if (describe_network_interface(interface_name, description, description_size)) {
				fclose(fp);
				return;
			}
		}
		fclose(fp);
	}

	snprintf(description, description_size, "none");
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
    /* This must match the version configured for the full update.img in Tuya. */
    snprintf(cfg->firmware_version, sizeof(cfg->firmware_version), "%s", "1.0.54");
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
        } else if (strcmp(key, "firmware_version") == 0) {
            set_string(cfg->firmware_version, sizeof(cfg->firmware_version), value);
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
    freopen(CJ96_RUNTIME_DIR "/cj96_tuya_demo.stdout.log", "ab", stdout);
    freopen(CJ96_RUNTIME_DIR "/cj96_tuya_demo.stderr.log", "ab", stderr);
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

static int write_all_fd(int fd, const char *data, size_t length)
{
    size_t written = 0U;
    while (written < length) {
        const ssize_t ret = write(fd, data + written, length - written);
        if (ret <= 0) {
            return -1;
        }
        written += (size_t)ret;
    }
    return 0;
}

static int ensure_directory(const char *path)
{
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

static int atomic_write_text(const char *path, const char *text)
{
    char temporary_path[256];
    int fd;
    size_t length;

    if (!path || !text) {
        return -1;
    }
    snprintf(temporary_path, sizeof(temporary_path), "%s.new", path);
    fd = open(temporary_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    length = strlen(text);
    if (write_all_fd(fd, text, length) != 0 || fsync(fd) != 0) {
        close(fd);
        unlink(temporary_path);
        return -1;
    }
    close(fd);
    if (rename(temporary_path, path) != 0) {
        unlink(temporary_path);
        return -1;
    }
    return 0;
}

static int write_all_binary(int fd, const unsigned char *data, size_t length)
{
    size_t written = 0U;

    while (written < length) {
        ssize_t result = write(fd, data + written, length - written);
        if (result <= 0) {
            return -1;
        }
        written += (size_t)result;
    }
    return 0;
}

static void report_ota_progress(tuya_mqtt_context_t *context, unsigned int progress)
{
    char data[64];
    char status[160];
    snprintf(data, sizeof(data), "{\"channel\":%u,\"progress\":%u}",
             CJ96_OTA_CHANNEL, progress);
    (void)tuyalink_ota_progress_report(context, s_config.device_id, data);
    snprintf(status, sizeof(status), "state=downloading\nprogress=%u\nmessage=\n", progress);
    (void)atomic_write_text(CJ96_OTA_STATUS_PATH, status);
}

static void report_ota_error(tuya_mqtt_context_t *context, int error_code, const char *message)
{
    char data[256];
    char status[512];
    snprintf(data, sizeof(data),
             "{\"channel\":%u,\"errorCode\":%d,\"errorMsg\":\"%s\"}",
             CJ96_OTA_CHANNEL, error_code, message ? message : "upgrade failed");
    (void)tuyalink_ota_progress_report(context, s_config.device_id, data);
    snprintf(status, sizeof(status), "state=error\nprogress=0\nmessage=%s\n",
             message ? message : "升级失败，请检查网络或云端版本");
    (void)atomic_write_text(CJ96_OTA_STATUS_PATH, status);
}

static void report_ota_version(tuya_mqtt_context_t *context, const char *biz_type)
{
    char data[256];
    snprintf(data, sizeof(data),
             "{\"bizType\":\"%s\",\"pid\":\"%s\",\"otaChannel\":[{\"channel\":%u,\"version\":\"%s\"}]}",
             biz_type, s_config.product_id, CJ96_OTA_CHANNEL, s_config.firmware_version);
    (void)tuyalink_ota_firmware_report(context, s_config.device_id, data);
}

static bool consume_ota_request(void)
{
    int fd = open(CJ96_OTA_REQUEST_PATH, O_RDONLY);
    char request[16];
    ssize_t length;

    if (fd < 0) {
        return false;
    }
    length = read(fd, request, sizeof(request) - 1U);
    close(fd);
    unlink(CJ96_OTA_REQUEST_PATH);
    if (length <= 0) {
        return false;
    }
    request[length] = '\0';
    return strncmp(request, "check", 5) == 0;
}

static bool hex_equals(const unsigned char *data, size_t length, const char *hex)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;
    unsigned char different = 0U;

    if (!data || !hex || strlen(hex) != length * 2U) {
        return false;
    }
    for (i = 0U; i < length; ++i) {
        char high = (char)tolower((unsigned char)hex[i * 2U]);
        char low = (char)tolower((unsigned char)hex[i * 2U + 1U]);
        different |= (unsigned char)(digits[data[i] >> 4U] ^ high);
        different |= (unsigned char)(digits[data[i] & 0x0fU] ^ low);
    }
    return different == 0U;
}

static int parse_http_url(const char *url, char *host, size_t host_size,
                          unsigned short *port, char *path, size_t path_size)
{
    const char *authority;
    const char *path_start;
    const char *port_start = NULL;
    size_t authority_length;
    size_t host_length;
    char port_text[8];

    if (!url || !host || !port || !path || strncmp(url, "http://", 7) != 0) {
        return -1;
    }
    authority = url + 7;
    path_start = strchr(authority, '/');
    authority_length = path_start ? (size_t)(path_start - authority) : strlen(authority);
    if (authority_length == 0U || authority_length >= host_size) {
        return -1;
    }
    for (size_t i = 0U; i < authority_length; ++i) {
        if (authority[i] == ':') {
            port_start = authority + i + 1U;
            authority_length = i;
            break;
        }
    }
    host_length = authority_length;
    if (host_length == 0U || host_length >= host_size) {
        return -1;
    }
    memcpy(host, authority, host_length);
    host[host_length] = '\0';
    *port = 80U;
    if (port_start) {
        size_t port_length = path_start ? (size_t)(path_start - port_start) : strlen(port_start);
        if (port_length == 0U || port_length >= sizeof(port_text)) {
            return -1;
        }
        memcpy(port_text, port_start, port_length);
        port_text[port_length] = '\0';
        *port = (unsigned short)strtoul(port_text, NULL, 10);
        if (*port == 0U) {
            return -1;
        }
    }
    snprintf(path, path_size, "%s", path_start ? path_start : "/");
    return 0;
}

static int parse_http_response_headers(char *headers, size_t header_length,
                                       unsigned long expected_size)
{
    char *line;
    char *next;
    unsigned long content_length = 0UL;
    bool have_content_length = false;

    if (header_length < 12U || strncmp(headers, "HTTP/", 5) != 0 ||
            strstr(headers, " 200 ") == NULL) {
        return -1;
    }
    line = strstr(headers, "\r\n");
    while (line && *line) {
        line += 2;
        if (line[0] == '\r' && line[1] == '\n') {
            break;
        }
        next = strstr(line, "\r\n");
        if (!next) {
            return -1;
        }
        *next = '\0';
        if (strncasecmp(line, "Content-Length:", 15) == 0) {
            char *value = trim(line + 15);
            char *end = NULL;
            content_length = strtoul(value, &end, 10);
            if (end == value || *trim(end) != '\0') {
                return -1;
            }
            have_content_length = true;
        }
        line = next;
    }
    return have_content_length && content_length == expected_size ? 0 : -1;
}

static int download_ota_image(tuya_mqtt_context_t *context, const char *url,
                              unsigned long expected_size, const char *expected_md5,
                              const char *expected_hmac)
{
    char host[160];
    char path[1024];
    char request[1400];
    unsigned short port;
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    struct addrinfo *candidate;
    int socket_fd = -1;
    int image_fd = -1;
    int result = -1;
    unsigned char receive_buffer[CJ96_OTA_DOWNLOAD_BUFFER_SIZE];
    char header[CJ96_OTA_HTTP_HEADER_MAX + 1U];
    size_t header_length = 0U;
    size_t initial_body_offset = 0U;
    size_t initial_body_length = 0U;
    unsigned long downloaded = 0UL;
    unsigned int last_progress = 0U;
    unsigned char md5[16];
    unsigned char hmac[32];
    mbedtls_md5_context md5_context;
    mbedtls_md_context_t hmac_context;
    const mbedtls_md_info_t *hmac_info;
    struct statvfs filesystem;

    if (expected_size == 0UL || expected_size > CJ96_OTA_MAX_IMAGE_SIZE ||
            !expected_md5 || !expected_hmac ||
            parse_http_url(url, host, sizeof(host), &port, path, sizeof(path)) != 0) {
        return -1;
    }
    if (statvfs("/mnt/extsd", &filesystem) != 0 ||
            (unsigned long long)filesystem.f_bavail * filesystem.f_frsize <
                (unsigned long long)expected_size + 1024U * 1024U) {
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(host, NULL, &hints, &addresses) != 0) {
        return -1;
    }
    for (candidate = addresses; candidate != NULL; candidate = candidate->ai_next) {
        struct sockaddr_storage address;
        socklen_t address_length = candidate->ai_addrlen;
        memcpy(&address, candidate->ai_addr, address_length);
        if (candidate->ai_family == AF_INET) {
            ((struct sockaddr_in *)&address)->sin_port = htons(port);
        } else if (candidate->ai_family == AF_INET6) {
            ((struct sockaddr_in6 *)&address)->sin6_port = htons(port);
        } else {
            continue;
        }
        socket_fd = socket(candidate->ai_family, SOCK_STREAM, 0);
        if (socket_fd >= 0 && connect(socket_fd, (struct sockaddr *)&address, address_length) == 0) {
            break;
        }
        if (socket_fd >= 0) {
            close(socket_fd);
            socket_fd = -1;
        }
    }
    freeaddrinfo(addresses);
    if (socket_fd < 0) {
        return -1;
    }
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
             path, host);
    if (write_all_fd(socket_fd, request, strlen(request)) != 0) {
        goto done;
    }
    unlink(CJ96_OTA_PART_PATH);
    image_fd = open(CJ96_OTA_PART_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (image_fd < 0) {
        goto done;
    }
    mbedtls_md5_init(&md5_context);
    mbedtls_md5_starts_ret(&md5_context);
    mbedtls_md_init(&hmac_context);
    hmac_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!hmac_info || mbedtls_md_setup(&hmac_context, hmac_info, 1) != 0 ||
            mbedtls_md_hmac_starts(&hmac_context, (const unsigned char *)s_config.device_secret,
                                   strlen(s_config.device_secret)) != 0) {
        goto done_hash;
    }
    report_ota_progress(context, 0U);
    for (;;) {
        ssize_t received = recv(socket_fd, receive_buffer, sizeof(receive_buffer), 0);
        if (received == 0) {
            break;
        }
        if (received < 0) {
            goto done_hash;
        }
        if (header_length < sizeof(header) - 1U) {
            size_t copy = (size_t)received;
            if (copy > sizeof(header) - 1U - header_length) {
                copy = sizeof(header) - 1U - header_length;
            }
            memcpy(header + header_length, receive_buffer, copy);
            header_length += copy;
            header[header_length] = '\0';
            char *header_end = strstr(header, "\r\n\r\n");
            if (!header_end) {
                if (copy != (size_t)received) {
                    goto done_hash;
                }
                continue;
            }
            initial_body_offset = (size_t)(header_end + 4 - header);
            initial_body_length = header_length - initial_body_offset;
            *header_end = '\0';
            if (parse_http_response_headers(header, initial_body_offset, expected_size) != 0) {
                goto done_hash;
            }
            if (initial_body_length > 0U) {
                if (write_all_binary(image_fd, (unsigned char *)header + initial_body_offset,
                                     initial_body_length) != 0) {
                    goto done_hash;
                }
                mbedtls_md5_update_ret(&md5_context, (unsigned char *)header + initial_body_offset,
                                       initial_body_length);
                mbedtls_md_hmac_update(&hmac_context, (unsigned char *)header + initial_body_offset,
                                       initial_body_length);
                downloaded += (unsigned long)initial_body_length;
            }
            header_length = sizeof(header);
        } else {
            if (write_all_binary(image_fd, receive_buffer, (size_t)received) != 0) {
                goto done_hash;
            }
            mbedtls_md5_update_ret(&md5_context, receive_buffer, (size_t)received);
            mbedtls_md_hmac_update(&hmac_context, receive_buffer, (size_t)received);
            downloaded += (unsigned long)received;
        }
        if (downloaded > expected_size) {
            goto done_hash;
        }
        unsigned int progress = (unsigned int)((downloaded * 95UL) / expected_size);
        if (progress >= last_progress + 5U) {
            last_progress = progress;
            report_ota_progress(context, progress);
        }
    }
    if (downloaded != expected_size || fsync(image_fd) != 0 ||
            mbedtls_md5_finish_ret(&md5_context, md5) != 0 ||
            mbedtls_md_hmac_finish(&hmac_context, hmac) != 0 ||
            !hex_equals(md5, sizeof(md5), expected_md5) ||
            !hex_equals(hmac, sizeof(hmac), expected_hmac)) {
        goto done_hash;
    }
    if (lseek(image_fd, 0, SEEK_SET) < 0 || read(image_fd, receive_buffer, 9U) != 9 ||
            memcmp(receive_buffer, "ZKSWEV1.0", 9U) != 0) {
        goto done_hash;
    }
    close(image_fd);
    image_fd = -1;
    if (rename(CJ96_OTA_PART_PATH, CJ96_OTA_IMAGE_PATH) != 0) {
        goto done_hash;
    }
    sync();
    result = 0;

done_hash:
    mbedtls_md_free(&hmac_context);
    mbedtls_md5_free(&md5_context);
done:
    if (image_fd >= 0) {
        close(image_fd);
    }
    if (socket_fd >= 0) {
        close(socket_fd);
    }
    if (result != 0) {
        unlink(CJ96_OTA_PART_PATH);
    }
    return result;
}

static int set_board_property(const char *name, const char *value)
{
    pid_t child = fork();
    int status;
    if (child < 0) {
        return -1;
    }
    if (child == 0) {
        execl("/bin/setprop", "setprop", name, value, (char *)NULL);
        _exit(127);
    }
    return waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

static int handoff_ota_to_board_upgrader(void)
{
    /* The vendor GUI loads libzkupgrade.so. These properties tell it to consume
     * the verified /mnt/extsd/update.img on its restart. */
    if (set_board_property("sys.zkupgrade.dir", "/mnt/extsd") != 0 ||
            set_board_property("sys.zkupgrade.flag", "zkautoupgrade") != 0 ||
            set_board_property("sys.zkupgrade.force", "1") != 0 ||
            set_board_property("ctl.restart", "zkswe") != 0) {
        return -1;
    }
    return 0;
}

static void handle_ota_message(tuya_mqtt_context_t *context, const char *data)
{
    cJSON *root = NULL;
    cJSON *channel;
    cJSON *url;
    cJSON *size;
    cJSON *md5;
    cJSON *hmac;
    cJSON *version;

    if (!data) {
        report_ota_error(context, 40, "missing upgrade data");
        return;
    }
    root = cJSON_Parse(data);
    channel = root ? cJSON_GetObjectItemCaseSensitive(root, "channel") : NULL;
    url = root ? cJSON_GetObjectItemCaseSensitive(root, "url") : NULL;
    size = root ? cJSON_GetObjectItemCaseSensitive(root, "size") : NULL;
    md5 = root ? cJSON_GetObjectItemCaseSensitive(root, "md5") : NULL;
    hmac = root ? cJSON_GetObjectItemCaseSensitive(root, "hmac") : NULL;
    version = root ? cJSON_GetObjectItemCaseSensitive(root, "version") : NULL;
    if (!cJSON_IsNumber(channel) || channel->valueint != (int)CJ96_OTA_CHANNEL ||
            !cJSON_IsString(url) ||
            !cJSON_IsNumber(size) || !cJSON_IsString(md5) || !cJSON_IsString(hmac) ||
            !cJSON_IsString(version) || !url->valuestring || !md5->valuestring ||
            !hmac->valuestring || !version->valuestring ||
            strcmp(version->valuestring, s_config.firmware_version) == 0 ||
            size->valuedouble <= 0.0 || size->valuedouble > (double)CJ96_OTA_MAX_IMAGE_SIZE) {
        cJSON_Delete(root);
        report_ota_error(context, 40, "invalid upgrade package");
        return;
    }
    trace_event("OTA download requested version=%s size=%d", version->valuestring, size->valueint);
    if (download_ota_image(context, url->valuestring, (unsigned long)size->valueint,
                           md5->valuestring, hmac->valuestring) != 0) {
        cJSON_Delete(root);
        report_ota_error(context, 42, "package download or verification failed");
        return;
    }
    report_ota_progress(context, 98U);
    if (handoff_ota_to_board_upgrader() != 0) {
        cJSON_Delete(root);
        report_ota_error(context, 44, "board upgrade service start failed");
        return;
    }
    (void)atomic_write_text(CJ96_OTA_STATUS_PATH,
            "state=ready\nprogress=100\nmessage=\n");
    trace_event("OTA image verified and handed to board upgrader version=%s", version->valuestring);
    cJSON_Delete(root);
}

static void ensure_command_directory(void)
{
    if (ensure_directory(CJ96_RUNTIME_DIR) != 0) {
        trace_event("mkdir tuya_demo failed errno=%d", errno);
    }
    (void)ensure_directory(CJ96_COMMAND_INBOX_DIR);
    (void)ensure_directory(CJ96_COMMAND_ACK_DIR);
}

static void ensure_persistent_storage(void)
{
    const char *roots[] = {CJ96_PERSIST_ROOT_PRIMARY, CJ96_PERSIST_ROOT_FALLBACK};
    const char *parents[] = {"/mnt/extsd", "/data"};
    char path[192];
    size_t i;

    snprintf(s_persistent_root, sizeof(s_persistent_root), "%s", CJ96_RUNTIME_DIR);
    for (i = 0U; i < sizeof(roots) / sizeof(roots[0]); ++i) {
        if (access(parents[i], W_OK) != 0 || ensure_directory(roots[i]) != 0) {
            continue;
        }
        snprintf(path, sizeof(path), "%s/inbox", roots[i]);
        if (ensure_directory(path) != 0) {
            continue;
        }
        snprintf(path, sizeof(path), "%s/ack", roots[i]);
        if (ensure_directory(path) != 0) {
            continue;
        }
        snprintf(s_persistent_root, sizeof(s_persistent_root), "%s", roots[i]);
        break;
    }
    ensure_command_directory();
    trace_event("persistent root=%s", s_persistent_root);
}

static unsigned long next_command_sequence(void)
{
    char path[192];
    char text[64];
    unsigned long value = 0UL;
    FILE *fp;

    snprintf(path, sizeof(path), "%s/command_seq", s_persistent_root);
    fp = fopen(path, "rb");
    if (fp != NULL) {
        (void)fscanf(fp, "%lu", &value);
        fclose(fp);
    }
    ++value;
    snprintf(text, sizeof(text), "%lu\n", value);
    if (atomic_write_text(path, text) != 0) {
        trace_event("command sequence persist failed path=%s errno=%d", path, errno);
    }
    s_command_sequence = value;
    return value;
}

static int enqueue_tuya_command(const char *kind, const char *command)
{
    char inbox_dir[192];
    char path[256];
    char content[512];
    char command_id[80];
    const unsigned long sequence = next_command_sequence();
    const unsigned long timestamp = (unsigned long)system_timestamp();

    if (!kind || !command) {
        return -1;
    }
    snprintf(inbox_dir, sizeof(inbox_dir), "%s/inbox", s_persistent_root);
    (void)ensure_directory(inbox_dir);
    snprintf(command_id, sizeof(command_id), "%08lX-%08lX", timestamp, sequence);
    snprintf(path, sizeof(path), "%s/%s%010lu_%s%s", inbox_dir,
             CJ96_COMMAND_FILE_PREFIX, sequence, command_id, CJ96_COMMAND_FILE_SUFFIX);
    snprintf(content, sizeof(content), "%s|%s|%lu|%s|%s\n",
             CJ96_COMMAND_PROTOCOL, command_id, sequence, kind, command);
    if (atomic_write_text(path, content) != 0) {
        trace_event("command enqueue failed kind=%s seq=%lu errno=%d", kind, sequence, errno);
        return -1;
    }
    trace_event("command enqueued id=%s seq=%lu kind=%s value=%s path=%s",
                command_id, sequence, kind, command, path);
    return 0;
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

static void reset_device_sync_report_queue(void)
{
    if (s_device_sync_queue_fp != NULL) {
        fclose(s_device_sync_queue_fp);
        s_device_sync_queue_fp = NULL;
    }
    s_device_sync_last_report_ms = 0U;
    s_device_sync_waiting_sequence = 0;
    s_device_sync_waiting_offset = 0L;
    s_device_sync_waiting_since_ms = 0U;
    s_device_sync_waiting_retries = 0U;
    (void)unlink(CJ96_DEVICE_SYNC_REPORT_SENDING_PATH);
    (void)unlink(CJ96_DEVICE_SYNC_REPORT_QUEUE_PATH);
    (void)unlink(CJ96_DEVICE_SYNC_ACK_PATH);
    trace_event("device sync report queue reset for fresh sync");
}

static int parse_device_sync_ack_token(const char *frame)
{
    char prefix[8];
    char kind;
    char session[8];
    unsigned int sequence;
    if (sscanf(frame, "%7[^|]|%c|%7[^|]|%u", prefix, &kind, session, &sequence) >= 3 &&
            strcmp(prefix, "CJDS1") == 0) {
        if (kind == 'S') return 10001;
        if (kind == 'C') return (int)sequence;
        if (kind == 'E') return 10002;
    }
    return 0;
}

static int read_device_sync_ack(void)
{
    FILE *fp = fopen(CJ96_DEVICE_SYNC_ACK_PATH, "rb");
    int sequence = 0;
    if (fp == NULL) return 0;
    (void)fscanf(fp, "%d", &sequence);
    fclose(fp);
    unlink(CJ96_DEVICE_SYNC_ACK_PATH);
    return sequence;
}

static void report_next_device_sync_frame(tuya_mqtt_context_t *context,
                                          unsigned int now_ms)
{
    char frame[CJ96_DEVICE_SYNC_FRAME_MAX + 4U];
    long frame_offset;
    size_t length;
    int ret;

    if (!s_connected) {
        return;
    }

    if (s_device_sync_waiting_sequence > 0) {
        const int ack_sequence = read_device_sync_ack();
        if (ack_sequence == s_device_sync_waiting_sequence) {
            trace_event("device sync report ack sequence=%d", ack_sequence);
            s_device_sync_waiting_sequence = 0;
            s_device_sync_waiting_retries = 0U;
        } else if ((now_ms - s_device_sync_waiting_since_ms) >= CJ96_DEVICE_SYNC_ACK_TIMEOUT_MS) {
            if (s_device_sync_waiting_retries >= CJ96_DEVICE_SYNC_ACK_MAX_RETRIES) {
                trace_event("device sync report abort missing ack sequence=%d", s_device_sync_waiting_sequence);
                if (s_device_sync_queue_fp != NULL) fclose(s_device_sync_queue_fp);
                s_device_sync_queue_fp = NULL;
                unlink(CJ96_DEVICE_SYNC_REPORT_SENDING_PATH);
                s_device_sync_waiting_sequence = 0;
                s_device_sync_waiting_retries = 0U;
                return;
            }
            clearerr(s_device_sync_queue_fp);
            (void)fseek(s_device_sync_queue_fp, s_device_sync_waiting_offset, SEEK_SET);
            ++s_device_sync_waiting_retries;
            s_device_sync_waiting_sequence = 0;
            trace_event("device sync report resend retry=%u", s_device_sync_waiting_retries);
        } else {
            return;
        }
    }
    if (s_device_sync_last_report_ms != 0U &&
            (now_ms - s_device_sync_last_report_ms) < CJ96_DEVICE_SYNC_REPORT_INTERVAL_MS) {
        return;
    }

    if (s_device_sync_queue_fp == NULL) {
        if (access(CJ96_DEVICE_SYNC_REPORT_SENDING_PATH, F_OK) != 0) {
            if (rename(CJ96_DEVICE_SYNC_REPORT_QUEUE_PATH,
                       CJ96_DEVICE_SYNC_REPORT_SENDING_PATH) != 0) {
                return;
            }
        }
        s_device_sync_queue_fp = fopen(CJ96_DEVICE_SYNC_REPORT_SENDING_PATH, "rb");
        if (s_device_sync_queue_fp == NULL) {
            return;
        }
        trace_event("device sync report queue opened");
    }

    frame_offset = ftell(s_device_sync_queue_fp);
    if (fgets(frame, sizeof(frame), s_device_sync_queue_fp) == NULL) {
        fclose(s_device_sync_queue_fp);
        s_device_sync_queue_fp = NULL;
        unlink(CJ96_DEVICE_SYNC_REPORT_SENDING_PATH);
        trace_event("device sync report queue complete");
        return;
    }
    length = strlen(frame);
    while (length > 0U && (frame[length - 1U] == '\r' || frame[length - 1U] == '\n')) {
        frame[--length] = '\0';
    }
    if (length == 0U) {
        return;
    }
    if (length > CJ96_DEVICE_SYNC_FRAME_MAX) {
        trace_event("device sync report frame rejected len=%u", (unsigned int)length);
        return;
    }

    ret = report_cj96_value(context, frame);
    if (ret < OPRT_OK) {
        clearerr(s_device_sync_queue_fp);
        (void)fseek(s_device_sync_queue_fp, frame_offset, SEEK_SET);
        trace_event("device sync report retry ret=%d", ret);
        return;
    }
    s_device_sync_last_report_ms = now_ms;
    s_device_sync_waiting_sequence = parse_device_sync_ack_token(frame);
    if (s_device_sync_waiting_sequence > 0) {
        s_device_sync_waiting_offset = frame_offset;
        s_device_sync_waiting_since_ms = now_ms;
    } else {
        s_device_sync_waiting_retries = 0U;
    }
    trace_event("device sync report sent len=%u frame=%s",
                (unsigned int)length, frame);
}

static const char *current_network_state_value(void)
{
    if (strncmp(s_active_network, "eth0:", 5) == 0) {
        return CJ96_NETWORK_ETHERNET_STATE_VALUE;
    }
    if (strncmp(s_active_network, "wlan0:", 6) == 0) {
        return CJ96_NETWORK_WIFI_STATE_VALUE;
    }
    if (strncmp(s_active_network, "ppp", 3) == 0
            || strncmp(s_active_network, "wwan", 4) == 0
            || strncmp(s_active_network, "usb0:", 5) == 0) {
        return CJ96_NETWORK_4G_STATE_VALUE;
    }
    return CJ96_NETWORK_NONE_STATE_VALUE;
}

static int report_network_state(tuya_mqtt_context_t *context)
{
    const char *value = current_network_state_value();
    int ret = report_cj96_value(context, value);
    if (ret >= OPRT_OK) {
        trace_event("network state reported=%s network=%s", value, s_active_network);
    }
    return ret;
}

static int write_screen_power_command(const char *command)
{
    const int ret = enqueue_tuya_command("screen_power", command);
    trace_event("screen power command=%s enqueue_ret=%d", command, ret);
    return ret;
}

static int write_round_irrigation_command(const char *command)
{
    const int ret = enqueue_tuya_command("round_irrigation", command);
    trace_event("round irrigation command=%s enqueue_ret=%d", command, ret);
    return ret;
}

static int write_home_command(const char *command)
{
    const int ret = enqueue_tuya_command("home", command);
    trace_event("home command=%s enqueue_ret=%d", command, ret);
    return ret;
}

static int parse_byte_command(const char *hex_value,
                              const char *prefix,
                              int minimum,
                              int maximum,
                              int *value)
{
    char byte_text[3];
    long parsed;

    if (!hex_value || !prefix || !value || strlen(hex_value) != 12
            || strlen(prefix) != 6
            || strncasecmp(hex_value, prefix, 6) != 0
            || strcasecmp(hex_value + 8, "55AA") != 0
            || !isxdigit((unsigned char)hex_value[6])
            || !isxdigit((unsigned char)hex_value[7])) {
        return 0;
    }

    byte_text[0] = hex_value[6];
    byte_text[1] = hex_value[7];
    byte_text[2] = '\0';
    parsed = strtol(byte_text, NULL, 16);
    if (parsed < minimum || parsed > maximum) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static int parse_device_table_version_command(const char *hex_value,
                                                unsigned int *hash)
{
    char hash_text[9];
    unsigned long parsed;
    size_t index;

    if (!hex_value || !hash || strlen(hex_value) != 20
            || strncasecmp(hex_value, CJ96_DEVICE_TABLE_VERSION_PREFIX, 8) != 0
            || strcasecmp(hex_value + 16, "55AA") != 0) {
        return 0;
    }
    for (index = 8; index < 16; ++index) {
        if (!isxdigit((unsigned char)hex_value[index])) {
            return 0;
        }
    }
    memcpy(hash_text, hex_value + 8, 8);
    hash_text[8] = '\0';
    parsed = strtoul(hash_text, NULL, 16);
    if (parsed > 0xFFFFFFFFUL) {
        return 0;
    }
    *hash = (unsigned int)parsed;
    return 1;
}

static int parse_two_byte_command(const char *hex_value,
                                   const char *prefix,
                                   int first_minimum,
                                   int first_maximum,
                                   int second_minimum,
                                   int second_maximum,
                                   int *first_value,
                                   int *second_value)
{
    char first_text[3];
    char second_text[3];
    long first_parsed;
    long second_parsed;

    if (!hex_value || !prefix || !first_value || !second_value
            || strlen(hex_value) != 14 || strlen(prefix) != 6
            || strncasecmp(hex_value, prefix, 6) != 0
            || strcasecmp(hex_value + 10, "55AA") != 0
            || !isxdigit((unsigned char)hex_value[6])
            || !isxdigit((unsigned char)hex_value[7])
            || !isxdigit((unsigned char)hex_value[8])
            || !isxdigit((unsigned char)hex_value[9])) {
        return 0;
    }

    first_text[0] = hex_value[6];
    first_text[1] = hex_value[7];
    first_text[2] = '\0';
    second_text[0] = hex_value[8];
    second_text[1] = hex_value[9];
    second_text[2] = '\0';
    first_parsed = strtol(first_text, NULL, 16);
    second_parsed = strtol(second_text, NULL, 16);
    if (first_parsed < first_minimum || first_parsed > first_maximum
            || second_parsed < second_minimum || second_parsed > second_maximum) {
        return 0;
    }
    *first_value = (int)first_parsed;
    *second_value = (int)second_parsed;
    return 1;
}

static int parse_group_rename_command(const char *hex_value,
                                         int *group_no,
                                         char *name,
                                         size_t name_capacity)
{
    size_t length;
    size_t name_length;
    size_t index;
    char group_text[3];
    char length_text[3];

    if (!hex_value || !group_no || !name || name_capacity < 2) {
        return 0;
    }
    length = strlen(hex_value);
    if (length < 16 || strncasecmp(hex_value, "AA55FD", 6) != 0 ||
            strcasecmp(hex_value + length - 4, "55AA") != 0) {
        return 0;
    }
    group_text[0] = hex_value[6];
    group_text[1] = hex_value[7];
    group_text[2] = '\0';
    length_text[0] = hex_value[8];
    length_text[1] = hex_value[9];
    length_text[2] = '\0';
    if (!isxdigit((unsigned char)group_text[0]) ||
            !isxdigit((unsigned char)group_text[1]) ||
            !isxdigit((unsigned char)length_text[0]) ||
            !isxdigit((unsigned char)length_text[1])) {
        return 0;
    }
    *group_no = (int)strtol(group_text, NULL, 16);
    name_length = (size_t)strtol(length_text, NULL, 16);
    if (*group_no < 1 || *group_no > 128 || name_length == 0 ||
            name_length >= name_capacity || length != 14 + name_length * 2) {
        return 0;
    }
    for (index = 0; index < name_length; ++index) {
        char byte_text[3] = { hex_value[10 + index * 2],
                              hex_value[11 + index * 2], '\0' };
        if (!isxdigit((unsigned char)byte_text[0]) ||
                !isxdigit((unsigned char)byte_text[1])) {
            return 0;
        }
        name[index] = (char)strtol(byte_text, NULL, 16);
    }
    name[name_length] = '\0';
    return 1;
}

static int parse_group_bind_command(const char *hex_value,
                                      int *group_code,
                                      int *addresses,
                                      size_t address_capacity,
                                      size_t *address_count)
{
    size_t length;
    size_t count;
    size_t index;

    if (!hex_value || !group_code || !addresses || !address_count) {
        return 0;
    }
    length = strlen(hex_value);
    if (length < 14 || strncasecmp(hex_value, "AA55FA", 6) != 0 ||
            strcasecmp(hex_value + length - 4, "55AA") != 0 ||
            !isxdigit((unsigned char)hex_value[6]) ||
            !isxdigit((unsigned char)hex_value[7]) ||
            !isxdigit((unsigned char)hex_value[8]) ||
            !isxdigit((unsigned char)hex_value[9])) {
        return 0;
    }

    {
        char group_text[3] = { hex_value[6], hex_value[7], '\0' };
        char count_text[3] = { hex_value[8], hex_value[9], '\0' };
        *group_code = (int)strtol(group_text, NULL, 16);
        count = (size_t)strtol(count_text, NULL, 16);
    }
    if (!((*group_code >= 1 && *group_code <= 128) || *group_code == 0xFF) ||
            count == 0 || count > address_capacity ||
            length != 14 + count * 2) {
        return 0;
    }

    for (index = 0; index < count; ++index) {
        char byte_text[3];
        const size_t offset = 10 + index * 2;
        if (!isxdigit((unsigned char)hex_value[offset]) ||
                !isxdigit((unsigned char)hex_value[offset + 1])) {
            return 0;
        }
        byte_text[0] = hex_value[offset];
        byte_text[1] = hex_value[offset + 1];
        byte_text[2] = '\0';
        addresses[index] = (int)strtol(byte_text, NULL, 16);
        if (addresses[index] <= 0) {
            return 0;
        }
    }
    *address_count = count;
    return 1;
}

static int parse_device_upsert_command(const char *hex_value,
                                       int *address,
                                       int *decoder_type,
                                       char *group_text,
                                       size_t group_capacity,
                                       char *name,
                                       size_t name_capacity)
{
    size_t length;
    size_t group_length;
    size_t name_length;
    size_t group_offset;
    size_t name_length_offset;
    size_t name_offset;
    size_t index;
    char byte_text[3];

    if (!hex_value || !address || !decoder_type || !group_text ||
            !name || group_capacity < 2U || name_capacity < 2U) {
        return 0;
    }
    length = strlen(hex_value);
    if (length < 20U || strncasecmp(hex_value, "AA55FE", 6) != 0 ||
            strcasecmp(hex_value + length - 4U, "55AA") != 0) {
        return 0;
    }
    byte_text[0] = hex_value[6]; byte_text[1] = hex_value[7]; byte_text[2] = 0;
    if (!isxdigit((unsigned char)byte_text[0]) ||
            !isxdigit((unsigned char)byte_text[1])) return 0;
    *address = (int)strtol(byte_text, NULL, 16);
    byte_text[0] = hex_value[8]; byte_text[1] = hex_value[9];
    if (!isxdigit((unsigned char)byte_text[0]) ||
            !isxdigit((unsigned char)byte_text[1])) return 0;
    *decoder_type = (int)strtol(byte_text, NULL, 16);
    byte_text[0] = hex_value[10]; byte_text[1] = hex_value[11];
    if (!isxdigit((unsigned char)byte_text[0]) ||
            !isxdigit((unsigned char)byte_text[1])) return 0;
    group_length = (size_t)strtol(byte_text, NULL, 16);
    group_offset = 12U;
    name_length_offset = group_offset + group_length * 2U;
    if (group_length == 0U || group_length >= group_capacity ||
            name_length_offset + 2U >= length - 4U) return 0;

    for (index = 0U; index < group_length; ++index) {
        const size_t offset = group_offset + index * 2U;
        if (!isxdigit((unsigned char)hex_value[offset]) ||
                !isxdigit((unsigned char)hex_value[offset + 1U])) return 0;
        byte_text[0] = hex_value[offset]; byte_text[1] = hex_value[offset + 1U];
        group_text[index] = (char)strtol(byte_text, NULL, 16);
        if (!(group_text[index] == '-' || group_text[index] == '*' ||
                group_text[index] == '=' ||
                group_text[index] == ',' ||
                (group_text[index] >= '0' && group_text[index] <= '9'))) return 0;
    }
    group_text[group_length] = '\0';

    byte_text[0] = hex_value[name_length_offset];
    byte_text[1] = hex_value[name_length_offset + 1U];
    if (!isxdigit((unsigned char)byte_text[0]) ||
            !isxdigit((unsigned char)byte_text[1])) return 0;
    name_length = (size_t)strtol(byte_text, NULL, 16);
    name_offset = name_length_offset + 2U;
    if (name_length == 0U || name_length >= name_capacity ||
            name_offset + name_length * 2U + 4U != length) return 0;
    for (index = 0U; index < name_length; ++index) {
        const size_t offset = name_offset + index * 2U;
        if (!isxdigit((unsigned char)hex_value[offset]) ||
                !isxdigit((unsigned char)hex_value[offset + 1U])) return 0;
        byte_text[0] = hex_value[offset]; byte_text[1] = hex_value[offset + 1U];
        name[index] = (char)strtol(byte_text, NULL, 16);
    }
    name[name_length] = '\0';
    return *address >= 20 && *address <= 255 &&
           (*decoder_type == 2 || *decoder_type == 3);
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
        /* Keep the DP's current value on the home-page network snapshot. */
        report_network_state(context);
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

static int strip_cj96_base_hash_frame(const char *value,
                                      char *normalized,
                                      size_t normalized_size,
                                      unsigned int *base_hash)
{
    size_t length;
    size_t marker;
    char hash_text[9];
    char *end;
    unsigned long parsed;
    if (!value || !normalized || normalized_size == 0 || !base_hash) return 0;
    length = strlen(value);
    if (length < 14 || length + 1 > normalized_size) {
        snprintf(normalized, normalized_size, "%s", value ? value : "");
        return 0;
    }
    marker = length - 14;
    if (strncasecmp(value + marker, "EE", 2) != 0 ||
            strcasecmp(value + length - 4, "55AA") != 0) {
        snprintf(normalized, normalized_size, "%s", value);
        return 0;
    }
    memcpy(hash_text, value + marker + 2, 8);
    hash_text[8] = '\0';
    parsed = strtoul(hash_text, &end, 16);
    if (!end || *end != '\0' || parsed > 0xFFFFFFFFUL) {
        snprintf(normalized, normalized_size, "%s", value);
        return 0;
    }
    memcpy(normalized, value, marker);
    memcpy(normalized + marker, value + marker + 10, length - marker - 10);
    normalized[length - 10] = '\0';
    *base_hash = (unsigned int)parsed;
    return 1;
}

static void append_cj96_base_hash(char *command,
                                  size_t command_size,
                                  int has_base_hash,
                                  unsigned int base_hash)
{
    size_t length;
    if (!command || command_size == 0 || !has_base_hash) return;
    length = strlen(command);
    if (length >= command_size) return;
    snprintf(command + length, command_size - length, "|base=%08X", base_hash);
}

static void handle_cj96_property_set_value(const char *hex_value)
{
    int value;
    int group_no;
    int capacity;
    int group_bind_code;
    int group_bind_addresses[60];
    int group_rename_no;
    size_t group_bind_count = 0;
    unsigned int version_hash;
    unsigned int base_hash = 0U;
    int has_base_hash = 0;
    char normalized_hex[CJ96_VALUE_MAX];
    char group_rename_name[64];
    int device_upsert_address;
    int device_upsert_type;
    char device_upsert_group[64];
    char device_upsert_name[32];
    char command[192];

    if (!hex_value) {
        trace_event("property set ignored: empty value");
        return;
    }
    has_base_hash = strip_cj96_base_hash_frame(hex_value, normalized_hex,
                                               sizeof(normalized_hex), &base_hash);
    if (has_base_hash) {
        hex_value = normalized_hex;
        trace_event("property set revision stripped base=%08X", base_hash);
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
    } else if (strcasecmp(hex_value, CJ96_HOME_STOP_SCHEDULE_VALUE) == 0) {
        trace_event("property set command stop schedule=%s", hex_value);
        write_home_command("stop_schedule");
    } else if (strcasecmp(hex_value, CJ96_HOME_ADVANCE_GROUP_VALUE) == 0) {
        trace_event("property set command advance group=%s", hex_value);
        write_home_command("advance_group");
    } else if (strcasecmp(hex_value, CJ96_DEVICE_TABLE_READ_VALUE) == 0) {
        trace_event("property set command device table read=%s", hex_value);
        reset_device_sync_report_queue();
        enqueue_tuya_command("device_sync", "read");
    } else if (parse_device_table_version_command(hex_value, &version_hash)) {
        char sync_command[64];
        trace_event("property set command device table version=%08X", version_hash);
        reset_device_sync_report_queue();
        snprintf(sync_command, sizeof(sync_command), "version=%08X", version_hash);
        enqueue_tuya_command("device_sync", sync_command);
    } else if (strcasecmp(hex_value, CJ96_DEVICE_SYNC_VALUE) == 0) {
        trace_event("property set command device sync=%s", hex_value);
        reset_device_sync_report_queue();
        enqueue_tuya_command("device_sync", "sync");
    } else if (strncasecmp(hex_value, "CJDS1|A|", 8) == 0) {
        char prefix[8];
        char kind;
        char session[8];
        unsigned int sequence = 0U;
        if (sscanf(hex_value, "%7[^|]|%c|%7[^|]|%u",
                   prefix, &kind, session, &sequence) == 4 &&
                strcmp(prefix, "CJDS1") == 0 && kind == 'A' &&
                sequence > 0U && sequence <= 10002U) {
            char ack_text[32];
            ensure_command_directory();
            snprintf(ack_text, sizeof(ack_text), "%u%c", sequence, 10);
            if (atomic_write_text(CJ96_DEVICE_SYNC_ACK_PATH, ack_text) == 0) {
                trace_event("device sync app ack session=%s sequence=%u",
                            session, sequence);
            } else {
                trace_event("device sync app ack write failed session=%s sequence=%u errno=%d",
                            session, sequence, errno);
            }
        }
    } else if (parse_group_bind_command(hex_value, &group_bind_code,
                                        group_bind_addresses,
                                        sizeof(group_bind_addresses) / sizeof(group_bind_addresses[0]),
                                        &group_bind_count)) {
        size_t index;
        int offset = snprintf(command, sizeof(command), "group_bind=%d", group_bind_code);
        for (index = 0; index < group_bind_count && offset > 0 &&
                (size_t)offset < sizeof(command); ++index) {
            offset += snprintf(command + offset, sizeof(command) - (size_t)offset,
                               ",%d", group_bind_addresses[index]);
        }
        if (offset <= 0 || (size_t)offset >= sizeof(command)) {
            trace_event("property set group bind command too long frame=%s", hex_value);
        } else {
            append_cj96_base_hash(command, sizeof(command), has_base_hash, base_hash);
            trace_event("property set command %s frame=%s", command, hex_value);
            write_home_command(command);
        }
    } else if (parse_group_rename_command(hex_value, &group_rename_no,
                                          group_rename_name,
                                          sizeof(group_rename_name))) {
        snprintf(command, sizeof(command), "group_rename=%d,%s",
                 group_rename_no, group_rename_name);
        append_cj96_base_hash(command, sizeof(command), has_base_hash, base_hash);
        trace_event("property set command %s frame=%s", command, hex_value);
        write_home_command(command);
    } else if (parse_byte_command(hex_value, "AA55FB", 1, 255, &value)) {
        snprintf(command, sizeof(command), "group_clear=%d", value);
        trace_event("property set command %s frame=%s", command, hex_value);
        append_cj96_base_hash(command, sizeof(command), has_base_hash, base_hash);
        write_home_command(command);
    } else if (parse_byte_command(hex_value, "AA55FC", 1, 128, &value)) {
        snprintf(command, sizeof(command), "group_delete=%d", value);
        trace_event("property set command %s frame=%s", command, hex_value);
        append_cj96_base_hash(command, sizeof(command), has_base_hash, base_hash);
        write_home_command(command);
    } else if (parse_two_byte_command(hex_value, "AA55F8", 1, 128, 1, 3,
                                      &group_no, &capacity)) {
        snprintf(command, sizeof(command), "group_auto_assign_single=%d,%d",
                 group_no, capacity);
        trace_event("property set command %s frame=%s", command, hex_value);
        append_cj96_base_hash(command, sizeof(command), has_base_hash, base_hash);
        write_home_command(command);
    } else if (parse_byte_command(hex_value, "AA55F9", 1, 3, &value)) {
        snprintf(command, sizeof(command), "group_auto_assign_all=%d", value);
        trace_event("property set command %s frame=%s", command, hex_value);
        append_cj96_base_hash(command, sizeof(command), has_base_hash, base_hash);
        write_home_command(command);
    } else if (parse_byte_command(hex_value, "AA55F7", 1, 3, &value)) {
        snprintf(command, sizeof(command), "group_auto_assign=%d", value);
        trace_event("property set command %s frame=%s", command, hex_value);
        append_cj96_base_hash(command, sizeof(command), has_base_hash, base_hash);
        write_home_command(command);
    } else if (parse_byte_command(hex_value, "AA55F6", 20, 255, &value)) {
        snprintf(command, sizeof(command), "device_delete=%d", value);
        trace_event("property set command %s frame=%s", command, hex_value);
        append_cj96_base_hash(command, sizeof(command), has_base_hash, base_hash);
        write_home_command(command);
    } else if (parse_byte_command(hex_value, "AA55F3", 1, 30, &value)) {
        snprintf(command, sizeof(command), "rain_delay=%d", value);
        trace_event("property set command %s frame=%s", command, hex_value);
        write_home_command(command);
    } else if (parse_byte_command(hex_value, "AA55F4", 0, 100, &value)) {
        snprintf(command, sizeof(command), "humidity_threshold=%d", value);
        trace_event("property set command %s frame=%s", command, hex_value);
        write_home_command(command);
    } else if (parse_device_upsert_command(hex_value, &device_upsert_address,
                                           &device_upsert_type,
                                           device_upsert_group,
                                           sizeof(device_upsert_group),
                                           device_upsert_name,
                                           sizeof(device_upsert_name))) {
        snprintf(command, sizeof(command), "device_upsert=%d,%d,%s,%s",
                 device_upsert_address, device_upsert_type,
                 device_upsert_group, device_upsert_name);
        append_cj96_base_hash(command, sizeof(command), has_base_hash, base_hash);
        trace_event("property set command %s frame=%s", command, hex_value);
        write_home_command(command);
    } else if (parse_two_byte_command(hex_value, "AA55E4", 1, 255, 0, 1,
                                      &group_no, &capacity)) {
        snprintf(command, sizeof(command), "device_state=%d,%d", group_no, capacity);
        trace_event("property set command %s frame=%s", command, hex_value);
        write_home_command(command);
    } else if (parse_two_byte_command(hex_value, "AA55E5", 1, 128, 0, 1,
                                      &group_no, &capacity)) {
        snprintf(command, sizeof(command), "group_state=%d,%d", group_no, capacity);
        trace_event("property set command %s frame=%s", command, hex_value);
        write_home_command(command);
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
    s_force_connect_pending = 0;
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
    report_cj96_value(context, CJ96_HEARTBEAT_VALUE);
    report_screen_power_state(context, true);
    report_network_state(context);
    report_ota_version(context, "INIT");
}

static void on_disconnect(tuya_mqtt_context_t *context, void *user_data)
{
    (void)context;
    (void)user_data;
    s_connected = 0;
    s_ota_check_pending = 0;
    trace_event("disconnected");
    printf("CJ96 Tuya disconnected\n");
}

static void on_messages(tuya_mqtt_context_t *context, void *user_data, const tuyalink_message_t *msg)
{
    (void)context;
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

    /* property_set_cb() is registered for the same property/set topic and is
     * the single execution path for downlink commands. The SDK also forwards
     * that message through on_messages(), so executing it here would enqueue
     * every command twice. */
    if (msg->type == THING_TYPE_PROPERTY_SET && msg->data_string) {
        trace_event("property set already handled by topic callback msgid=%s",
                    msg->msgid ? msg->msgid : "");
    }

    if (s_ota_check_pending &&
            (msg->type == THING_TYPE_OTA_ISSUE || msg->type == THING_TYPE_OTA_GET_RSP)) {
        s_ota_check_pending = 0;
        handle_ota_message(context, msg->data_string);
    }
}


static int report_next_command_ack(tuya_mqtt_context_t *context)
{
    const char *roots[3];
    size_t root_count = 0U;
    char selected_path[256] = "";
    char selected_name[160] = "";
    size_t i;
    FILE *fp;
    char content[512];
    size_t length;
    int ret;

    roots[root_count++] = s_persistent_root;
    if (strcmp(s_persistent_root, CJ96_RUNTIME_DIR) != 0) {
        roots[root_count++] = CJ96_RUNTIME_DIR;
    }
    if (strcmp(s_persistent_root, CJ96_PERSIST_ROOT_PRIMARY) != 0) {
        roots[root_count++] = CJ96_PERSIST_ROOT_PRIMARY;
    }

    for (i = 0U; i < root_count; ++i) {
        char directory_path[192];
        DIR *directory;
        struct dirent *entry;
        snprintf(directory_path, sizeof(directory_path), "%s/ack", roots[i]);
        directory = opendir(directory_path);
        if (directory == NULL) {
            continue;
        }
        while ((entry = readdir(directory)) != NULL) {
            const size_t name_length = strlen(entry->d_name);
            if (strncmp(entry->d_name, "ack_", 4) != 0 ||
                    name_length <= 9U ||
                    strcmp(entry->d_name + name_length - 5U, ".cj96") != 0) {
                continue;
            }
            if (selected_name[0] == 0 || strcmp(entry->d_name, selected_name) < 0) {
                snprintf(selected_name, sizeof(selected_name), "%s", entry->d_name);
                snprintf(selected_path, sizeof(selected_path), "%s/%s", directory_path, entry->d_name);
            }
        }
        closedir(directory);
    }

    if (selected_path[0] == 0) {
        return 0;
    }
    fp = fopen(selected_path, "rb");
    if (fp == NULL) {
        trace_event("command ack open failed path=%s errno=%d", selected_path, errno);
        return 0;
    }
    if (fgets(content, sizeof(content), fp) == NULL) {
        fclose(fp);
        unlink(selected_path);
        trace_event("command ack empty path=%s", selected_path);
        return 0;
    }
    fclose(fp);
    length = strlen(content);
    while (length > 0U && (content[length - 1U] == 13 || content[length - 1U] == 10)) {
        content[--length] = 0;
    }
    if (strncmp(content, CJ96_ACK_PROTOCOL "|", 7) != 0) {
        unlink(selected_path);
        trace_event("command ack invalid path=%s content=%s", selected_path, content);
        return 0;
    }
    ret = report_cj96_value(context, content);
    if (ret >= OPRT_OK) {
        unlink(selected_path);
        trace_event("command ack reported path=%s content=%s", selected_path, content);
        return 1;
    }
    trace_event("command ack report failed ret=%d path=%s", ret, selected_path);
    return 0;
}

int main(int argc, char **argv)
{
    const char *config_path = "/data/cj96_tuya_demo.conf";
    int ret;
    unsigned int now_ms;
    unsigned int last_connected_ms;
    unsigned int last_report_ms;
    unsigned int last_network_check_ms;
    unsigned int next_connect_ms;
    char detected_network[sizeof(s_active_network)];

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    signal(SIGPIPE, SIG_IGN);

    if (argc > 1) {
        config_path = argv[1];
    }

    ensure_persistent_storage();

    if (load_config(config_path, &s_config) != 0) {
        return 2;
    }

    if (s_config.daemonize && daemonize_process() != 0) {
        fprintf(stderr, "daemonize failed\n");
        trace_event("daemonize failed");
        return 5;
    }

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
        .loop_timeout_ms = CJ96_MQTT_LOOP_TIMEOUT_MS,
        .on_connected = on_connected,
        .on_disconnect = on_disconnect,
        .on_messages = on_messages,
    });
    if (ret != OPRT_OK) {
        fprintf(stderr, "tuya_mqtt_init failed: %d\n", ret);
        trace_event("tuya_mqtt_init failed: %d", ret);
        return 3;
    }

    detect_active_network(s_active_network, sizeof(s_active_network));
    trace_event("active network=%s", s_active_network);
    last_connected_ms = system_ticks();
    last_report_ms = last_connected_ms;
    last_network_check_ms = last_connected_ms;
    next_connect_ms = last_connected_ms;
    s_force_connect_pending = 1;

    for (;;) {
        now_ms = system_ticks();

        if ((now_ms - last_network_check_ms) >= CJ96_NETWORK_CHECK_INTERVAL_MS) {
            detect_active_network(detected_network, sizeof(detected_network));
            if (strcmp(detected_network, s_active_network) != 0) {
                trace_event("network changed %s -> %s", s_active_network, detected_network);
                strncpy(s_active_network, detected_network, sizeof(s_active_network) - 1);
                s_active_network[sizeof(s_active_network) - 1] = '\0';
                s_connected = 0;
                (void)tuya_mqtt_disconnect(&s_client);
                s_force_connect_pending = 1;
                next_connect_ms = now_ms + CJ96_NETWORK_SETTLE_MS;
                last_connected_ms = now_ms;
            }
            last_network_check_ms = now_ms;
        }

        if (s_force_connect_pending) {
            if (strcmp(s_active_network, "none") != 0
                    && tick_deadline_reached(now_ms, next_connect_ms)) {
                trace_event("connect on active network=%s", s_active_network);
                ret = tuya_mqtt_connect(&s_client);
                trace_event("tuya_mqtt_connect ret=%d network=%s", ret, s_active_network);
                if (ret == OPRT_OK) {
                    s_force_connect_pending = 0;
                    last_connected_ms = now_ms;
                } else {
                    next_connect_ms = now_ms + CJ96_CONNECT_RETRY_MS;
                }
            }
            system_sleep(50);
            continue;
        }

        ret = tuya_mqtt_loop(&s_client);
        if (ret != OPRT_OK) {
            trace_event("tuya_mqtt_loop failed ret=%d, waiting for reconnect", ret);
            s_connected = 0;
            (void)tuya_mqtt_disconnect(&s_client);
            s_force_connect_pending = 1;
            next_connect_ms = now_ms + CJ96_CONNECT_RETRY_MS;
            system_sleep(50);
            continue;
        }

        if (s_connected) {
            last_connected_ms = now_ms;
            if (consume_ota_request()) {
                (void)atomic_write_text(CJ96_OTA_STATUS_PATH,
                        "state=checking\nprogress=0\nmessage=\n");
                /* Set this before publishing: an OTA response can arrive in the
                 * next MQTT yield immediately after the request is accepted. */
                s_ota_check_pending = 1;
                ret = tuyalink_ota_get(&s_client, s_config.device_id);
                trace_event("OTA check requested from local UI ret=%d", ret);
                /* tuyalink_message_send returns a positive MQTT message ID
                 * when the publish is accepted, not OPRT_OK (zero). */
                if (ret < 0) {
                    s_ota_check_pending = 0;
                    report_ota_error(&s_client, 40, "升级请求失败，请检查云端固件");
                }
            }
            report_next_device_sync_frame(&s_client, now_ms);
            report_next_command_ack(&s_client);
            report_screen_power_state(&s_client, false);
            if ((now_ms - last_report_ms) >= 60000U) {
                report_cj96_value(&s_client, CJ96_HEARTBEAT_VALUE);
                report_screen_power_state(&s_client, true);
                report_network_state(&s_client);
                last_report_ms = now_ms;
            }
        } else if ((now_ms - last_connected_ms) >= CJ96_DISCONNECTED_RESET_MS) {
            trace_event("mqtt disconnected too long, resetting session");
            (void)tuya_mqtt_disconnect(&s_client);
            s_force_connect_pending = 1;
            next_connect_ms = now_ms + CJ96_CONNECT_RETRY_MS;
            last_connected_ms = now_ms;
        }

        system_sleep(50);
    }

    return 0;
}
