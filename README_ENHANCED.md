# GoW Co-op Mod - Улучшенная версия

Модификация для добавления кооперативного режима в God of War (2018) на ПК с улучшенной системой сканирования адресов, современным UI и проверкой целостности.

## 🎯 Версия игры
- **EXE:** 1.0.668.5700
- **SteamDB:** build 18979360
- **Дата:** 20 August 2025

---

## ✨ Новые возможности

### 🔍 Автоматический сканер адресов (GameScanner + Cache)
**Решает проблему зависимости от версий игры**

- Множественные паттерны для поиска локального игрока
- Авто-определение оффсетов позиции (X/Y/Z)
- Поиск оффсета имени сущности
- Кэширование результатов в JSON
- Fallback на дефолтные значения если сканирование не удалось
- Поддержка нескольких версий игры одновременно

**Файлы:**
- `src/game_scanner/game_scanner.h/cpp` - Сканер паттернов
- `src/cache/offset_cache.h/cpp` - Система кэширования

**Использование:**
```cpp
#include "game_scanner/game_scanner.h"
#include "cache/offset_cache.h"

// Загрузка кэша
if (Cache::OffsetCache::Instance().Load("cache/offsets_cache.json")) {
    // Кэш загружен
}

// Сканирование
auto result = GameScanner::ScanAll();
if (result.success) {
    g_gow_config.pos_x = result.offset_pos_x;
    // ...
    
    // Сохранение в кэш
    Cache::OffsetCache::Instance().Save("cache/offsets_cache.json");
}
```

### 🎨 Современный UI с темами
**Полная переработка интерфейса**

- 4 встроенные темы: Dark Modern, Light Clean, Cyberpunk, Midnight
- Настройка масштаба шрифтов (0.5x - 2.0x)
- Конфигурация через JSON
- Цветовые индикаторы статуса
- Градиенты latency

**Файлы:**
- `src/ui/theme_manager.h/cpp` - Менеджер тем
- `src/ui/network_graph.h/cpp` - Графики сети
- `assets/config/ui_config.json` - Конфигурация UI

**Управление:**
- **F1** - Главное меню
- **F8** - Окно сети
- **Insert** - Быстрое переключение тем

### 📊 Графики Latency и Packet Loss
**Визуализация сетевого состояния**

- График пинга в реальном времени (последние 100 значений)
- График потерь пакетов
- Статус качества соединения (Excellent/Good/Fair/High/Very High)
- Цветовая индикация
- Агрегация статистики каждые 5 секунд

**Интеграция:**
```cpp
#include "ui/network_graph.h"

// В цикле обновления сети
UI::NetworkGraph::Instance().UpdateStats(latency_ms, packet_loss_pct);
UI::NetworkGraph::Instance().RecordPacketSent(bytes);
UI::NetworkGraph::Instance().RecordPacketReceived(bytes);

// Отрисовка графика в ImGui
ImPlot::PlotLine("Latency", 
    UI::NetworkGraph::Instance().GetLatencyHistory().data(),
    UI::NetworkGraph::Instance().GetLatencyHistory().size());
```

### 🔒 Проверка целостности (Integrity Checker)
**Безопасность перед инъекцией**

- CRC32 хэширование DLL и EXE
- Проверка размера файлов
- Верификация версии игры
- Генерация отчета о проверке

**Файлы:**
- `src/integrity/integrity_checker.h/cpp` - Проверка целостности

**Использование:**
```cpp
#include "integrity/integrity_checker.h"

// Перед запуском
auto& checker = Integrity::IntegrityChecker::Instance();

// Проверка DLL
auto dll_result = checker.CheckDLL("GoW_CoOp.dll");
if (!dll_result.is_valid) {
    LOG_ERROR("DLL integrity check failed: %s", dll_result.error_message.c_str());
    return false;
}

// Проверка игры
auto exe_result = checker.CheckGameEXE("GodOfWar.exe");
if (!exe_result.is_valid) {
    LOG_ERROR("Game EXE integrity check failed: %s", exe_result.error_message.c_str());
    return false;
}

// Генерация отчета
std::string report = checker.GenerateReport("GoW_CoOp.dll", "GodOfWar.exe");
LOG_INFO("%s", report.c_str());
```

