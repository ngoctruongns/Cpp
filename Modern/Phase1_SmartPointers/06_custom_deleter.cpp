/**
 * PHASE 1 - Bài 06: Custom Deleter
 *
 * Mục tiêu:
 *  - Dùng unique_ptr/shared_ptr với custom deleter cho non-heap resources
 *  - Wrap FILE*, socket fd, hardware handle
 *  - Dùng function object, lambda, function pointer làm deleter
 *
 * Compile: g++ -std=c++17 06_custom_deleter.cpp -o out
 */

#include <iostream>
#include <memory>
#include <functional>
#include <cstdio>
#include <string>
#include <stdexcept>

// ─── Ví dụ 1: Wrap FILE* bằng unique_ptr + custom deleter ────────────────

// Custom deleter là functor
struct FileDeleter {
    void operator()(FILE* fp) const {
        if (fp) {
            std::cout << "[FileDeleter] fclose()\n";
            std::fclose(fp);
        }
    }
};

void demo_file_unique_ptr() {
    std::cout << "\n=== unique_ptr<FILE*> with Custom Deleter ===\n";

    // unique_ptr<FILE, FileDeleter> tự động gọi fclose khi ra scope
    std::unique_ptr<FILE, FileDeleter> fp(std::fopen("/tmp/test_deleter.txt", "w"));

    if (!fp) {
        std::cerr << "Failed to open file\n";
        return;
    }

    std::fprintf(fp.get(), "Hello from custom deleter!\n");
    std::cout << "Written to file\n";
    // fp ra scope → FileDeleter::operator() → fclose() tự động
}

// ─── Ví dụ 2: Lambda deleter với shared_ptr ──────────────────────────────
void demo_lambda_deleter() {
    std::cout << "\n=== shared_ptr with Lambda Deleter ===\n";

    // Lambda làm deleter — tiện hơn functor
    auto file_open = [](const char* path) -> FILE* {
        FILE* f = std::fopen(path, "r");
        if (f) std::cout << "[+] Opened: " << path << "\n";
        return f;
    };

    auto file_close = [](FILE* f) {
        if (f) {
            std::cout << "[-] fclose called\n";
            std::fclose(f);
        }
    };

    // shared_ptr<FILE> — dùng khi cần chia sẻ file handle
    std::shared_ptr<FILE> fp(file_open("/tmp/test_deleter.txt"), file_close);

    if (fp) {
        char buf[64] = {};
        if (std::fgets(buf, sizeof(buf), fp.get())) {
            std::cout << "Read: " << buf;
        }
    }
    // fp ra scope → lambda deleter → fclose
}

// ─── Ví dụ 3: Giả lập hardware handle (sensor/device fd) ─────────────────
struct HardwareHandle {
    int fd;
    std::string device_name;
};

// Mô phỏng open/close hardware (thay vì dùng real ioctl)
HardwareHandle* hw_open(const std::string& dev) {
    std::cout << "[HW] Opening device: " << dev << "\n";
    auto* h = new HardwareHandle{42, dev};
    return h;
}

void hw_close(HardwareHandle* h) {
    if (h) {
        std::cout << "[HW] Closing device: " << h->device_name
                  << " (fd=" << h->fd << ")\n";
        delete h;
    }
}

void demo_hardware_handle() {
    std::cout << "\n=== Hardware Handle with Custom Deleter ===\n";

    // Wrap hardware handle với unique_ptr + function pointer deleter
    using HWPtr = std::unique_ptr<HardwareHandle, decltype(&hw_close)>;
    HWPtr sensor(hw_open("/dev/lidar0"), &hw_close);

    if (sensor) {
        std::cout << "[Main] Sensor fd=" << sensor->fd << "\n";
    }

    // sensor ra scope → hw_close() tự động
}

// ─── Ví dụ 4: make_shared KHÔNG hỗ trợ custom deleter ────────────────────
// unique_ptr: type encode deleter → zero overhead
// shared_ptr: deleter stored in control block → type erasure → flexible
void demo_shared_vs_unique_deleter() {
    std::cout << "\n=== Shared vs Unique Deleter Type ===\n";

    // unique_ptr: deleter là PHẦN CỦA TYPE
    auto lam1 = [](int* p) { std::cout << "unique deleter\n"; delete p; };
    std::unique_ptr<int, decltype(lam1)> up(new int(1), lam1);

    // shared_ptr: deleter được TYPE-ERASED trong control block
    // → có thể assign shared_ptr với deleter khác nhau vào cùng biến
    std::shared_ptr<int> sp1(new int(10), [](int* p) {
        std::cout << "shared lambda deleter A\n"; delete p;
    });
    std::shared_ptr<int> sp2(new int(20), [](int* p) {
        std::cout << "shared lambda deleter B\n"; delete p;
    });

    // sp2 = sp1 OK vì type là shared_ptr<int> (deleter không phải phần type)
    sp2 = sp1;
    std::cout << "sp2 now shares sp1's object (use_count=" << sp1.use_count() << ")\n";
}

// ─── Ví dụ 5: RAII wrapper cho socket (giả lập) ──────────────────────────
class SocketHandle {
    int fd_;
public:
    explicit SocketHandle(int fd) : fd_(fd) {
        std::cout << "[Socket] Opened fd=" << fd_ << "\n";
    }
    ~SocketHandle() {
        std::cout << "[Socket] Closed fd=" << fd_ << "\n";
        // close(fd_); // thực tế
    }
    int get() const { return fd_; }
};

using SocketPtr = std::unique_ptr<SocketHandle>;

SocketPtr make_socket(int port) {
    // Trong thực tế: gọi socket() system call
    std::cout << "[Socket] Connecting to port " << port << "\n";
    return std::make_unique<SocketHandle>(port + 1000);
}

void demo_socket_raii() {
    std::cout << "\n=== Socket RAII with unique_ptr ===\n";
    {
        auto sock = make_socket(8080);
        std::cout << "Using socket fd=" << sock->get() << "\n";
        // sock ra scope → destructor → "close"
    }
    std::cout << "Socket automatically closed\n";
}

int main() {
    demo_file_unique_ptr();
    demo_lambda_deleter();
    demo_hardware_handle();
    demo_shared_vs_unique_deleter();
    demo_socket_raii();
    return 0;
}
