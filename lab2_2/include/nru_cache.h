#ifndef NRU_CACHE_H
#define NRU_CACHE_H

#include <windows.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

// Структура для хранения информации об открытом файле
struct FileHandleInternal {
    HANDLE hFile;          // Дескриптор файла
    std::string path;      // Путь к файлу
    off_t current_pos;     // Текущая позиция в файле
};

class NRUCache {
private:
    // Ключ для поиска блоков в кэше
    struct CacheKey {
        int fd;            // Идентификатор файла
        off_t block_number; // Номер блока

        // Оператор сравнения для использования в unordered_map
        bool operator==(const CacheKey& other) const {
            return fd == other.fd && block_number == other.block_number;
        }
    };

    // Хэш-функция для CacheKey
    struct CacheKeyHash {
        size_t operator()(const CacheKey& k) const {
            return std::hash<int>()(k.fd) ^ (std::hash<off_t>()(k.block_number) << 1);
        }
    };

    // Структура для хранения данных блока в кэше
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

    // Поиск блока в кэше
    CacheBlock* findBlock(int fd, off_t block_number) {
        CacheKey key{fd, block_number};
        auto it = cache_map.find(key);
        if (it != cache_map.end()) {
            it->second->accessed = true; // Помечаем блок как использованный
            return it->second;
        }
        return nullptr;
    }

    // Запись измененного блока на диск
    void writeBackBlock(CacheBlock* block) {
        if (!block->dirty) return; // Если блок не изменен, ничего не делаем

        FileHandleInternal& file = open_files[block->fd];
        LARGE_INTEGER file_size, new_pos;
        new_pos.QuadPart = block->block_number * block_size;

        if (!SetFilePointerEx(file.hFile, new_pos, NULL, FILE_BEGIN)) return;

        DWORD written;
        WriteFile(file.hFile, block->data.data(), block_size, &written, NULL);
        block->dirty = false; // Сбрасываем флаг изменений
    }

    // Вытеснение блока из кэша по алгоритму NRU
    void evictBlock() {
        std::vector<CacheBlock*> classes[4];

        // Классификация блоков по их состоянию
        for (auto& pair : cache_map) {
            CacheBlock* block = pair.second;
            int cls = 0;
            if (!block->accessed && !block->dirty) cls = 0;
            else if (!block->accessed && block->dirty) cls = 1;
            else if (block->accessed && !block->dirty) cls = 2;
            else cls = 3;
            classes[cls].push_back(block);
        }

        // Вытеснение блока из наименее приоритетного класса
        for (int i = 0; i < 4; ++i) {
            if (!classes[i].empty()) {
                CacheBlock* block = classes[i].front();
                writeBackBlock(block); // Записываем блок на диск, если он изменен
                cache_map.erase({block->fd, block->block_number});
                delete block;
                return;
            }
        }
    }

    // Загрузка блока данных из файла в кэш
    CacheBlock* loadBlock(int fd, off_t block_number) {
        while (cache_map.size() >= max_blocks)
            evictBlock(); // Вытесняем блоки, если кэш переполнен

        FileHandleInternal& file = open_files[fd];
        LARGE_INTEGER pos;
        pos.QuadPart = block_number * block_size;
        SetFilePointerEx(file.hFile, pos, NULL, FILE_BEGIN);

        CacheBlock* block = new CacheBlock(fd, block_number, block_size);
        DWORD read = 0;
        ReadFile(file.hFile, block->data.data(), block_size, &read, NULL);

        if (read < block_size)
            memset(block->data.data() + read, 0, block_size - read);

        cache_map[{fd, block_number}] = block;
        return block;
    }

public:
    // Конструктор
    NRUCache(size_t block_size, size_t max_blocks)
        : block_size(block_size), max_blocks(max_blocks), next_fd(1) {}

    // Деструктор
    ~NRUCache() {
        for (auto& pair : cache_map) {
            writeBackBlock(pair.second); // Записываем все измененные блоки на диск
            delete pair.second;
        }
        for (auto& pair : open_files)
            CloseHandle(pair.second.hFile); // Закрываем все открытые файлы
    }

