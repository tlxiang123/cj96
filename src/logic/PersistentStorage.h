#ifndef LOGIC_PERSISTENTSTORAGE_H_
#define LOGIC_PERSISTENTSTORAGE_H_

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace cj96_persist {

inline const char* dataRoot() {
    return "/mnt/extsd/cj96_data";
}

inline const char* configDir() {
    return "/mnt/extsd/cj96_data/config";
}

inline const char* logDir() {
    return "/mnt/extsd/cj96_data/logs";
}

inline bool ensureDirectory(const char* path) {
    if (!path || !*path) {
        return false;
    }
    if (mkdir(path, 0775) == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

inline bool ensureDirectoryTree(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    for (size_t index = 1; index <= path.size(); ++index) {
        if (index != path.size() && path[index] != '/') {
            continue;
        }
        const std::string part = path.substr(0, index);
        if (part.size() <= 1) {
            continue;
        }
        if (!ensureDirectory(part.c_str())) {
            return false;
        }
    }
    return true;
}

inline bool ensureConfigDir() {
    return ensureDirectoryTree(configDir());
}

inline bool ensureLogDir() {
    return ensureDirectoryTree(logDir());
}

inline std::string configPath(const char* filename) {
    std::string path = configDir();
    path += "/";
    path += filename ? filename : "";
    return path;
}

inline const char* settingsPath() {
    return "/mnt/extsd/cj96_data/config/settings.tsv";
}

inline std::string logPath(const char* filename) {
    std::string path = logDir();
    path += "/";
    path += filename ? filename : "";
    return path;
}

enum EWriteTarget {
    WRITE_TARGET_SETTINGS = 1u << 0,
    WRITE_TARGET_VALVE_LOG = 1u << 1,
    WRITE_TARGET_DISPLAY_LOG = 1u << 2,
};

bool initializeAsyncWriter();
bool primeTextWrite(EWriteTarget target, const std::string& text);
bool queueTextWrite(EWriteTarget target, const std::string& text);
bool flushAsyncWrites(unsigned int targetMask);

inline bool writeAll(int fd, const char* data, size_t size) {
    size_t written = 0;
    while (written < size) {
        const ssize_t ret = write(fd, data + written, size - written);
        if (ret <= 0) {
            return false;
        }
        written += static_cast<size_t>(ret);
    }
    return true;
}

inline bool writeTextAtomic(const std::string& path, const std::string& text) {
    const std::string tmpPath = path + ".tmp";
    const int fd = open(tmpPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd < 0) {
        return false;
    }
    const bool writeOk = writeAll(fd, text.data(), text.size());
    const bool syncOk = writeOk && fsync(fd) == 0;
    const bool closeOk = close(fd) == 0;
    const bool ok = writeOk && syncOk && closeOk;
    if (!ok) {
        (void)unlink(tmpPath.c_str());
        return false;
    }
    if (rename(tmpPath.c_str(), path.c_str()) != 0) {
        (void)unlink(tmpPath.c_str());
        return false;
    }
    sync();
    return true;
}

inline bool readTextFile(const std::string& path, std::string& text) {
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp) {
        return false;
    }
    text.clear();
    char buffer[512];
    for (;;) {
        const size_t got = fread(buffer, 1, sizeof(buffer), fp);
        if (got > 0) {
            text.append(buffer, got);
        }
        if (got < sizeof(buffer)) {
            break;
        }
    }
    const bool ok = ferror(fp) == 0;
    fclose(fp);
    return ok;
}

inline std::vector<std::string> splitTabLine(const std::string& line) {
    std::vector<std::string> fields;
    size_t start = 0;
    for (;;) {
        const size_t pos = line.find('\t', start);
        if (pos == std::string::npos) {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    return fields;
}

inline std::string escapeField(const char* text) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(text ? text : "");
    std::string out;
    for (; *p != '\0'; ++p) {
        if (*p == '%' || *p == '\t' || *p == '\n' || *p == '\r') {
            char encoded[4] = {0};
            snprintf(encoded, sizeof(encoded), "%%%02X", static_cast<unsigned int>(*p));
            out += encoded;
        } else {
            out.push_back(static_cast<char>(*p));
        }
    }
    return out;
}

inline int fromHex(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

inline std::string unescapeField(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size()) {
            const int hi = fromHex(text[i + 1]);
            const int lo = fromHex(text[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(text[i]);
    }
    return out;
}

inline int parseInt(const std::string& text, int defaultValue, int minValue, int maxValue) {
    if (text.empty()) {
        return defaultValue;
    }
    char* end = NULL;
    long parsed = strtol(text.c_str(), &end, 10);
    if (end == text.c_str()) {
        return defaultValue;
    }
    if (parsed < minValue) {
        parsed = minValue;
    }
    if (parsed > maxValue) {
        parsed = maxValue;
    }
    return static_cast<int>(parsed);
}

inline bool parseBool(const std::string& text, bool defaultValue) {
    if (text == "1" || text == "true" || text == "TRUE") {
        return true;
    }
    if (text == "0" || text == "false" || text == "FALSE") {
        return false;
    }
    return defaultValue;
}

}  // namespace cj96_persist

void requestPersistentStateCheckpoint();
bool flushPersistentState();

#endif /* LOGIC_PERSISTENTSTORAGE_H_ */
