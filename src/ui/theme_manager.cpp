#include "theme_manager.h"
#include "../utils/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// ImGui includes
#include "imgui.h"

namespace UI {

// Helper для конвертации hex цвета в ImVec4 (0-1 float)
static inline ImVec4 HexToImVec4(uint32_t color, uint8_t alpha = 255) {
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    float a = alpha / 255.0f;
    return ImVec4(r, g, b, a);
}

ThemeManager& ThemeManager::Instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager() : m_current_theme_name("dark_modern") {
    LoadDarkModern();
    
    m_font_config.font_path = "";
    m_font_config.font_size_base = 16.0f;
    m_font_config.font_size_small = 12.0f;
    m_font_config.font_size_large = 20.0f;
    m_font_config.font_size_title = 24.0f;
    m_font_config.scale_factor = 1.0f;
    m_font_config.use_custom_font = false;
}

ThemeManager::~ThemeManager() = default;

uint32_t ThemeManager::ParseColor(const std::string& color_str) {
    if (color_str.empty()) return 0x000000;
    
    std::string str = color_str;
    // Удаляем # или 0x префикс
    if (str[0] == '#') str = str.substr(1);
    else if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) 
        str = str.substr(2);
    
    try {
        return std::stoul(str, nullptr, 16);
    } catch (...) {
        LOG_WARN("[Theme] Failed to parse color: %s", color_str.c_str());
        return 0x000000;
    }
}

std::string ThemeManager::ColorToString(uint32_t color) {
    std::stringstream ss;
    ss << "#" << std::hex << std::setfill('0') << std::setw(6) << color;
    return ss.str();
}

void ThemeManager::LoadDarkModern() {
    m_current_theme_name = "dark_modern";
    
    // Основной фон - глубокий темно-серый с синим оттенком
    m_current_theme.bg_primary = 0x0F0F14;
    m_current_theme.bg_secondary = 0x1A1A24;
    m_current_theme.bg_tertiary = 0x252530;
    
    // Текст - белый с легкой голубизной
    m_current_theme.text_primary = 0xE8E8F0;
    m_current_theme.text_secondary = 0xA0A0B0;
    m_current_theme.text_disabled = 0x606070;
    
    // Акцент - современный сине-фиолетовый
    m_current_theme.accent_primary = 0x6B5CFF;
    m_current_theme.accent_hover = 0x8B7DFF;
    m_current_theme.accent_active = 0x5A4ADF;
    
    // Статусы
    m_current_theme.success = 0x4CAF50;
    m_current_theme.warning = 0xFF9800;
    m_current_theme.error = 0xF44336;
    m_current_theme.info = 0x2196F3;
    
    // Границы
    m_current_theme.border = 0x3A3A4A;
    m_current_theme.border_active = 0x6B5CFF;
    
    // Latency градиенты
    m_current_theme.latency_excellent = 0x4CAF50;  // Зеленый
    m_current_theme.latency_good = 0xFF9800;       // Оранжевый
    m_current_theme.latency_high = 0xF44336;       // Красный
    
    // Прозрачность
    m_current_theme.alpha_bg = 240;
    m_current_theme.alpha_overlay = 200;
    
    LOG_INFO("[Theme] Loaded Dark Modern theme");
}

