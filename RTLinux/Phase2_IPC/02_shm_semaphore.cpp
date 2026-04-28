/**
 * PHASE 2 - Bài 02: Shared Memory + Semaphore + Process-shared Mutex
 *
 * Mục tiêu:
 *  - Dùng named semaphore (sem_open) để đồng bộ giữa processes
 *  - Dùng process-shared mutex (PTHREAD_PROCESS_SHARED) trong shared memory
 *  - Dùng process-shared condvar cho producer-consumer pattern
 *
 * Compile: g++ -std=c++17 -O2 02_shm_semaphore.cpp -o out -lrt -lpthread
 */

#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <sys/wait.h>

// ─── Ví dụ 1: Named Semaphore ─────────────────────────────────────────────
void example_named_semaphore() {
    std::cout << "\n=== Ví dụ 1: Named Semaphore (sem_open) ===\n";

    constexpr const char* SEM_NAME = "/robot_sem_01";

    // sem_open: tạo semaphore với initial value = 0
    sem_t* sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED) { perror("sem_open"); return; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // ─── Child: đợi semaphore rồi làm việc
        sem_t* child_sem = sem_open(SEM_NAME, 0);  // mở semaphore đã có
        printf("[Child]  Waiting for semaphore...\n");
        sem_wait(child_sem);  // block đến khi parent post
        printf("[Child]  Semaphore received! Proceeding.\n");
        sem_close(child_sem);
        _exit(0);

    } else {
        // ─── Parent: làm việc rồi signal
        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 200'000'000 };
        nanosleep(&sleep, nullptr);  // Simulate work
        printf("[Parent] Work done, posting semaphore...\n");
        sem_post(sem);  // signal child

        wait(nullptr);
        sem_close(sem);
        sem_unlink(SEM_NAME);
        std::cout << "Named semaphore example done\n";
    }
}

// ─── Shared region với process-shared mutex + condvar ─────────────────────
struct SharedRegion {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int             data_ready;
    double          value;
};

SharedRegion* create_shared_region(const char* name) {
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SharedRegion));
    auto* r = static_cast<SharedRegion*>(
        mmap(nullptr, sizeof(SharedRegion),
             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    return r;
}

void init_process_shared_mutex(SharedRegion* r) {
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
    pthread_mutex_init(&r->mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);

    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&r->cond, &cattr);
    pthread_condattr_destroy(&cattr);

    r->data_ready = 0;
    r->value      = 0.0;
}

// ─── Ví dụ 2: Process-shared mutex + condvar ──────────────────────────────
void example_process_shared_mutex() {
    std::cout << "\n=== Ví dụ 2: Process-shared mutex + condvar ===\n";

    constexpr const char* NAME = "/robot_shm_mutex";
    SharedRegion* r = create_shared_region(NAME);
    init_process_shared_mutex(r);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // ─── Child: Consumer — đợi data rồi đọc
        SharedRegion* child_r = create_shared_region(NAME);

        pthread_mutex_lock(&child_r->mutex);
        while (!child_r->data_ready)
            pthread_cond_wait(&child_r->cond, &child_r->mutex);

        printf("[Consumer] Received value=%.3f\n", child_r->value);
        pthread_mutex_unlock(&child_r->mutex);

        munmap(child_r, sizeof(SharedRegion));
        _exit(0);

    } else {
        // ─── Parent: Producer — ghi data rồi signal
        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 100'000'000 };
        nanosleep(&sleep, nullptr);

        pthread_mutex_lock(&r->mutex);
        r->value      = 3.14159;
        r->data_ready = 1;
        pthread_cond_signal(&r->cond);
        pthread_mutex_unlock(&r->mutex);
        printf("[Producer] Sent value=3.14159\n");

        wait(nullptr);

        pthread_mutex_destroy(&r->mutex);
        pthread_cond_destroy(&r->cond);
        munmap(r, sizeof(SharedRegion));
        shm_unlink(NAME);
        std::cout << "Process-shared mutex example done\n";
    }
}

// ─── Ví dụ 3: Anonymous semaphore trong shared memory ─────────────────────
struct SemShm {
    sem_t  ready_sem;   // KHÔNG phải named — process-shared sem
    double payload;
    int    count;
};

void example_anonymous_process_sem() {
    std::cout << "\n=== Ví dụ 3: Anonymous process-shared semaphore ===\n";

    constexpr const char* NAME = "/sem_anon_demo";
    int fd = shm_open(NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SemShm));
    auto* shm = static_cast<SemShm*>(
        mmap(nullptr, sizeof(SemShm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    // sem_init với pshared=1 → visible to child processes
    sem_init(&shm->ready_sem, 1 /*pshared*/, 0);
    shm->count = 0;

    pid_t pid = fork();
    if (pid == 0) {
        SemShm* c = static_cast<SemShm*>(
            mmap(nullptr, sizeof(SemShm), PROT_READ | PROT_WRITE, MAP_SHARED,
                 shm_open(NAME, O_RDWR, 0), 0));

        for (int i = 0; i < 3; ++i) {
            sem_wait(&c->ready_sem);
            printf("[Child] Received payload=%.1f (seq=%d)\n", c->payload, c->count);
        }
        munmap(c, sizeof(SemShm));
        _exit(0);
    }

    for (int i = 0; i < 3; ++i) {
        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 100'000'000 };
        nanosleep(&sleep, nullptr);

        shm->payload = i * 1.5;
        shm->count   = i + 1;
        sem_post(&shm->ready_sem);
        printf("[Parent] Sent payload=%.1f\n", shm->payload);
    }

    wait(nullptr);
    sem_destroy(&shm->ready_sem);
    munmap(shm, sizeof(SemShm));
    shm_unlink(NAME);
    std::cout << "Anonymous semaphore example done\n";
}

int main() {
    std::cout << "=== Phase2 Bài 02: Shared Memory + Semaphore ===\n";

    example_named_semaphore();
    example_process_shared_mutex();
    example_anonymous_process_sem();

    return 0;
}
