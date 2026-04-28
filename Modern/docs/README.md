# Modern C++ — Tài liệu tham khảo

Tài liệu được tổ chức theo 4 Phase học tập. Mỗi file tương ứng với một nhóm bài tập.

---

## Phương pháp học hiệu quả

### Quy trình cho mỗi bài tập
1. Đọc section tương ứng trong file doc (~5 phút)
2. Đọc lướt comment trong file `.cpp` để nắm ý đồ
3. **Xóa phần implementation, tự viết lại từ đầu** — đây là bước quan trọng nhất
4. So sánh với bản gốc, chú ý chỗ khác biệt

### Thứ tự ưu tiên (theo ứng dụng thực tế với robot)
```
Tuần 1-2:  Phase1 (01-05)  →  Smart ptr & move       [dùng ngay trong driver code]
Tuần 3:    Phase3 (01-04)  →  Thread & mutex          [UART reader thread]
Tuần 4:    Phase4 (01, 09) →  RAII + RealtimeBuffer   [resource guard + control loop]
Tuần 5-6:  Phase2 (01-05)  →  Templates               [generic message types]
Tuần 7-8:  Phase3 (05-10)  →  atomic, thread pool     [sensor fusion]
Tuần 9-10: Phase2 (06-10)  →  CRTP, Policy            [advanced generic code]
Tuần 11-12: Phase4 (02-08) →  Full ROS2 patterns
```

### Áp dụng ngay vào project robot
| Concept vừa học | Áp dụng vào đâu |
|-----------------|-----------------|
| `unique_ptr` | Serial port handle trong `linux_comm_driver` |
| `shared_ptr` + `enable_shared_from_this` | ROS2 Node |
| `thread` + `condition_variable` | UART reader thread |
| `atomic<bool>` | `running_` flag trong vòng lặp |
| `RealtimeBuffer` | Buffer cmd_vel giữa ROS2 callback và control loop |
| RAII ScopeGuard | Mở/đóng serial port |
| Observer / EventBus | Callback khi nhận packet từ STM32 |
| Active Object | Mỗi subsystem (motor, sensor) chạy thread riêng |

### Quy tắc tối ưu thời gian
- **1 concept/ngày** nếu bận: 30 phút đọc + 1 bài tập là đủ
- **Hiểu sâu 1 bài > lướt qua 5 bài**
- Sau 1 tuần, quay lại bài đầu của Phase và tự hỏi: *"Tôi có tự viết được không?"*
- Bật warning đầy đủ khi compile: `g++ -std=c++17 -Wall -Wextra -Wpedantic`

### Những bài bắt buộc hiểu sâu (nền tảng của mọi thứ)
- Phase1/01: move semantics
- Phase1/03: unique_ptr
- Phase3/03: mutex & lock_guard
- Phase3/04: condition_variable
- Phase4/09: realtime buffer

---

## Phase 1 — Smart Pointers & Move Semantics
| File | Nội dung | Bài tập |
|------|----------|---------|
| [01_move_semantics.md](01_move_semantics.md) | lvalue/rvalue, std::move, Rule of Five | 01, 02 |
| [02_smart_pointers.md](02_smart_pointers.md) | unique_ptr, shared_ptr, weak_ptr | 03, 04, 05 |
| [03_advanced_smart_ptr.md](03_advanced_smart_ptr.md) | Custom deleter, enable_shared_from_this, perfect forwarding | 06, 07, 08, 09, 10 |

## Phase 2 — Templates
| File | Nội dung | Bài tập |
|------|----------|---------|
| [04_variadic_templates.md](04_variadic_templates.md) | Variadic templates, fold expressions | 01, 02 |
| [05_template_constraints.md](05_template_constraints.md) | Specialization, SFINAE, enable_if, if constexpr, type traits, Concepts (C++20) | 03, 04, 05, 06, 07 |
| [06_crtp_policy.md](06_crtp_policy.md) | CRTP, Policy-based design | 08, 09, 10 |

## Phase 3 — Concurrency
| File | Nội dung | Bài tập |
|------|----------|---------|
| [07_thread_mutex.md](07_thread_mutex.md) | std::thread, mutex, lock_guard, condition_variable | 01, 02, 03, 04 |
| [08_concurrency_tools.md](08_concurrency_tools.md) | atomic, thread pool, jthread/stop_token, producer-consumer, deadlock | 05, 06, 07, 08, 09, 10 |

## Phase 4 — ROS2 Design Patterns
| File | Nội dung | Bài tập |
|------|----------|---------|
| [09_raii_patterns.md](09_raii_patterns.md) | RAII, ScopeGuard, Observer, State Machine, Command | 01, 02, 03, 04 |
| [10_ros2_patterns.md](10_ros2_patterns.md) | Plugin, Param Server, Lifecycle Node, Realtime Buffer | 05, 06, 07, 08, 09, 10 |

## Design Patterns bổ sung & Lộ trình học rộng hơn
| File | Nội dung |
|------|----------|
| [11_extra_patterns.md](11_extra_patterns.md) | Active Object, Pipeline, Strategy, Object Pool, EventBus, Template Method |
| [12_syseng_roadmap.md](12_syseng_roadmap.md) | Roadmap Robotics System Engineer: ROS2 deep dive, RT Linux, Protocols, Build, Test, Architecture |

## Yêu cầu biên dịch
```
g++ -std=c++17 -pthread <file>.cpp -o out
# C++20 concepts: g++ -std=c++20 ...
# Full warnings (khuyến nghị):
g++ -std=c++17 -pthread -Wall -Wextra -Wpedantic <file>.cpp -o out
```
