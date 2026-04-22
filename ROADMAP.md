# Modern C++ Roadmap → ROS2 & Robotics

> **Mục tiêu**: Nắm vững C++14/17/20 để làm chủ ROS2 và lập trình robotics.
> **Thời gian dự kiến**: 10–14 tuần

---

## Đánh giá hiện tại (Current State)

| Kỹ năng | Mức độ | Ghi chú |
|---|---|---|
| OOP (Inheritance, Polymorphism) | ✅ Tốt | Class/, inherith/, abstract_class/ |
| STL Containers & Algorithms | ✅ Tốt | STL/container/, STL/Algorithm/ |
| DSA (Linked list, Tree, Graph) | ✅ Tốt | DSA/ |
| `std::async`, `std::future`, `std::promise` | 🟡 Cơ bản | STL/Future/, STL/Async/ |
| Smart Pointers | 🟠 Dùng Boost, chưa dùng std:: | STL/Memory/ |
| Template Programming | 🟠 Cơ bản | Class/template/ |
| Concurrency (thread, mutex) | 🔴 Chưa có | Cần bổ sung |
| Move Semantics / Rvalue | 🔴 Chưa có | Cần bổ sung |
| Lambda nâng cao | 🟡 Cơ bản | STL/Functional/Lambda/ |
| Design Patterns | 🟡 Mới bắt đầu | Pattern/Factory/ |

---

## Lộ trình học (Learning Phases)

```
Phase 1 (2–3 tuần)       Phase 2 (3–4 tuần)       Phase 3 (3–4 tuần)       Phase 4 (2 tuần)
─────────────────────    ─────────────────────    ─────────────────────    ─────────────────
Move Semantics &         Template                 Concurrency &            ROS2-Ready
Smart Pointers           Programming              Multithreading           Patterns
─────────────────────    ─────────────────────    ─────────────────────    ─────────────────
• Rvalue references      • Variadic templates     • std::thread            • RAII in ROS2
• std::move/forward      • Template specialization• mutex / lock_guard     • Callback patterns
• unique_ptr             • SFINAE                 • condition_variable     • Executor model
• shared_ptr (std)       • if constexpr           • atomic<T>              • Publisher/Sub sim
• weak_ptr               • Concepts (C++20)       • std::jthread (C++20)   • Parameter server
• Custom deleters        • Policy-based design    • Thread pool            • Service/Action sim
• enable_shared_from_this• Type traits            • Memory ordering        • Component nodes
```

---

## PHASE 1: Move Semantics & Smart Pointers
**Thư mục**: `Modern/Phase1_SmartPointers/`

### Lý thuyết cần nắm
1. **Lvalue vs Rvalue** — Biểu thức nào là lvalue, rvalue, xvalue?
2. **Rvalue Reference (`T&&`)** — Dùng để bind rvalue, tránh copy
3. **`std::move`** — Cast lvalue → rvalue, không thực sự di chuyển
4. **`std::forward`** — Perfect forwarding trong template
5. **Rule of 5** — Destructor, copy ctor, copy assign, move ctor, move assign
6. **std::unique_ptr** — Ownership độc quyền, không copy được
7. **std::shared_ptr** — Reference counting, chia sẻ ownership
8. **std::weak_ptr** — Observer, tránh circular reference
9. **Custom Deleter** — Dùng cho resources không phải heap (file, socket)
10. **enable_shared_from_this** — Lấy shared_ptr từ `this`

### Bài tập (theo thứ tự)
```
01_rvalue_move_basics.cpp       — Rvalue ref, std::move, copy vs move cost
02_rule_of_five.cpp             — Implement đủ 5 special member functions
03_unique_ptr_basics.cpp        — unique_ptr, make_unique, move ownership
04_shared_ptr_refcount.cpp      — shared_ptr, use_count, weak_ptr
05_circular_ref_fix.cpp         — Bug: circular ref → fix bằng weak_ptr
06_custom_deleter.cpp           — Deleter cho FILE*, socket fd
07_enable_shared_from_this.cpp  — Pattern dùng trong ROS2 Node
08_smart_ptr_containers.cpp     — vector<unique_ptr<T>>, polymorphism
09_perfect_forwarding.cpp       — std::forward, forwarding reference
10_robot_resource_manager.cpp   — Bài tổng hợp: sensor + actuator manager
```

### Liên kết ROS2
- `rclcpp::Node` kế thừa `enable_shared_from_this`
- Publisher/Subscriber dùng `shared_ptr`
- Plugins dùng `unique_ptr`
- `rclcpp::make_node<T>()` → `std::make_shared<T>()`

---

## PHASE 2: Template Programming
**Thư mục**: `Modern/Phase2_Templates/`

### Lý thuyết cần nắm
1. **Function/Class Templates** — Ôn tập + nâng cao
2. **Variadic Templates** — Parameter pack, `sizeof...`, fold expressions
3. **Template Specialization** — Full & partial specialization
4. **SFINAE** — Substitution Failure Is Not An Error
5. **`if constexpr`** — Compile-time branching (C++17)
6. **Type Traits** — `std::is_same`, `std::enable_if`, `std::conditional`
7. **Concepts** — Compile-time constraints (C++20)
8. **Policy-Based Design** — Template parameters as behavior policies
9. **CRTP** — Curiously Recurring Template Pattern

