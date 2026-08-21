#include "EnvFile.h"

#include <cstdlib>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <stdlib.h>
#endif

namespace {

std::string trim(std::string_view text) {
    size_t begin = 0;
    while (begin < text.size() && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r')) {
        ++begin;
    }
    size_t end = text.size();
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r')) {
        --end;
    }
    return std::string(text.substr(begin, end - begin));
}

std::string unquote(std::string value) {
    if (value.size() >= 2) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.substr(1, value.size() - 2);
        }
    }
    return value;
}

void setEnvIfMissing(const std::string& key, const std::string& value) {
    if (std::getenv(key.c_str()) != nullptr) {
        return;
    }
#ifdef _WIN32
    _putenv_s(key.c_str(), value.c_str());
#else
    setenv(key.c_str(), value.c_str(), 0);
#endif
}

}  // namespace

bool loadEnvFile(std::string_view path) {
    std::ifstream input{std::string(path)};
    if (!input) {
        return false;
    }

    std::string line;
    while (std::getline(input, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        if (trimmed.rfind("export ", 0) == 0) {
            trimmed = trim(trimmed.substr(7));
        }
        const size_t eq = trimmed.find('=');
        if (eq == std::string::npos || eq == 0) {
            continue;
        }
        const std::string key = trim(trimmed.substr(0, eq));
        const std::string value = unquote(trim(trimmed.substr(eq + 1)));
        if (key.empty()) {
            continue;
        }
        setEnvIfMissing(key, value);
    }
    return true;
}

std::string loadEnvFromDefaultLocations() {
    static const char* kCandidates[] = {
        ".env",
        "../.env",
        "../../.env",
        "../../../.env",
    };
    for (const char* path : kCandidates) {
        if (loadEnvFile(path)) {
            return path;
        }
    }
    return {};
}
