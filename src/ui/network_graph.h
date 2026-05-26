#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <chrono>

namespace UI {
    // Структура для хранения статистики сети
    struct NetworkStats {
        float latency_ms;           // Текущий пинг
        float packet_loss_pct;      // Потеря пакетов %
        uint64_t packets_sent;      // Отправлено пакетов
        uint64_t packets_received;  // Получено пакетов
        uint64_t bytes_sent;        // Отправлено байт
        uint64_t bytes_received;    // Получено байт
        uint64_t last_update;       // Время последнего обновления
    };

    class NetworkGraph {
    public:
        static NetworkGraph& Instance();
        
        // Обновление статистики
        void UpdateStats(float latency_ms, float packet_loss_pct = 0.0f);
        void RecordPacketSent(uint64_t bytes);
        void RecordPacketReceived(uint64_t bytes);
        
        // Получение статистики
        const NetworkStats& GetStats() const { return m_stats; }
        
        // Данные для графика (последние N значений)
        const std::vector<float>& GetLatencyHistory() const { return m_latency_history; }
        const std::vector<float>& GetPacketLossHistory() const { return m_packet_loss_history; }
        
        // Настройки графика
        void SetHistorySize(int size) { m_history_size = size; }
        int GetHistorySize() const { return m_history_size; }
        
        // Очистка статистики
        void Reset();
        
        // Получение строкового статуса latency
        std::string GetLatencyStatus() const;
        uint32_t GetLatencyColor() const;
        
    private:
        NetworkGraph();
        ~NetworkGraph() = default;
        NetworkGraph(const NetworkGraph&) = delete;
        NetworkGraph& operator=(const NetworkGraph&) = delete;
        
        NetworkStats m_stats;
        std::vector<float> m_latency_history;
        std::vector<float> m_packet_loss_history;
        int m_history_size;
        
        // Временные счетчики для агрегации
        uint64_t m_temp_packets_sent;
        uint64_t m_temp_packets_received;
        uint64_t m_temp_bytes_sent;
        uint64_t m_temp_bytes_received;
        std::chrono::steady_clock::time_point m_last_aggregation;
    };
}