---

## 📁 Структура проекта

```
GoW_CoOp/
├── src/
│   ├── core/              # Ядро, хуки, управление
│   ├── memory/            # Работа с памятью
│   ├── game_scanner/      # Автосканер адресов ✨ НОВОЕ
│   ├── cache/             # Кэширование оффсетов ✨ НОВОЕ
│   ├── integrity/         # Проверка целостности ✨ НОВОЕ
│   ├── network/           # Сетевой менеджер (UDP)
│   ├── render/            # Рендеринг (D3D12 + ImGui)
│   ├── ui/                # UI компоненты
│   │   ├── ui_manager.cpp # Основной UI
│   │   ├── theme_manager.*# Менеджер тем ✨ НОВОЕ
│   │   └── network_graph.*# Графики сети ✨ НОВОЕ
│   ├── replication/       # Синхронизация состояния
│   └── ...
├── assets/
│   └── config/
│       ├── ui_config.json     # Конфигурация UI ✨ НОВОЕ
│       └── cache_config.json  # Конфигурация кэша ✨ НОВОЕ
├── vendor/                # Сторонние библиотеки
└── GoW_CoOp.vcxproj       # Проект VS2022
```

---

## 🚀 Сборка и установка

### Требования
- Visual Studio 2022 (v145 toolset)
- Windows SDK 10.0+
- DirectX 12

### Сборка
1. Откройте `GoW_CoOp.sln` в Visual Studio
2. Выберите конфигурацию **Release|x64**
3. Сборка решения (Ctrl+Shift+B)
4. DLL будет создана в: `C:\Users\nukklye\Desktop\COOP\GoW_CoOp.dll`

### Интеграция новых компонентов

#### 1. Инициализация кэша в GameState.cpp
```cpp
#include "../cache/offset_cache.h"
#include "../integrity/integrity_checker.h"

bool GameState::Init() {
    // Проверка целостности
    auto& checker = Integrity::IntegrityChecker::Instance();
    if (!checker.CheckDLL("GoW_CoOp.dll").is_valid) {
        LOG_ERROR("Integrity check failed!");
        return false;
    }
    
    // Загрузка кэша
    if (Cache::OffsetCache::Instance().Load("cache/offsets_cache.json")) {
        LOG_INFO("Using cached offsets");
    } else {
        // Сканирование если кэш не найден
        auto scanResult = GameScanner::ScanAll();
        if (scanResult.success) {
            g_gow_config.pos_x = scanResult.offset_pos_x;
            g_gow_config.pos_y = scanResult.offset_pos_y;
            g_gow_config.pos_z = scanResult.offset_pos_z;
            
            // Сохранение в кэш
            Cache::OffsetCache::Instance().Save("cache/offsets_cache.json");
        }
    }
    
    // остальной код инициализации
    return true;
}
```

#### 2. Обновление UI с темой и графиками
```cpp
#include "ui/theme_manager.h"
#include "ui/network_graph.h"

void UIManager::Render() {
    // Загрузка темы из конфига
    static bool theme_loaded = false;
    if (!theme_loaded) {
        UI::ThemeManager::Instance().LoadTheme("assets/config/ui_config.json");
        theme_loaded = true;
    }
    
    // Применение темы
    ApplyTheme(UI::ThemeManager::Instance().GetTheme());
    
    // Отрисовка графика latency
    if (show_network_window) {
        ImGui::Begin("Network Status");
        
        const auto& stats = UI::NetworkGraph::Instance().GetStats();
        ImGui::Text("Latency: %.1f ms (%s)", 
            stats.latency_ms, 
            UI::NetworkGraph::Instance().GetLatencyStatus().c_str());
        
        // График
        if (ImPlot::BeginPlot("Latency Graph")) {
            ImPlot::PlotLine("Ping", 
                UI::NetworkGraph::Instance().GetLatencyHistory().data(),
                UI::NetworkGraph::Instance().GetLatencyHistory().size());
            ImPlot::EndPlot();
        }
        
        ImGui::End();
    }
}
```