### Bài tập (theo thứ tự)
```
01_variadic_templates.cpp       — Implement printf-like, tuple-like
02_fold_expressions.cpp         — C++17 fold, sum/product/print pack
03_template_specialization.cpp  — Specialization cho int, float, pointer
04_sfinae_enable_if.cpp         — enable_if, is_integral, type dispatch
05_if_constexpr.cpp             — C++17 if constexpr, compile-time branch
06_type_traits_custom.cpp       — Tự viết is_pointer, remove_const
07_concepts_c20.cpp             — C++20 concept, requires clause
08_crtp_pattern.cpp             — CRTP cho static polymorphism
09_policy_based_logger.cpp      — Logger với Output/Format policy
10_generic_robot_msg.cpp        — Generic message serializer cho ROS2
```

### Liên kết ROS2
- `rclcpp::Publisher<T>` là class template
- Message type hết sức đa dạng → cần template
- `rclcpp::TypeAdapter` dùng template specialization
- Lifecycle nodes dùng CRTP pattern

---

## PHASE 3: Concurrency & Multithreading
**Thư mục**: `Modern/Phase3_Concurrency/`

### Lý thuyết cần nắm
1. **`std::thread`** — Tạo, join, detach
2. **Race Condition** — Nhận biết và tránh
3. **`std::mutex` + `std::lock_guard`** — RAII locking
4. **`std::unique_lock`** — Flexible locking
5. **`std::condition_variable`** — Thread synchronization
6. **`std::atomic<T>`** — Lock-free operations
7. **Memory Ordering** — relaxed, acquire/release, seq_cst
8. **Thread Pool** — Tái sử dụng thread, task queue
9. **`std::jthread` (C++20)** — Auto-joining, stop token
10. **`std::async` nâng cao** — Launch policy, exception handling

### Bài tập (theo thứ tự)
```
01_thread_basics.cpp            — Tạo thread, join, detach, lambda thread
02_race_condition_demo.cpp      — Minh họa race condition + fix bằng mutex
03_mutex_and_lockguard.cpp      — lock_guard, scoped_lock (C++17)
04_unique_lock_condition.cpp    — unique_lock + condition_variable
05_producer_consumer.cpp        — Classic producer-consumer problem
06_atomic_operations.cpp        — atomic counter, CAS, memory_order
07_thread_pool.cpp              — Thread pool với task queue
08_jthread_stop_token.cpp       — C++20 jthread, stop_source/stop_token
09_deadlock_prevention.cpp      — Deadlock scenarios + std::lock() fix
10_sensor_fusion_sim.cpp        — Bài tổng hợp: multi-sensor data fusion
```

### Liên kết ROS2
- ROS2 Executor chạy callback trong thread pool
- `MultiThreadedExecutor` → cần hiểu thread safety
- `ReentrantCallbackGroup` vs `MutuallyExclusiveCallbackGroup`
- Timer callbacks, topic callbacks → concurrent execution
- `rclcpp::spin_some()`, `spin_until_future_complete()`

---

## PHASE 4: ROS2-Ready Patterns
**Thư mục**: `Modern/Phase4_ROS2Patterns/`

### Bài tập (theo thứ tự)
```
01_raii_resource_guard.cpp      — RAII cho hardware resources
02_observer_pattern.cpp         — Event system ~ topic pub/sub
03_state_machine.cpp            — Robot state machine
04_command_pattern.cpp          — Action commands ~ ROS2 Action
05_plugin_system.cpp            — Dynamic loading ~ ROS2 plugin
06_param_server_sim.cpp         — Key-value store ~ ROS2 params
07_lifecycle_node_sim.cpp       — Lifecycle state machine simulation
08_component_node_sim.cpp       — Node composition simulation
09_realtime_buffer.cpp          — Lock-free ring buffer cho real-time
10_mini_ros_framework.cpp       — Bài tổng hợp: mini pub/sub framework
```

---

## Checklist trước khi học ROS2 thực sự

- [ ] Hiểu `shared_ptr` và tại sao `rclcpp::Node` dùng `enable_shared_from_this`
- [ ] Implement được Thread Pool từ đầu
- [ ] Hiểu `std::condition_variable` và Producer-Consumer
- [ ] Biết dùng `concepts` để constrain template parameters
- [ ] Implement được Observer Pattern thread-safe
- [ ] Hiểu Memory Ordering (`memory_order_acquire/release`)
- [ ] Biết CRTP và tại sao dùng nó thay virtual function
- [ ] Implement được generic message serializer

---

## Tài nguyên tham khảo

| Chủ đề | Nguồn |
|---|---|
| Modern C++ | *Effective Modern C++* - Scott Meyers |
| Concurrency | *C++ Concurrency in Action* - Anthony Williams |
| Template | *C++ Templates: The Complete Guide* - Vandevoorde & Josuttis |
| ROS2 C++ | https://docs.ros.org/en/humble/Tutorials/Beginner-Client-Libraries/ |
| cppreference | https://en.cppreference.com |

---

## Cấu trúc thư mục bài tập

```
Modern/
├── Phase1_SmartPointers/
│   ├── 01_rvalue_move_basics.cpp
│   ├── ...
│   └── 10_robot_resource_manager.cpp
├── Phase2_Templates/
│   ├── 01_variadic_templates.cpp
│   ├── ...
│   └── 10_generic_robot_msg.cpp
├── Phase3_Concurrency/
│   ├── 01_thread_basics.cpp
│   ├── ...
│   └── 10_sensor_fusion_sim.cpp
└── Phase4_ROS2Patterns/
    ├── 01_raii_resource_guard.cpp
    ├── ...
    └── 10_mini_ros_framework.cpp
```
