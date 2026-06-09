/**
 * @file Config.h
 * @brief 配置文件读写模块
 *
 * 读取 GoBanBot.cfg 配置文件，支持 key = value 格式和 # 注释。
 * 配置文件不存在时自动生成默认配置。
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

struct Config {
    int depth = 6;
    int threads = 1;
    int maxTimeMs = 0;
    bool highlight = false;
};

inline std::string cfgTrim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

inline void saveDefaultConfig(const std::string& path) {
    std::ofstream f(path);
    if (!f) return;
    f << "# GoBanBot Configuration File\n";
    f << "# Modify values below and restart the program.\n\n";
    f << "depth = 6\n";
    f << "threads = 1\n";
    f << "max_time_ms = 0\n";
    f << "highlight = false\n";
}

inline Config loadConfig(const std::string& path = "GoBanBot.cfg") {
    Config config;
    std::ifstream f(path);
    if (!f) {
        saveDefaultConfig(path);
        std::cout << "Config file '" << path << "' not found. A default one has been created.\n";
        std::cout << "Edit it and restart the program.\n\n";
        return config;
    }
    std::string line;
    while (std::getline(f, line)) {
        std::string trimmed = cfgTrim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        size_t eq = trimmed.find('=');
        if (eq == std::string::npos) continue;
        std::string key = cfgTrim(trimmed.substr(0, eq));
        std::string value = cfgTrim(trimmed.substr(eq + 1));
        std::string lv = value;
        std::transform(lv.begin(), lv.end(), lv.begin(), ::tolower);

        if (key == "depth") {
            int v = std::stoi(value);
            if (v >= 2 && v <= 12) config.depth = v;
        } else if (key == "threads") {
            int v = std::stoi(value);
            if (v >= 1) config.threads = v;
        } else if (key == "max_time_ms") {
            int v = std::stoi(value);
            if (v >= 0) config.maxTimeMs = v;
        } else if (key == "highlight") {
            config.highlight = (lv == "true" || lv == "yes" || lv == "1");
        }
    }
    return config;
}

#endif
