#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Integrity {
    struct IntegrityResult {
        bool is_valid;
        std::string error_message;
        uint64_t file_size;
        std::string file_hash;
        uint64_t timestamp;
    };

    class IntegrityChecker {
    public:
        static IntegrityChecker& Instance();
        
        // Проверка целостности DLL перед инъекцией
        IntegrityResult CheckDLL(const std::string& dll_path);
        
        // Проверка целостности игрового exe
        IntegrityResult CheckGameEXE(const std::string& exe_path);
        
        // Вычисление хэша файла (упрощенный CRC32)
        std::string ComputeFileHash(const std::string& path);
        
        // Получение размера файла
        uint64_t GetFileSize(const std::string& path);
        
        // Проверка версии игры по размеру и хэшу
        bool VerifyGameVersion(const std::string& exe_path, const std::string& expected_version);
        
        // Создание отчета о проверке
        std::string GenerateReport(const std::string& dll_path, const std::string& exe_path);
        
    private:
        IntegrityChecker() = default;
        ~IntegrityChecker() = default;
        IntegrityChecker(const IntegrityChecker&) = delete;
        IntegrityChecker& operator=(const IntegrityChecker&) = delete;
        
        // CRC32 таблица
        uint32_t crc32_table[256];
        void InitCRC32Table();
        uint32_t ComputeCRC32(const uint8_t* data, size_t length);
    };
}
