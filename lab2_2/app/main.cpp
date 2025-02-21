#include <cstdio>
#include <cstdlib>
#include <string>
#include <windows.h>
#include "file_operations.h"

#define FILE_SIZE (1 << 26) // 64 MB
#define BLOCK_SIZE 4096
#define NUM_BLOCKS (FILE_SIZE / BLOCK_SIZE)
#define HOT_AREA_SIZE 1024
#define ITER_COUNT 3000

// Helper function to get current time in nanoseconds
long long get_time_ns() {
    LARGE_INTEGER frequency, time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&time);
    return (long long)(time.QuadPart * 1e9 / frequency.QuadPart);
}

// Convert nanoseconds to milliseconds
double ns_to_ms(long long ns) {
    return ns / 1e6;
}

// Generate random block number within a range
size_t random_block(size_t start, size_t end) {
    return start + rand() % (end - start);
}

// Print a separator line for better readability
void print_separator() {
    printf("--------------------------------------------------\n");
}

// Print a test header
void print_test_header(const char *test_name) {
    printf("\n=== %s ===\n", test_name);
}

// Print test results in a readable format
void print_test_result(const char *test_name, long long duration_ns) {
    printf("%s: %.2f ms\n", test_name, ns_to_ms(duration_ns));
}

// RandomRead_Cached
void test_random_read_cached(const char *path) {
    int fd = lab2_open(path);
    if (fd < 0) {
        perror("lab2_open");
        return;
    }

    char buf[BLOCK_SIZE];
    long long start = get_time_ns();

    for (int i = 0; i < ITER_COUNT; i++) {
        size_t block = random_block(0, NUM_BLOCKS);
        off_t offset = block * BLOCK_SIZE;
        lab2_lseek(fd, offset, SEEK_SET);
        lab2_read(fd, buf, BLOCK_SIZE);
    }

    long long end = get_time_ns();
    print_test_result("RandomRead_Cached  ", end - start);

    lab2_close(fd);
}

// RandomRead_Uncached
void test_random_read_uncached(const char *path) {
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        perror("CreateFileA");
        return;
    }

    char buf[BLOCK_SIZE];
    long long start = get_time_ns();

    for (int i = 0; i < ITER_COUNT; i++) {
        size_t block = random_block(0, NUM_BLOCKS);
        off_t offset = block * BLOCK_SIZE;
        DWORD bytesRead;
        SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
        ReadFile(hFile, buf, BLOCK_SIZE, &bytesRead, NULL);
    }

    long long end = get_time_ns();
    print_test_result("RandomRead_Uncached", end - start);

    CloseHandle(hFile);
}

// MixedWorkload_Cached
void test_mixed_workload_cached(const char *path) {
    int fd = lab2_open(path);
    if (fd < 0) {
        perror("lab2_open");
        return;
    }

    char buf[BLOCK_SIZE];
    long long start = get_time_ns();

    for (int i = 0; i < ITER_COUNT; i++) {
        size_t block = random_block(0, NUM_BLOCKS);
        off_t offset = block * BLOCK_SIZE;
        lab2_lseek(fd, offset, SEEK_SET);

        if (i % 10 < 7) { // 70% reads
            lab2_read(fd, buf, BLOCK_SIZE);
        } else { // 30% writes
            lab2_write(fd, buf, BLOCK_SIZE);
        }
    }

    long long end = get_time_ns();
    print_test_result("MixedWorkload_Cached  ", end - start);

    lab2_close(fd);
}

// MixedWorkload_Uncached
void test_mixed_workload_uncached(const char *path) {
    HANDLE hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        perror("CreateFileA");
        return;
    }

    char buf[BLOCK_SIZE];
    long long start = get_time_ns();

    for (int i = 0; i < ITER_COUNT; i++) {
        size_t block = random_block(0, NUM_BLOCKS);
        off_t offset = block * BLOCK_SIZE;
        DWORD bytesRead, bytesWritten;

        if (i % 10 < 7) { // 70% reads
            SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
            ReadFile(hFile, buf, BLOCK_SIZE, &bytesRead, NULL);
        } else { // 30% writes
            SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
            WriteFile(hFile, buf, BLOCK_SIZE, &bytesWritten, NULL);
            FlushFileBuffers(hFile);
        }
    }

    long long end = get_time_ns();
    print_test_result("MixedWorkload_Uncached", end - start);

    CloseHandle(hFile);
}