#### 3. Обновление статистики сети в NetworkManager
```cpp
#include "ui/network_graph.h"

void NetworkManager::ProcessPacket(...) {
    // ... обработка пакета
    
    // Запись статистики
    UI::NetworkGraph::Instance().RecordPacketReceived(packet_size);
}

void NetworkManager::SendPacket(...) {
    // ... отправка пакета
    
    // Запись статистики
    UI::NetworkGraph::Instance().RecordPacketSent(packet_size);
}

void NetworkManager::Update() {
    float latency = CalculateLatency();
    float packet_loss = CalculatePacketLoss();
    
    UI::NetworkGraph::Instance().UpdateStats(latency, packet_loss);
}
```

---

## ⚙️ Конфигурация

### ui_config.json
```json
{
  "theme": "dark_modern",        // dark_modern, light_clean, cyberpunk, midnight
  "scale_factor": 1.0,           // Масштаб шрифтов (0.5 - 2.0)
  "font_path": "",               // Путь к кастомному шрифту
  "network_history_size": 100,   // Размер истории графиков
  "show_latency_graph": true,    // Показывать график пинга
  "show_packet_loss_graph": true // Показывать график потерь
}
```

### cache_config.json
```json
{
  "cache_enabled": true,
  "cache_path": "cache/offsets_cache.json",
  "cache_ttl_hours": 24,         // Время жизни кэша
  "auto_save_on_scan": true,     // Автосохранение после сканирования
  "game_version": "1.0.668.5700"
}
```

---

## 🎮 Использование в игре

1. **Запуск:** Инжектируйте DLL в процесс GodOfWar.exe
2. **Главное меню:** Нажмите **F1**
3. **Настройки сети:** Нажмите **F8**
4. **Смена темы:** Insert или через меню настроек
5. **Масштаб UI:** Ctrl+Scroll или через настройки

---

## 🛠️ Расширение функционала

### Добавление паттернов для новой версии игры

1. Найдите новые сигнатуры в IDA Pro/Ghidra
2. Добавьте в `src/game_scanner/game_scanner.cpp`:
```cpp
// Для версии 1.0.XXX.XXXX
patterns["local_player"].push_back({
    "NEW SIGATURE BYTES HERE",
    "xx xx ? ? ? ? xx xx",
    0x123  // оффсет
});
```

3. Протестируйте сканирование
4. Сохраните в кэш

### Создание новой темы

```cpp
void ThemeManager::LoadCustomTheme() {
    m_current_theme_name = "custom";
    
    m_current_theme.bg_primary = 0xRRGGBB;
    m_current_theme.accent_primary = 0xRRGGBB;
    // ... остальные цвета
    
    LOG_INFO("[Theme] Loaded Custom theme");
}
```

---

## 📝 Changelog

### v4.0 - Enhancement Update
- ✨ Добавлен автоматический сканер адресов
- ✨ Система кэширования оффсетов в JSON
- ✨ Менеджер тем с 4 preset'ами
- ✨ Графики latency и packet loss
- ✨ Проверка целостности DLL/EXE
- 🎨 Полностью переработанный современный UI
- 📊 Визуальная индикация статуса сети
- ⚙️ JSON конфигурация для всех систем

### v3.0 - Simple Co-op
- Базовый кооперативный режим
- UDP сеть
- ImGui рендер

---

## ⚠️ Предупреждения

- Используйте на свой страх и риск
- Возможна нестабильность на непроверенных версиях игры
- Не рекомендуется использовать в онлайн-режимах с античитом

---

## 📄 Лицензия

MIT License - свободное использование с указанием авторства.

## 👥 Авторы

Разработано сообществом GoW Modding Community.
