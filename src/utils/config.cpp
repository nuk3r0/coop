#include "config.h"
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

void Config::Load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;
    try {
        nlohmann::json j;
        f >> j;
        coopEnabled = j.value("coopEnabled", false);
        lastIp = j.value("lastIp", "127.0.0.1");
        lastPort = j.value("lastPort", 7777);
        isHost = j.value("isHost", false);
        ipHistory = j.value("ipHistory", std::vector<std::string>{});
        netTickRate = j.value("netTickRate", 60);
        uiOpacity = j.value("uiOpacity", 0.95f);
        showDebug = j.value("showDebug", false);
    } catch (...) {
        // Silently fail to default values
    }
}

void Config::Save(const std::string& path) {
    nlohmann::json j;
    j["coopEnabled"] = coopEnabled;
    j["lastIp"] = lastIp;
    j["lastPort"] = lastPort;
    j["isHost"] = isHost;
    j["ipHistory"] = ipHistory;
    j["netTickRate"] = netTickRate;
    j["uiOpacity"] = uiOpacity;
    j["showDebug"] = showDebug;
    std::ofstream f(path);
    if (f.is_open()) f << j.dump(4);
}