// TightAreaRandomRead_Cached
void test_tight_area_random_read_cached(const char *path) {
    int fd = lab2_open(path);
    if (fd < 0) {
        perror("lab2_open");
        return;
    }

    char buf[BLOCK_SIZE];
    long long start = get_time_ns();

    for (int i = 0; i < ITER_COUNT; i++) {
        size_t block = random_block(0, HOT_AREA_SIZE);
        off_t offset = block * BLOCK_SIZE;
        lab2_lseek(fd, offset, SEEK_SET);
        lab2_read(fd, buf, BLOCK_SIZE);
    }

    long long end = get_time_ns();
    print_test_result("TightAreaRandomRead_Cached  ", end - start);

    lab2_close(fd);
}

// TightAreaRandomRead_Uncached
void test_tight_area_random_read_uncached(const char *path) {
    HANDLE hFile = CreateFileA(path,
        GENERIC_READ, FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        perror("CreateFileA");
        return;
    }

    char buf[BLOCK_SIZE];
    long long start = get_time_ns();

    for (int i = 0; i < ITER_COUNT; i++) {
        size_t block = random_block(0, HOT_AREA_SIZE);
        off_t offset = block * BLOCK_SIZE;
        DWORD bytesRead;
        SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
        ReadFile(hFile, buf, BLOCK_SIZE, &bytesRead, NULL);
    }

    long long end = get_time_ns();
    print_test_result("TightAreaRandomRead_Uncached", end - start);

    CloseHandle(hFile);
}

// SequentialRead_Cached
void test_sequential_read_cached(const char *path) {
    int fd = lab2_open(path);
    if (fd < 0) {
        perror("lab2_open");
        return;
    }

    char buf[BLOCK_SIZE];
    long long start = get_time_ns();

    for (size_t block = 0; block < NUM_BLOCKS; block++) {
        off_t offset = block * BLOCK_SIZE;
        lab2_lseek(fd, offset, SEEK_SET);
        lab2_read(fd, buf, BLOCK_SIZE);
    }

    long long end = get_time_ns();
    print_test_result("SequentialRead_Cached  ", end - start);

    lab2_close(fd);
}

// SequentialRead_Uncached
void test_sequential_read_uncached(const char *path) {
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        perror("CreateFileA");
        return;
    }

    char buf[BLOCK_SIZE];
    long long start = get_time_ns();

    for (size_t block = 0; block < NUM_BLOCKS; block++) {
        off_t offset = block * BLOCK_SIZE;
        DWORD bytesRead;
        SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
        ReadFile(hFile, buf, BLOCK_SIZE, &bytesRead, NULL);
    }

    long long end = get_time_ns();
    print_test_result("SequentialRead_Uncached", end - start);

    CloseHandle(hFile);
}

int main() {
    const char *path = "testfile.bin";

    // Create a large file for testing
    HANDLE hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        perror("CreateFileA");
        return 1;
    }

    LARGE_INTEGER fileSize;
    fileSize.QuadPart = FILE_SIZE;
    if (!SetFilePointerEx(hFile, fileSize, NULL, FILE_BEGIN)) {
        perror("SetFilePointerEx");
        return 1;
    }
    if (!SetEndOfFile(hFile)) {
        perror("SetEndOfFile");
        return 1;
    }
    CloseHandle(hFile);

    // Run tests
    print_separator();
    print_test_header("Random Read Tests");
    test_random_read_cached(path);
    test_random_read_uncached(path);

    print_separator();
    print_test_header("Mixed Workload Tests");
    test_mixed_workload_cached(path);
    test_mixed_workload_uncached(path);

    print_separator();
    print_test_header("Tight Area Random Read Tests");
    test_tight_area_random_read_cached(path);
    test_tight_area_random_read_uncached(path);

    print_separator();
    print_test_header("Sequential Read Tests");
    test_sequential_read_cached(path);
    test_sequential_read_uncached(path);

    print_separator();
    return 0;
}