    // Открытие файла
    int openFile(const char* path) {
        HANDLE hFile = CreateFileA(
            path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_NO_BUFFERING,
            nullptr
        );
        if (hFile == INVALID_HANDLE_VALUE) return -1;

        int fd = next_fd++;
        open_files[fd] = {hFile, path, 0};
        return fd;
    }

    // Закрытие файла
    int closeFile(int fd) {
        auto it = open_files.find(fd);
        if (it == open_files.end()) return -1;

        syncFile(fd); // Синхронизируем изменения
        CloseHandle(it->second.hFile);

        for (auto cit = cache_map.begin(); cit != cache_map.end(); ) {
            if (cit->first.fd == fd) {
                delete cit->second;
                cit = cache_map.erase(cit);
            } else ++cit;
        }

        open_files.erase(it);
        return 0;
    }

    // Чтение данных из файла
    ssize_t readFile(int fd, void* buf, size_t count) {
        auto it = open_files.find(fd);
        if (it == open_files.end()) return -1;

        FileHandleInternal& file = it->second;
        off_t start = file.current_pos;
        off_t end = start + count;
        char* dest = (char*)buf;
        ssize_t total = 0;

        for (off_t bn = start / block_size; bn <= (end - 1) / block_size; ++bn) {
            CacheBlock* block = findBlock(fd, bn);
            if (!block) block = loadBlock(fd, bn);
            if (!block) return -1;

            off_t block_start = bn * block_size;
            off_t read_start = std::max(start, block_start);
            off_t read_end = std::min(end, static_cast<off_t>(block_start + block_size));
            size_t offset = read_start - block_start;
            size_t bytes = read_end - read_start;

            memcpy(dest + total, block->data.data() + offset, bytes);
            total += bytes;
        }

        file.current_pos += total;
        return total;
    }

    // Запись данных в файл
    ssize_t writeFile(int fd, const void* buf, size_t count) {
        auto it = open_files.find(fd);
        if (it == open_files.end()) return -1;

        FileHandleInternal& file = it->second;
        off_t start = file.current_pos;
        off_t end = start + count;
        const char* src = (const char*)buf;
        ssize_t total = 0;

        for (off_t bn = start / block_size; bn <= (end - 1) / block_size; ++bn) {
            CacheBlock* block = findBlock(fd, bn);
            if (!block) block = loadBlock(fd, bn);
            if (!block) return -1;

            off_t block_start = bn * block_size;
            off_t write_start = std::max(start, block_start);
            off_t write_end = std::min(end, static_cast<off_t>(block_start + block_size));
            size_t offset = write_start - block_start;
            size_t bytes = write_end - write_start;

            memcpy(block->data.data() + offset, src + total, bytes);
            total += bytes;
            block->dirty = true; // Помечаем блок как измененный
        }

        file.current_pos += total;
        return total;
    }

    // Изменение текущей позиции в файле
    off_t seekFile(int fd, off_t offset, int whence) {
        auto it = open_files.find(fd);
        if (it == open_files.end()) return -1;

        LARGE_INTEGER size;
        switch (whence) {
            case SEEK_SET: it->second.current_pos = offset; break;
            case SEEK_CUR: it->second.current_pos += offset; break;
            case SEEK_END:
                GetFileSizeEx(it->second.hFile, &size);
                it->second.current_pos = size.QuadPart + offset;
                break;
            default: return -1;
        }
        return it->second.current_pos;
    }

    // Синхронизация измененных блоков с диском
    int syncFile(int fd) {
        auto it = open_files.find(fd);
        if (it == open_files.end()) return -1;

        for (auto& pair : cache_map) {
            CacheBlock* block = pair.second;
            if (block->fd == fd && block->dirty)
                writeBackBlock(block); // Записываем измененные блоки на диск
        }

        FlushFileBuffers(it->second.hFile);
        return 0;
    }
};

#endif //NRU_CACHE_H