void ThemeManager::LoadLightClean() {
    m_current_theme_name = "light_clean";
    
    m_current_theme.bg_primary = 0xFFFFFF;
    m_current_theme.bg_secondary = 0xF5F5F5;
    m_current_theme.bg_tertiary = 0xE8E8E8;
    
    m_current_theme.text_primary = 0x1A1A1A;
    m_current_theme.text_secondary = 0x505050;
    m_current_theme.text_disabled = 0x909090;
    
    m_current_theme.accent_primary = 0x2196F3;
    m_current_theme.accent_hover = 0x42A5F5;
    m_current_theme.accent_active = 0x1976D2;
    
    m_current_theme.success = 0x4CAF50;
    m_current_theme.warning = 0xFF9800;
    m_current_theme.error = 0xF44336;
    m_current_theme.info = 0x2196F3;
    
    m_current_theme.border = 0xD0D0D0;
    m_current_theme.border_active = 0x2196F3;
    
    m_current_theme.latency_excellent = 0x4CAF50;
    m_current_theme.latency_good = 0xFF9800;
    m_current_theme.latency_high = 0xF44336;
    
    m_current_theme.alpha_bg = 250;
    m_current_theme.alpha_overlay = 230;
    
    LOG_INFO("[Theme] Loaded Light Clean theme");
}

void ThemeManager::LoadCyberpunk() {
    m_current_theme_name = "cyberpunk";
    
    m_current_theme.bg_primary = 0x0A0A0F;
    m_current_theme.bg_secondary = 0x14141F;
    m_current_theme.bg_tertiary = 0x1E1E2F;
    
    m_current_theme.text_primary = 0x00FFFF;
    m_current_theme.text_secondary = 0xFF00FF;
    m_current_theme.text_disabled = 0x404050;
    
    m_current_theme.accent_primary = 0xFF00FF;
    m_current_theme.accent_hover = 0xFF40FF;
    m_current_theme.accent_active = 0xCC00CC;
    
    m_current_theme.success = 0x00FF00;
    m_current_theme.warning = 0xFFFF00;
    m_current_theme.error = 0xFF0000;
    m_current_theme.info = 0x00FFFF;
    
    m_current_theme.border = 0xFF00FF;
    m_current_theme.border_active = 0x00FFFF;
    
    m_current_theme.latency_excellent = 0x00FF00;
    m_current_theme.latency_good = 0xFFFF00;
    m_current_theme.latency_high = 0xFF0000;
    
    m_current_theme.alpha_bg = 230;
    m_current_theme.alpha_overlay = 180;
    
    LOG_INFO("[Theme] Loaded Cyberpunk theme");
}

void ThemeManager::LoadMidnight() {
    m_current_theme_name = "midnight";
    
    m_current_theme.bg_primary = 0x0D1117;
    m_current_theme.bg_secondary = 0x161B22;
    m_current_theme.bg_tertiary = 0x21262D;
    
    m_current_theme.text_primary = 0xC9D1D9;
    m_current_theme.text_secondary = 0x8B949E;
    m_current_theme.text_disabled = 0x484F58;
    
    m_current_theme.accent_primary = 0x58A6FF;
    m_current_theme.accent_hover = 0x79C0FF;
    m_current_theme.accent_active = 0x1F6FEB;
    
    m_current_theme.success = 0x3FB950;
    m_current_theme.warning = 0xD29922;
    m_current_theme.error = 0xF85149;
    m_current_theme.info = 0x58A6FF;
    
    m_current_theme.border = 0x30363D;
    m_current_theme.border_active = 0x58A6FF;
    
    m_current_theme.latency_excellent = 0x3FB950;
    m_current_theme.latency_good = 0xD29922;
    m_current_theme.latency_high = 0xF85149;
    
    m_current_theme.alpha_bg = 245;
    m_current_theme.alpha_overlay = 210;
    
    LOG_INFO("[Theme] Loaded Midnight theme");
}

void ThemeManager::SetTheme(const std::string& theme_name) {
    if (theme_name == "dark_modern") LoadDarkModern();
    else if (theme_name == "light_clean") LoadLightClean();
    else if (theme_name == "cyberpunk") LoadCyberpunk();
    else if (theme_name == "midnight") LoadMidnight();
    else {
        LOG_WARN("[Theme] Unknown theme: %s, using dark_modern", theme_name.c_str());
        LoadDarkModern();
    }
}

