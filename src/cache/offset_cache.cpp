#include "offset_cache.h"
#include "../../utils/logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;
namespace json {
    // Простая JSON парсилка для минимизации зависимостей
    inline std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r\"");
        size_t end = s.find_last_not_of(" \t\n\r\"");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }
    
    inline std::string extractValue(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t pos = json.find(searchKey);
        if (pos == std::string::npos) return "";
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        
        pos = json.find_first_not_of(" \t\n\r", pos + 1);
        if (pos == std::string::npos) return "";
        
        if (json[pos] == '"') {
            size_t end = json.find('"', pos + 1);
            if (end == std::string::npos) return "";
            return json.substr(pos + 1, end - pos - 1);
        } else {
            size_t end = json.find_first_of(",}", pos);
            if (end == std::string::npos) end = json.length();
            return trim(json.substr(pos, end - pos));
        }
    }
}

namespace Cache {

OffsetCache& OffsetCache::Instance() {
    static OffsetCache instance;
    return instance;
}

bool OffsetCache::Load(const std::string& path) {
    if (!fs::exists(path)) {
        LOG_INFO("[Cache] No cache file found at: %s", path.c_str());
        return false;
    }

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("[Cache] Failed to open cache file: %s", path.c_str());
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        m_last_game_version = json::extractValue(content, "game_version");
        std::string timestamp_str = json::extractValue(content, "timestamp");
        m_cache_timestamp = timestamp_str.empty() ? 0 : std::stoull(timestamp_str);

        // Парсим оффсеты
        size_t offsets_pos = content.find("\"offsets\"");
        if (offsets_pos == std::string::npos) {
            LOG_WARN("[Cache] No offsets section in cache file");
            return false;
        }

        // Находим массив оффсетов
        size_t array_start = content.find('[', offsets_pos);
        size_t array_end = content.find(']', array_start);
        if (array_start == std::string::npos || array_end == std::string::npos) {
            LOG_ERROR("[Cache] Invalid offsets array format");
            return false;
        }

        std::string array_content = content.substr(array_start + 1, array_end - array_start - 1);
        
        // Парсим каждый объект в массиве
        size_t obj_start = 0;
        while ((obj_start = array_content.find('{', obj_start)) != std::string::npos) {
            size_t obj_end = array_content.find('}', obj_start);
            if (obj_end == std::string::npos) break;

            std::string obj = array_content.substr(obj_start, obj_end - obj_start + 1);
            
            CachedOffset offset;
            offset.pattern_name = json::extractValue(obj, "pattern_name");
            std::string addr_str = json::extractValue(obj, "address");
            std::string off_str = json::extractValue(obj, "offset");
            offset.game_version = json::extractValue(obj, "game_version");
            
            if (!offset.pattern_name.empty() && !addr_str.empty() && !off_str.empty()) {
                offset.address = std::stoull(addr_str, nullptr, 16);
                offset.offset = std::stoi(off_str);
                offset.timestamp = m_cache_timestamp;
                
                m_offsets[offset.pattern_name] = offset;
                LOG_DEBUG("[Cache] Loaded cached offset: %s -> 0x%llX (+%d)", 
                         offset.pattern_name.c_str(), offset.address, offset.offset);
            }
            
            obj_start = obj_end + 1;
        }

        LOG_INFO("[Cache] Successfully loaded %zu cached offsets for version %s", 
                m_offsets.size(), m_last_game_version.c_str());
        return !m_offsets.empty();
    } catch (const std::exception& e) {
        LOG_ERROR("[Cache] Exception while loading cache: %s", e.what());
        return false;
    }
}

bool OffsetCache::Save(const std::string& path) {
    try {
        // Создаем директорию если не существует
        fs::path cache_path(path);
        if (cache_path.has_parent_path()) {
            fs::create_directories(cache_path.parent_path());
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("[Cache] Failed to create cache file: %s", path.c_str());
            return false;
        }

        auto now = std::chrono::system_clock::now();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();

        file << "{\n";
        file << "  \"game_version\": \"" << m_last_game_version << "\",\n";
        file << "  \"timestamp\": " << timestamp << ",\n";
        file << "  \"created\": \"" << __DATE__ " " __TIME__ << "\",\n";
        file << "  \"offsets\": [\n";

        bool first = true;
        for (const auto& [name, offset] : m_offsets) {
            if (!first) file << ",\n";
            first = false;
            
            file << "    {\n";
            file << "      \"pattern_name\": \"" << offset.pattern_name << "\",\n";
            file << "      \"address\": \"0x" << std::hex << offset.address << std::dec << "\",\n";
            file << "      \"offset\": " << offset.offset << ",\n";
            file << "      \"game_version\": \"" << offset.game_version << "\"\n";
            file << "    }";
        }

        file << "\n  ]\n";
        file << "}\n";
        file.close();

        LOG_INFO("[Cache] Successfully saved %zu offsets to %s", m_offsets.size(), path.c_str());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("[Cache] Exception while saving cache: %s", e.what());
        return false;
    }
}

bool OffsetCache::GetCachedOffset(const std::string& pattern_name, uintptr_t& address, int32_t& offset) {
    auto it = m_offsets.find(pattern_name);
    if (it != m_offsets.end()) {
        address = it->second.address;
        offset = it->second.offset;
        return true;
    }
    return false;
}

void OffsetCache::SetCachedOffset(const std::string& pattern_name, uintptr_t address, int32_t offset, const std::string& game_version) {
    CachedOffset cached;
    cached.pattern_name = pattern_name;
    cached.address = address;
    cached.offset = offset;
    cached.game_version = game_version;
    cached.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    m_offsets[pattern_name] = cached;
    m_last_game_version = game_version;
    
    LOG_DEBUG("[Cache] Cached offset: %s -> 0x%llX (+%d) for version %s", 
             pattern_name.c_str(), address, offset, game_version.c_str());
}

bool OffsetCache::IsCacheValid(const std::string& game_version) {
    if (m_offsets.empty()) return false;
    if (m_last_game_version != game_version) {
        LOG_WARN("[Cache] Cache version mismatch: expected %s, got %s", 
                m_last_game_version.c_str(), game_version.c_str());
        return false;
    }
    
    // Проверяем возраст кэша (не старше 24 часов)
    auto now = std::chrono::system_clock::now();
    uint64_t now_ts = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    if (now_ts - m_cache_timestamp > 86400) {
        LOG_WARN("[Cache] Cache is older than 24 hours");
        return false;
    }
    
    return true;
}

void OffsetCache::Clear() {
    m_offsets.clear();
    m_last_game_version.clear();
    m_cache_timestamp = 0;
    LOG_INFO("[Cache] Cache cleared");
}

} // namespace Cache
