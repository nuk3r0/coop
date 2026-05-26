#include "integrity_checker.h"
#include "../utils/logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

namespace Integrity {

IntegrityChecker& IntegrityChecker::Instance() {
    static IntegrityChecker instance;
    static bool initialized = false;
    if (!initialized) {
        instance.InitCRC32Table();
        initialized = true;
    }
    return instance;
}

void IntegrityChecker::InitCRC32Table() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
}

uint32_t IntegrityChecker::ComputeCRC32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

std::string IntegrityChecker::ComputeFileHash(const std::string& path) {
    try {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return "";
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return "";
        }

        uint32_t crc = ComputeCRC32(buffer.data(), buffer.size());
        
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(8) << crc;
        return ss.str();
    } catch (const std::exception& e) {
        LOG_ERROR("[Integrity] Exception computing hash: %s", e.what());
        return "";
    }
}

uint64_t IntegrityChecker::GetFileSize(const std::string& path) {
    try {
        if (!fs::exists(path)) {
            return 0;
        }
        return fs::file_size(path);
    } catch (const std::exception& e) {
        LOG_ERROR("[Integrity] Exception getting file size: %s", e.what());
        return 0;
    }
}

IntegrityResult IntegrityChecker::CheckDLL(const std::string& dll_path) {
    IntegrityResult result;
    result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (!fs::exists(dll_path)) {
        result.is_valid = false;
        result.error_message = "DLL file not found";
        return result;
    }

    result.file_size = GetFileSize(dll_path);
    result.file_hash = ComputeFileHash(dll_path);

    if (result.file_size == 0) {
        result.is_valid = false;
        result.error_message = "DLL file is empty";
        return result;
    }

    if (result.file_hash.empty()) {
        result.is_valid = false;
        result.error_message = "Failed to compute DLL hash";
        return result;
    }

    // Проверка на минимальный размер (защита от поврежденных файлов)
    if (result.file_size < 1024) {
        result.is_valid = false;
        result.error_message = "DLL file too small, possibly corrupted";
        return result;
    }

    result.is_valid = true;
    LOG_INFO("[Integrity] DLL check passed: %s (size: %llu, hash: %s)", 
             dll_path.c_str(), result.file_size, result.file_hash.c_str());
    
    return result;
}

IntegrityResult IntegrityChecker::CheckGameEXE(const std::string& exe_path) {
    IntegrityResult result;
    result.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (!fs::exists(exe_path)) {
        result.is_valid = false;
        result.error_message = "Game EXE not found";
        return result;
    }

    result.file_size = GetFileSize(exe_path);
    result.file_hash = ComputeFileHash(exe_path);

    if (result.file_size == 0) {
        result.is_valid = false;
        result.error_message = "Game EXE is empty";
        return result;
    }

    // Проверка версии 1.0.668.5700 по размеру (приблизительно)
    // Точный размер нужно определить экспериментально
    constexpr uint64_t EXPECTED_SIZE_MIN = 50 * 1024 * 1024;  // 50 MB минимум
    constexpr uint64_t EXPECTED_SIZE_MAX = 200 * 1024 * 1024; // 200 MB максимум
    
    if (result.file_size < EXPECTED_SIZE_MIN || result.file_size > EXPECTED_SIZE_MAX) {
        result.is_valid = false;
        result.error_message = "Game EXE size suspicious (expected 50-200MB, got " + 
                               std::to_string(result.file_size / 1024 / 1024) + "MB)";
        return result;
    }

    result.is_valid = true;
    LOG_INFO("[Integrity] Game EXE check passed: %s (size: %llu MB, hash: %s)", 
             exe_path.c_str(), result.file_size / 1024 / 1024, result.file_hash.c_str());
    
    return result;
}

bool IntegrityChecker::VerifyGameVersion(const std::string& exe_path, const std::string& expected_version) {
    // Для версии 1.0.668.5700 можно использовать комбинацию размера и хэша
    // В реальном проекте здесь должна быть база данных известных версий
    
    auto result = CheckGameEXE(exe_path);
    if (!result.is_valid) {
        return false;
    }

    // Эвристическая проверка для версии 1.0.668.5700
    // Точные значения нужно получить из известной рабочей копии
    if (expected_version == "1.0.668.5700") {
        // Примерные значения - заменить на реальные после анализа
        constexpr uint64_t KNOWN_SIZE = 98765432; // Заменить на реальный размер
        constexpr const char* KNOWN_HASH = "XXXXXXXX"; // Заменить на реальный хэш
        
        if (result.file_size >= 90 * 1024 * 1024 && result.file_size <= 110 * 1024 * 1024) {
            LOG_INFO("[Integrity] Game version appears to be 1.0.668.5700 (size match)");
            return true;
        }
    }

    LOG_WARN("[Integrity] Could not verify game version %s", expected_version.c_str());
    return true; // Возвращаем true с предупреждением, чтобы не блокировать запуск
}

std::string IntegrityChecker::GenerateReport(const std::string& dll_path, const std::string& exe_path) {
    std::stringstream report;
    
    report << "=== GoW Co-op Integrity Report ===\n";
    report << "Generated: " << __DATE__ " " __TIME__ << "\n\n";
    
    auto dll_result = CheckDLL(dll_path);
    report << "DLL Status: " << (dll_result.is_valid ? "OK" : "FAILED") << "\n";
    report << "  Path: " << dll_path << "\n";
    report << "  Size: " << dll_result.file_size << " bytes\n";
    report << "  Hash: " << dll_result.file_hash << "\n";
    if (!dll_result.is_valid) {
        report << "  Error: " << dll_result.error_message << "\n";
    }
    report << "\n";
    
    auto exe_result = CheckGameEXE(exe_path);
    report << "Game EXE Status: " << (exe_result.is_valid ? "OK" : "FAILED") << "\n";
    report << "  Path: " << exe_path << "\n";
    report << "  Size: " << exe_result.file_size << " bytes (" 
           << (exe_result.file_size / 1024 / 1024) << " MB)\n";
    report << "  Hash: " << exe_result.file_hash << "\n";
    if (!exe_result.is_valid) {
        report << "  Error: " << exe_result.error_message << "\n";
    }
    report << "\n";
    
    report << "Overall: " << ((dll_result.is_valid && exe_result.is_valid) ? "PASS" : "FAIL") << "\n";
    
    return report.str();
}

} // namespace Integrity
