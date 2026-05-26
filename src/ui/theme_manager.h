#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace UI {
    struct ThemeColors {
        // Основные цвета
        uint32_t bg_primary;      // Основной фон
        uint32_t bg_secondary;    // Вторичный фон
        uint32_t bg_tertiary;     // Третичный фон
        
        // Цвета текста
        uint32_t text_primary;    // Основной текст
        uint32_t text_secondary;  // Вторичный текст
        uint32_t text_disabled;   // Неактивный текст
        
        // Акцентные цвета
        uint32_t accent_primary;  // Основной акцент
        uint32_t accent_hover;    // Акцент при наведении
        uint32_t accent_active;   // Акцент при нажатии
        
        // Статусы
        uint32_t success;         // Успех
        uint32_t warning;         // Предупреждение
        uint32_t error;           // Ошибка
        uint32_t info;            // Информация
        
        // Границы
        uint32_t border;          // Цвет границ
        uint32_t border_active;   // Активная граница
        
        // Градиенты для latency
        uint32_t latency_excellent;
        uint32_t latency_good;
        uint32_t latency_high;
        
        // Прозрачность (0-255)
        uint8_t alpha_bg;
        uint8_t alpha_overlay;
    };

    struct FontConfig {
        std::string font_path;
        float font_size_base;
        float font_size_small;
        float font_size_large;
        float font_size_title;
        float scale_factor;
        bool use_custom_font;
    };

    class ThemeManager {
    public:
        static ThemeManager& Instance();
        
        // Загрузка темы из конфига
        bool LoadTheme(const std::string& config_path);
        bool SaveTheme(const std::string& config_path);
        
        // Применение темы к ImGui
        void ApplyToImGui();
        
        // Получение текущей темы
        const ThemeColors& GetTheme() const { return m_current_theme; }
        const FontConfig& GetFontConfig() const { return m_font_config; }
        
        // Установка темы по имени
        void SetTheme(const std::string& theme_name);
        
        // Масштабирование шрифтов
        void SetScaleFactor(float scale);
        float GetScaleFactor() const { return m_font_config.scale_factor; }
        
        // Предопределенные темы
        void LoadDarkModern();    // Современная темная (по умолчанию)
        void LoadLightClean();    // Светлая чистая
        void LoadCyberpunk();     // Киберпанк неон
        void LoadMidnight();      // Полночь синяя
        
    private:
        ThemeManager();
        ~ThemeManager();
        ThemeManager(const ThemeManager&) = delete;
        ThemeManager& operator=(const ThemeManager&) = delete;
        
        ThemeColors m_current_theme;
        FontConfig m_font_config;
        std::string m_current_theme_name;
        
        // Парсинг цветов из строки формата "#RRGGBB" или "0xRRGGBB"
        uint32_t ParseColor(const std::string& color_str);
        std::string ColorToString(uint32_t color);
    };
}
