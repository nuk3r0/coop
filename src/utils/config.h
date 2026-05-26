#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Config {
    bool coopEnabled = false;
    std::string lastIp = "127.0.0.1";
    int lastPort = 7777;
    bool isHost = false;
    std::vector<std::string> ipHistory;
    int netTickRate = 60;
    float uiOpacity = 0.95f;
    bool showDebug = false;

    void Load(const std::string& path);
    void Save(const std::string& path);
};