void ThemeManager::SetScaleFactor(float scale) {
    m_font_config.scale_factor = std::max(0.5f, std::min(2.0f, scale));
    m_font_config.font_size_base = 16.0f * m_font_config.scale_factor;
    m_font_config.font_size_small = 12.0f * m_font_config.scale_factor;
    m_font_config.font_size_large = 20.0f * m_font_config.scale_factor;
    m_font_config.font_size_title = 24.0f * m_font_config.scale_factor;
    
    LOG_INFO("[Theme] Scale factor set to %.2f", m_font_config.scale_factor);
}

bool ThemeManager::LoadTheme(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            LOG_WARN("[Theme] Config not found, using defaults: %s", config_path.c_str());
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();
        
        // Простой парсинг JSON-like конфига
        auto getValue = [&](const std::string& key) -> std::string {
            std::string searchKey = "\"" + key + "\"";
            size_t pos = content.find(searchKey);
            if (pos == std::string::npos) return "";
            
            pos = content.find(':', pos);
            if (pos == std::string::npos) return "";
            
            pos = content.find_first_not_of(" \t\n\r", pos + 1);
            if (pos == std::string::npos) return "";
            
            if (content[pos] == '"') {
                size_t end = content.find('"', pos + 1);
                if (end == std::string::npos) return "";
                return content.substr(pos + 1, end - pos - 1);
            } else {
                size_t end = content.find_first_of(",}", pos);
                if (end == std::string::npos) end = content.length();
                std::string val = content.substr(pos, end - pos);
                // trim
                size_t start = val.find_first_not_of(" \t\n\r");
                size_t finish = val.find_last_not_of(" \t\n\r");
                if (start == std::string::npos) return "";
                return val.substr(start, finish - start + 1);
            }
        };
        
        // Загружаем тему
        std::string theme_name = getValue("theme");
        if (!theme_name.empty()) {
            SetTheme(theme_name);
        }
        
        // Загружаем шрифты
        std::string font_path = getValue("font_path");
        if (!font_path.empty()) {
            m_font_config.font_path = font_path;
            m_font_config.use_custom_font = true;
        }
        
        std::string scale_str = getValue("scale_factor");
        if (!scale_str.empty()) {
            float scale = std::stof(scale_str);
            SetScaleFactor(scale);
        }
        
        // Можно добавить загрузку кастомных цветов
        // ...
        
        LOG_INFO("[Theme] Loaded theme configuration from %s", config_path.c_str());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("[Theme] Exception loading theme: %s", e.what());
        return false;
    }
}

bool ThemeManager::SaveTheme(const std::string& config_path) {
    try {
        std::ofstream file(config_path);
        if (!file.is_open()) {
            LOG_ERROR("[Theme] Failed to create config: %s", config_path.c_str());
            return false;
        }
        
        file << "{\n";
        file << "  \"theme\": \"" << m_current_theme_name << "\",\n";
        file << "  \"font_path\": \"" << m_font_config.font_path << "\",\n";
        file << "  \"scale_factor\": " << m_font_config.scale_factor << ",\n";
        file << "  \"use_custom_font\": " << (m_font_config.use_custom_font ? "true" : "false") << ",\n";
        file << "\n";
        file << "  // Custom colors (override theme)\n";
        file << "  \"accent_primary\": \"" << ColorToString(m_current_theme.accent_primary) << "\",\n";
        file << "  \"bg_primary\": \"" << ColorToString(m_current_theme.bg_primary) << "\"\n";
        file << "}\n";
        
        file.close();
        LOG_INFO("[Theme] Saved theme configuration to %s", config_path.c_str());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("[Theme] Exception saving theme: %s", e.what());
        return false;
    }
}

void ThemeManager::ApplyToImGui() {
    // Применение темы через ImGui стиль будет реализовано в ui_manager.cpp
    // Эта функция предоставляет данные о теме
    LOG_DEBUG("[Theme] Applying theme: %s", m_current_theme_name.c_str());
}

} // namespace UI
