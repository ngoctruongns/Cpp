// Compile: g++ -std=c++17 -pthread thread_test.cpp -o test_thread

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <shared_mutex>

using namespace std::chrono_literals;
using namespace std;

void do_work(int thread_id) {
    std::cout << "Thread " << thread_id << " started (id: " << std::this_thread::get_id() << ")\n";
    std::this_thread::sleep_for(100ms);
    std::cout << "Thread " << thread_id << " finished\n";
}

static int cnt;
std::mutex mtx;
std::timed_mutex tmtx;

void incCounter(int thread_id) {
    tmtx.lock();
    for (int i = 0; i < 10000; ++i) {
        ++cnt;
    }

    // delay to simulate some work
    std::this_thread::sleep_for(200ms);
    tmtx.unlock();
    std::cout << "Thread " << thread_id << " incremented counter\n";
}

void incCounter1(int thread_id) {
    std::lock_guard<std::timed_mutex> lock(tmtx);
    for (int i = 0; i < 10000; ++i) {
        ++cnt;
    }
    std::this_thread::sleep_for(200ms);
    std::cout << "Thread " << thread_id << " incremented counter\n";
}

void incCounter2(int thread_id) {
    if (tmtx.try_lock_for(100ms)) {
        for (int i = 0; i < 10000; ++i) {
            ++cnt;
        }
        tmtx.unlock();
        std::cout << "Thread " << thread_id << " acquired lock and incremented counter\n";
    } else {
        std::cout << "Thread " << thread_id << " --X-- could not acquire lock\n";
    }
}

// atomic: counter an toàn giữa nhiều thread mà không cần mutex
std::atomic<int> atomicCnt{0};

void atomicIncrement(int thread_id) {
    for (int i = 0; i < 10000; ++i) {
        ++atomicCnt;
    }
    std::cout << "Thread " << thread_id << " atomic increment done\n";
}

// condition_variable: thread chờ tín hiệu từ thread khác
std::condition_variable cv;
std::mutex cvMtx;
bool ready = false;

void waitForSignal(int thread_id) {
    std::unique_lock<std::mutex> lock(cvMtx);
    cv.wait(lock, [] { return ready; });
    std::cout << "Thread " << thread_id << " received signal\n";
}

void sendSignal() {
    {
        std::lock_guard<std::mutex> lock(cvMtx);
        ready = true;
    }
    cv.notify_all();
}

// async/future: chạy task bất đồng bộ và nhận kết quả trả về
int longCalculation(int x) {
    std::this_thread::sleep_for(500ms);
    return x * x;
}

// unique_lock: linh hoạt hơn lock_guard, có thể unlock/relock thủ công
void uniqueLockDemo(int thread_id) {
    std::unique_lock<std::timed_mutex> lock(tmtx);
    std::cout << "Thread " << thread_id << " acquired unique_lock\n";
    std::this_thread::sleep_for(100ms);

    lock.unlock();
    std::cout << "Thread " << thread_id << " released unique_lock early\n";
}

// shared_mutex: nhiều reader có thể đọc cùng lúc, writer cần độc quyền
std::shared_mutex sharedMtx;
int sharedData = 0;

void reader(int id) {
    std::shared_lock<std::shared_mutex> lock(sharedMtx);
    std::cout << "Reader " << id << " reads sharedData = " << sharedData << "\n";
    std::this_thread::sleep_for(100ms);
}

void writer(int id) {
    std::unique_lock<std::shared_mutex> lock(sharedMtx);
    ++sharedData;
    std::cout << "Writer " << id << " updates sharedData = " << sharedData << "\n";
    std::this_thread::sleep_for(100ms);
}

// call_once: đảm bảo resource chỉ được init một lần
std::once_flag initFlag;

void initResource() {
    std::cout << "Resource initialized only once\n";
}

void callOnceDemo(int thread_id) {
    std::call_once(initFlag, initResource);
    std::cout << "Thread " << thread_id << " passed call_once\n";
}

// thread_local: mỗi thread có bản copy riêng của biến
thread_local int tlsCounter = 0;

void threadLocalDemo(int thread_id) {
    ++tlsCounter;
    std::cout << "Thread " << thread_id << " tlsCounter = " << tlsCounter << "\n";
}

// scoped_lock: lock nhiều mutex cùng lúc, giúp tránh deadlock
std::mutex m1;
std::mutex m2;

void scopedLockDemo(int thread_id) {
    std::scoped_lock lock(m1, m2);
    std::cout << "Thread " << thread_id << " locked m1 and m2 safely\n";
}

int main() {
    std::cout << "=== Thread Test ===\n";
    std::cout << "Main thread: " << std::this_thread::get_id() << "\n";

    std::thread t1(do_work, 12);
    std::thread t2(incCounter, 34);
    std::thread t3(incCounter1, 56);
    std::this_thread::sleep_for(50ms);
    std::thread t4(incCounter2, 78);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    cout << "Counter: " << cnt << endl;

    std::cout << "\n=== Atomic Demo ===\n";
    std::thread a1(atomicIncrement, 101);
    std::thread a2(atomicIncrement, 102);
    a1.join();
    a2.join();
    std::cout << "Atomic Counter: " << atomicCnt << "\n";

    std::cout << "\n=== Condition Variable Demo ===\n";
    ready = false;
    std::thread w1(waitForSignal, 201);
    std::thread w2(waitForSignal, 202);
    std::this_thread::sleep_for(300ms);
    sendSignal();
    w1.join();
    w2.join();

    std::cout << "\n=== Async/Future Demo ===\n";
    auto futureResult = std::async(std::launch::async, longCalculation, 12);
    std::cout << "Async result: " << futureResult.get() << "\n";

    std::cout << "\n=== Unique Lock Demo ===\n";
    std::thread u1(uniqueLockDemo, 301);
    std::thread u2(uniqueLockDemo, 302);
    u1.join();
    u2.join();

    std::cout << "\n=== Shared Mutex Reader/Writer Demo ===\n";
    std::thread r1(reader, 401);
    std::thread r2(reader, 402);
    std::thread wr1(writer, 403);
    r1.join();
    r2.join();
    wr1.join();

    std::cout << "\n=== call_once Demo ===\n";
    std::thread o1(callOnceDemo, 501);
    std::thread o2(callOnceDemo, 502);
    std::thread o3(callOnceDemo, 503);
    o1.join();
    o2.join();
    o3.join();

    std::cout << "\n=== thread_local Demo ===\n";
    std::thread tls1(threadLocalDemo, 601);
    std::thread tls2(threadLocalDemo, 602);
    tls1.join();
    tls2.join();

    std::cout << "\n=== scoped_lock Demo ===\n";
    std::thread s1(scopedLockDemo, 701);
    std::thread s2(scopedLockDemo, 702);
    s1.join();
    s2.join();

    // Tạo một vector để chứa các thread
    std::cout << "\n=== Vector Thread Demo ===\n";
    std::vector<std::thread> threads;

    // Tạo 5 thread và thêm vào vector
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i]() {
            std::cout << "Thread " << i << " started (id: " << std::this_thread::get_id() << ")\n";
            std::this_thread::sleep_for(100ms);
            std::cout << "Thread " << i << " finished\n";
        });
    }

    // Join tất cả các thread
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "All threads completed.\n";
    return 0;
}