/**
 * PHASE 2 - Bài 01: POSIX Shared Memory
 *
 * Mục tiêu:
 *  - Tạo/mở shared memory với shm_open + ftruncate + mmap
 *  - Chia sẻ struct giữa 2 processes (dùng fork)
 *  - munmap + shm_unlink để cleanup
 *  - Xem shared memory tại /dev/shm/
 *
 * Compile: g++ -std=c++17 -O2 01_shared_memory.cpp -o out -lrt
 */

#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <sys/wait.h>

constexpr const char* SHM_NAME = "/robot_shm_01";

// ─── Ví dụ 1: Tạo và ghi vào shared memory ────────────────────────────────
void example_create_shm() {
    std::cout << "\n=== Ví dụ 1: Tạo shared memory ===\n";

    struct SharedData {
        double velocity;
        int    counter;
        char   message[64];
    };

    // Tạo hoặc mở (O_CREAT | O_RDWR)
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) { perror("shm_open"); return; }

    // Đặt kích thước
    if (ftruncate(fd, sizeof(SharedData)) < 0) {
        perror("ftruncate"); close(fd); return;
    }

    // Map vào address space
    auto* data = static_cast<SharedData*>(
        mmap(nullptr, sizeof(SharedData),
             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);  // fd không cần nữa sau mmap

    if (data == MAP_FAILED) { perror("mmap"); return; }

    // Ghi dữ liệu
    data->velocity = 1.23;
    data->counter  = 42;
    strncpy(data->message, "Hello from writer!", sizeof(data->message) - 1);

    printf("Wrote: velocity=%.2f counter=%d message='%s'\n",
           data->velocity, data->counter, data->message);
    printf("Shared memory at /dev/shm%s\n", SHM_NAME);

    // Cleanup
    munmap(data, sizeof(SharedData));
    shm_unlink(SHM_NAME);
    std::cout << "Unlinked shared memory\n";
}

// ─── Ví dụ 2: Writer + Reader với fork ────────────────────────────────────
struct SensorShm {
    double   velocity_left;
    double   velocity_right;
    uint64_t timestamp_ns;
    int      seq;
};

void example_fork_shm() {
    std::cout << "\n=== Ví dụ 2: Fork — Writer + Reader ===\n";

    constexpr const char* NAME = "/robot_sensor_shm";

    // Tạo shared memory trước khi fork
    int fd = shm_open(NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SensorShm));
    auto* shm = static_cast<SensorShm*>(
        mmap(nullptr, sizeof(SensorShm),
             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    memset(shm, 0, sizeof(SensorShm));

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // ─── Child: Reader ─────────────────────────────────────────────
        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 50'000'000 };
        nanosleep(&sleep, nullptr);  // Chờ parent ghi

        printf("[Reader] velocity_left=%.2f velocity_right=%.2f seq=%d\n",
               shm->velocity_left, shm->velocity_right, shm->seq);

        munmap(shm, sizeof(SensorShm));
        _exit(0);

    } else {
        // ─── Parent: Writer ────────────────────────────────────────────
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        shm->velocity_left  = 1.5;
        shm->velocity_right = 1.4;
        shm->timestamp_ns   = (uint64_t)ts.tv_sec * 1'000'000'000ULL + ts.tv_nsec;
        shm->seq            = 1;
        printf("[Writer] Wrote seq=1\n");

        wait(nullptr);  // Chờ child kết thúc

        munmap(shm, sizeof(SensorShm));
        shm_unlink(NAME);
    }
}

// ─── Ví dụ 3: Kiểm tra /dev/shm ──────────────────────────────────────────
void example_list_dev_shm() {
    std::cout << "\n=== Ví dụ 3: /dev/shm (tmpfs) ===\n";
    std::cout << "Shared memory objects nằm tại /dev/shm/\n";
    std::cout << "Xem bằng: ls -la /dev/shm/\n";
    std::cout << "Xóa thủ công: rm /dev/shm/<name>\n";
    std::cout << "Hay dùng shm_unlink() trong code\n\n";

    // Tạo một SHM và kiểm tra
    constexpr const char* NAME = "/demo_inspect";
    int fd = shm_open(NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, 1024);
    printf("Created /dev/shm%s (1024 bytes)\n", NAME);
    printf("Check: ls -la /dev/shm%s\n", NAME);
    close(fd);

    struct timespec sleep{ .tv_sec = 0, .tv_nsec = 100'000'000 };
    nanosleep(&sleep, nullptr);

    shm_unlink(NAME);
    printf("Unlinked %s\n", NAME);
}

int main() {
    std::cout << "=== Phase2 Bài 01: POSIX Shared Memory ===\n";

    example_create_shm();
    example_fork_shm();
    example_list_dev_shm();

    return 0;
}
