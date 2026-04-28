# RT Linux Concepts

> **Bài tập liên quan:** Phase1 / tất cả

---

## 1. Linux Scheduler — Tổng quan

Linux kernel có nhiều scheduling policy:

```
SCHED_OTHER (CFS)      — mặc định, fair-share, không có priority cứng
SCHED_BATCH            — cho batch processing, low priority
SCHED_IDLE             — cực thấp, chỉ chạy khi idle
─────────────────────────────────────────────
SCHED_FIFO             — RT, first-in-first-out, preemptive by priority
SCHED_RR               — RT, round-robin trong cùng priority
SCHED_DEADLINE         — RT, EDF (Earliest Deadline First)
```

**RT policies (FIFO/RR) luôn preempt CFS** — một RT thread priority 1 vẫn preempt mọi CFS thread.

### Priority range

```
CFS (SCHED_OTHER):  nice value -20..+19  (không phải RT priority)
RT  (SCHED_FIFO/RR): rt_priority 1..99   (99 = cao nhất)
```

```
Priority 99 (RT) ─── preempts ───► Priority 1 (RT) ─── preempts ───► CFS
```

---

## 2. PREEMPT_RT — Tại sao cần?

**Kernel thường (Vanilla):** Nhiều vùng code kernel không thể bị preempt (spinlock, interrupt handler, soft IRQ). Một interrupt handler chạy có thể block RT thread hàng ms.

**PREEMPT_RT:** Gần như toàn bộ kernel code có thể bị preempt. spinlock → sleeping mutex. Hard IRQ → kernel threads. Kết quả: worst-case latency giảm từ ~1ms xuống ~50-100μs.

```
Vanilla kernel:  worst-case latency ~1-10ms   (không dùng được cho servo control)
lowlatency:      worst-case latency ~200-500μs (edge case cho robotics)
PREEMPT_RT:      worst-case latency ~50-150μs  (dùng được cho control loop 1kHz)
```

### Kiểm tra kernel type

```bash
uname -v
# Vanilla:     #1 SMP PREEMPT_DYNAMIC
# Lowlatency:  #1 SMP PREEMPT
# PREEMPT_RT:  #1 SMP PREEMPT_RT
```

---

## 3. Nguyên nhân latency

Các nguồn gây latency trong RT system:

| Nguồn | Nguyên nhân | Giải pháp |
|-------|------------|-----------|
| Page fault | Process truy cập page chưa load vào RAM | `mlockall()` |
| Stack expansion | Stack page chưa được allocate | Stack pre-fault |
| Memory allocation | `malloc`/`new` gọi `mmap`/`brk` | Object pool, pre-allocate |
| Cache miss | Data không có trong L1/L2 cache | CPU affinity, data locality |
| IRQ | Interrupt handler chạy trên cùng CPU | IRQ affinity, `isolcpus` |
| Scheduler | OS chọn thread khác để chạy | SCHED_FIFO + priority |
| Kernel lock | RT thread chờ kernel spinlock | PREEMPT_RT |

---

## 4. RT Throttling

Linux có cơ chế bảo vệ: RT threads không được chiếm CPU 100% mãi mãi (sẽ làm hệ thống không responsive).

```bash
# Mặc định: RT threads chỉ dùng 95% CPU trong mỗi giây
cat /proc/sys/kernel/sched_rt_period_us   # = 1000000 (1 giây)
cat /proc/sys/kernel/sched_rt_runtime_us  # = 950000  (0.95 giây)
```

**Khi học/dev:** Nên tắt throttling để không bị interrupt:

```bash
sudo sysctl -w kernel.sched_rt_runtime_us=-1   # -1 = disable throttling
```

**Khi production:** Giữ throttling để hệ thống vẫn respond được.

---

## 5. CPU Affinity & Isolation

**CPU affinity:** Chỉ định thread chỉ chạy trên core cụ thể.
**CPU isolation:** Loại core ra khỏi scheduler của OS hoàn toàn.

```
Core 0, 1, 2:  OS scheduler, CFS threads, IRQs, v.v.
Core 3:        ISOLATED — chỉ RT thread của bạn
```

```bash
# Isolate core 3 (cần reboot):
# /etc/default/grub: GRUB_CMDLINE_LINUX_DEFAULT="isolcpus=3 rcu_nocbs=3"

# Pin process vào core 3 không cần reboot:
taskset -c 3 ./rt_program
```

---

## 6. DMA Latency Hint

Yêu cầu system không vào C-state sâu (CPU sleep) — vào/ra C-state tốn ~100μs.

```cpp
// Mở /dev/cpu_dma_latency và ghi 0 → yêu cầu latency = 0
int fd = open("/dev/cpu_dma_latency", O_RDWR);
int32_t zero = 0;
write(fd, &zero, sizeof(zero));
// Giữ fd mở suốt quá trình RT — close fd thì C-state trở lại bình thường
```

---

## 7. Tóm tắt setup đầy đủ cho RT thread

```
1. Boot với PREEMPT_RT kernel
2. Tắt RT throttling: sched_rt_runtime_us = -1
3. Cấp quyền: CAP_SYS_NICE hoặc sudo
4. mlockall(MCL_CURRENT | MCL_FUTURE)    ← tránh page fault
5. Stack pre-fault (touch stack pages)   ← tránh stack expansion fault
6. SCHED_FIFO priority 80-90             ← ưu tiên cao
7. CPU affinity → isolated core          ← giảm jitter
8. /dev/cpu_dma_latency = 0              ← ngăn C-state
9. Pre-allocate memory, object pool      ← tránh malloc trong RT loop
10. Không dùng: printf, malloc, mutex trong hot path
```
