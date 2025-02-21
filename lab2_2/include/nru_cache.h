#ifndef NRU_CACHE_H
#define NRU_CACHE_H

#include <windows.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

struct FileHandleInternal {
    HANDLE hFile{};      // Дескриптор файла
    std::string path;    // Путь к файлу
    off_t current_pos{}; // Текущая позиция в файле
};

class NRUCache {
private:
    struct CacheKey {
        int fd;            // Идентификатор файла
        off_t block_number; // Номер блока

        bool operator==(const CacheKey& other) const {
            return fd == other.fd && block_number == other.block_number;
        }
    };

    struct CacheKeyHash {
        size_t operator()(const CacheKey& k) const {
            return std::hash<int>()(k.fd) ^ (std::hash<off_t>()(k.block_number) << 1);
        }
    };

    struct CacheBlock {
        int fd;                   // Идентификатор файла
        off_t block_number;       // Номер блока
        bool accessed;           // Был ли блок недавно использован
        bool dirty;              // Был ли блок изменен
        std::vector<char> data;   // Данные блока

        CacheBlock(int fd, off_t bn, size_t size)
            : fd(fd), block_number(bn), accessed(true), dirty(false), data(size) {}
    };

    size_t block_size;            // Размер блока данных
    size_t max_blocks;            // Максимальное количество блоков в кэше
    std::unordered_map<CacheKey, CacheBlock*, CacheKeyHash> cache_map; // Кэш блоков
    std::unordered_map<int, FileHandleInternal> open_files; // Открытые файлы
    int next_fd;                  // Следующий идентификатор файла

    CacheBlock* findBlock(int fd, off_t block_number);

    void writeBackBlock(CacheBlock* block);

    void evictBlock();

    CacheBlock* loadBlock(int fd, off_t block_number);

public:
    NRUCache(size_t block_size, size_t max_blocks);

    ~NRUCache();

    int openFile(const char* path);

    int closeFile(int fd);

    ssize_t readFile(int fd, void* buf, size_t count);

    ssize_t writeFile(int fd, const void* buf, size_t count);

    off_t seekFile(int fd, off_t offset, int whence);

    int syncFile(int fd);
};

#endif //NRU_CACHE_H
