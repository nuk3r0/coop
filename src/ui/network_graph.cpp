#include "network_graph.h"
#include "../utils/logger.h"
#include <algorithm>
#include <numeric>

namespace UI {

NetworkGraph& NetworkGraph::Instance() {
    static NetworkGraph instance;
    return instance;
}

NetworkGraph::NetworkGraph() 
    : m_history_size(100)
    , m_temp_packets_sent(0)
    , m_temp_packets_received(0)
    , m_temp_bytes_sent(0)
    , m_temp_bytes_received(0)
    , m_last_aggregation(std::chrono::steady_clock::now())
{
    Reset();
    LOG_INFO("[NetworkGraph] Initialized with history size: %d", m_history_size);
}

void NetworkGraph::Reset() {
    m_stats = {};
    m_stats.last_update = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    m_latency_history.clear();
    m_packet_loss_history.clear();
    m_latency_history.reserve(m_history_size);
    m_packet_loss_history.reserve(m_history_size);
    
    m_temp_packets_sent = 0;
    m_temp_packets_received = 0;
    m_temp_bytes_sent = 0;
    m_temp_bytes_received = 0;
    m_last_aggregation = std::chrono::steady_clock::now();
    
    LOG_DEBUG("[NetworkGraph] Statistics reset");
}

void NetworkGraph::UpdateStats(float latency_ms, float packet_loss_pct) {
    auto now = std::chrono::steady_clock::now();
    
    // Обновляем текущие значения
    m_stats.latency_ms = latency_ms;
    m_stats.packet_loss_pct = packet_loss_pct;
    m_stats.last_update = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Добавляем в историю
    m_latency_history.push_back(latency_ms);
    m_packet_loss_history.push_back(packet_loss_pct);
    
    // Ограничиваем размер истории
    if (static_cast<int>(m_latency_history.size()) > m_history_size) {
        m_latency_history.erase(m_latency_history.begin());
    }
    if (static_cast<int>(m_packet_loss_history.size()) > m_history_size) {
        m_packet_loss_history.erase(m_packet_loss_history.begin());
    }
    
    // Агрегация счетчиков каждые 5 секунд
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_last_aggregation).count();
    if (elapsed >= 5) {
        m_stats.packets_sent += m_temp_packets_sent;
        m_stats.packets_received += m_temp_packets_received;
        m_stats.bytes_sent += m_temp_bytes_sent;
        m_stats.bytes_received += m_temp_bytes_received;
        
        m_temp_packets_sent = 0;
        m_temp_packets_received = 0;
        m_temp_bytes_sent = 0;
        m_temp_bytes_received = 0;
        m_last_aggregation = now;
    }
}

void NetworkGraph::RecordPacketSent(uint64_t bytes) {
    m_temp_packets_sent++;
    m_temp_bytes_sent += bytes;
}

void NetworkGraph::RecordPacketReceived(uint64_t bytes) {
    m_temp_packets_received++;
    m_temp_bytes_received += bytes;
}

std::string NetworkGraph::GetLatencyStatus() const {
    if (m_stats.latency_ms < 30.0f) return "Excellent";
    if (m_stats.latency_ms < 60.0f) return "Good";
    if (m_stats.latency_ms < 100.0f) return "Fair";
    if (m_stats.latency_ms < 200.0f) return "High";
    return "Very High";
}

uint32_t NetworkGraph::GetLatencyColor() const {
    if (m_stats.latency_ms < 30.0f) return 0x4CAF50;      // Зеленый
    if (m_stats.latency_ms < 60.0f) return 0x8BC34A;      // Светло-зеленый
    if (m_stats.latency_ms < 100.0f) return 0xFF9800;     // Оранжевый
    if (m_stats.latency_ms < 200.0f) return 0xFF5722;     // Глубокий оранжевый
    return 0xF44336;                                      // Красный
}

} // namespace UI
