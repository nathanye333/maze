#pragma once

#include <iostream>
#include <string>

inline void checkCondition(bool condition, const char* message, const char* file, int line, int& passed, int& failed) {
    if (condition) {
        ++passed;
    } else {
        ++failed;
        std::cerr << "FAIL: " << message << " (" << file << ":" << line << ")\n";
    }
}

#define CHECK(cond, msg) checkCondition(static_cast<bool>(cond), msg, __FILE__, __LINE__, passed, failed)
