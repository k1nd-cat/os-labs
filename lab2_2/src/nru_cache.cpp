#include "nru_cache.h"

NRUCache::CacheBlock *NRUCache::findBlock(int fd, off_t block_number) {
    CacheKey key{fd, block_number};
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        it->second->accessed = true;
        return it->second;
    }
    return nullptr;
}

void NRUCache::writeBackBlock(CacheBlock *block)  {
    if (!block->dirty) return;

    FileHandleInternal& file = open_files[block->fd];
    LARGE_INTEGER file_size, new_pos;
    new_pos.QuadPart = block->block_number * block_size;

    if (!SetFilePointerEx(file.hFile, new_pos, nullptr, FILE_BEGIN)) return;

    DWORD written;
    WriteFile(file.hFile, block->data.data(), block_size, &written, nullptr);
    block->dirty = false;
}

void NRUCache::evictBlock() {
    std::vector<CacheBlock*> classes[4];

    for (auto& pair : cache_map) {
        CacheBlock* block = pair.second;
        int cls = 0;
        if (!block->accessed && !block->dirty) cls = 0;
        else if (!block->accessed && block->dirty) cls = 1;
        else if (block->accessed && !block->dirty) cls = 2;
        else cls = 3;
        classes[cls].push_back(block);
    }

    for (int i = 0; i < 4; ++i) {
        if (!classes[i].empty()) {
            CacheBlock* block = classes[i].front();
            writeBackBlock(block);
            cache_map.erase({block->fd, block->block_number});
            delete block;
            return;
        }
    }
}

NRUCache::CacheBlock * NRUCache::loadBlock(int fd, off_t block_number) {
    while (cache_map.size() >= max_blocks)
        evictBlock();

    FileHandleInternal& file = open_files[fd];
    LARGE_INTEGER pos;
    pos.QuadPart = block_number * block_size;
    SetFilePointerEx(file.hFile, pos, nullptr, FILE_BEGIN);

    CacheBlock* block = new CacheBlock(fd, block_number, block_size);
    DWORD read = 0;
    ReadFile(file.hFile, block->data.data(), block_size, &read, NULL);

    if (read < block_size)
        memset(block->data.data() + read, 0, block_size - read);

    cache_map[{fd, block_number}] = block;
    return block;
}

NRUCache::NRUCache(size_t block_size, size_t max_blocks)
    : block_size(block_size), max_blocks(max_blocks), next_fd(1) {
}


NRUCache::~NRUCache() {
    for (auto &pair: cache_map) {
        writeBackBlock(pair.second);
        delete pair.second;
    }
    for (auto &pair: open_files)
        CloseHandle(pair.second.hFile);
}


int NRUCache::openFile(const char *path) {
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


int NRUCache::closeFile(int fd) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    syncFile(fd);
    CloseHandle(it->second.hFile);

    for (auto cit = cache_map.begin(); cit != cache_map.end();) {
        if (cit->first.fd == fd) {
            delete cit->second;
            cit = cache_map.erase(cit);
        } else ++cit;
    }

    open_files.erase(it);
    return 0;
}


ssize_t NRUCache::readFile(int fd, void *buf, size_t count) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    FileHandleInternal &file = it->second;
    off_t start = file.current_pos;
    off_t end = start + count;
    char *dest = (char *) buf;
    ssize_t total = 0;

    for (off_t bn = start / block_size; bn <= (end - 1) / block_size; ++bn) {
        CacheBlock *block = findBlock(fd, bn);
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


ssize_t NRUCache::writeFile(int fd, const void *buf, size_t count) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    FileHandleInternal &file = it->second;
    off_t start = file.current_pos;
    off_t end = start + count;
    const char *src = static_cast<const char *>(buf);
    ssize_t total = 0;

    for (off_t bn = start / block_size; bn <= (end - 1) / block_size; ++bn) {
        CacheBlock *block = findBlock(fd, bn);
        if (!block) block = loadBlock(fd, bn);
        if (!block) return -1;

        off_t block_start = bn * block_size;
        off_t write_start = std::max(start, block_start);
        off_t write_end = std::min(end, static_cast<off_t>(block_start + block_size));
        size_t offset = write_start - block_start;
        size_t bytes = write_end - write_start;

        memcpy(block->data.data() + offset, src + total, bytes);
        total += bytes;
        block->dirty = true;
    }

    file.current_pos += total;
    return total;
}

off_t NRUCache::seekFile(int fd, off_t offset, int whence) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    LARGE_INTEGER size;
    switch (whence) {
        case SEEK_SET: it->second.current_pos = offset;
            break;
        case SEEK_CUR: it->second.current_pos += offset;
            break;
        case SEEK_END:
            GetFileSizeEx(it->second.hFile, &size);
            it->second.current_pos = size.QuadPart + offset;
            break;
        default: return -1;
    }
    return it->second.current_pos;
}

int NRUCache::syncFile(int fd) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    for (auto &pair: cache_map) {
        CacheBlock *block = pair.second;
        if (block->fd == fd && block->dirty)
            writeBackBlock(block);
    }

    FlushFileBuffers(it->second.hFile);
    return 0;
}
