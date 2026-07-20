# DSA Roadmap – LG CodeJam Preparation

> **Mục tiêu**: Nắm vững các thuật toán và cấu trúc dữ liệu thường xuất hiện trong LG CodeJam.
> **Ngôn ngữ**: C++17
> **Thư mục**: `/home/tvn/code_ws/Cpp/DSA/`

---

## Tổng quan LG CodeJam

LG CodeJam thường có 3-5 bài, thời gian 2-3 tiếng. Bài tập tập trung vào:
- **Simulation** – Mô phỏng bài toán thực tế
- **Graph / BFS / DFS** – Tìm đường, đồ thị
- **Dynamic Programming** – Tối ưu hóa
- **Greedy / Sorting** – Tham lam + sắp xếp
- **Data Structures** – Segment Tree, Fenwick, Trie

---

## Cấu trúc thư mục

```
DSA/
├── ROADMAP.md                  ← File này
├── Sorting/
│   └── sorting_algorithms.cpp  ← Bubble, Merge, Quick, Counting, Radix
├── Searching/
│   └── binary_search.cpp       ← Binary search + STL lower/upper_bound
├── DynamicProgramming/
│   ├── dp_01_basics.cpp        ← Fibonacci, Coin change, Climbing stairs
│   ├── dp_02_knapsack.cpp      ← 0/1 Knapsack, Unbounded, Bounded
│   └── dp_03_subsequence.cpp   ← LIS, LCS, Edit distance
├── graph/
│   ├── adj_matrix_and_list.cpp ← (có sẵn)
│   ├── dfs.cpp                 ← (có sẵn)
│   ├── bfs.cpp                 ← BFS + shortest path grid
│   ├── dijkstra.cpp            ← Dijkstra (priority_queue)
│   ├── floyd_warshall.cpp      ← All-pairs shortest path
│   ├── union_find.cpp          ← DSU + Kruskal MST
│   └── topological_sort.cpp    ← Kahn's BFS + DFS toposort
├── tree/
│   ├── segment_tree.cpp        ← (có sẵn) Sum segment tree
│   ├── fenwick_tree.cpp        ← BIT – prefix sum, point update
│   └── trie.cpp                ← Trie – insert, search, prefix count
├── String/
│   └── kmp.cpp                 ← KMP pattern matching + Z-function
├── Math/
│   └── number_theory.cpp       ← Sieve, GCD/LCM, Fast Power, nCr mod p
├── TwoPointer/
│   └── two_pointer.cpp         ← Two sum, sliding window, subarray
├── Stack_Queue/
│   └── monotonic_stack.cpp     ← Monotonic stack, largest rectangle
├── Linklist/                   ← (có sẵn)
├── binary_tree.cpp             ← (có sẵn)
└── Exercise/
    └── problems.cpp            ← Bài tập tổng hợp
```

---

## Lộ trình học (6 tuần)

### Tuần 1 – Nền tảng (Foundation)
| Topic | File | Độ ưu tiên |
|-------|------|-----------|
| Sorting (Merge, Quick) | `Sorting/sorting_algorithms.cpp` | ⭐⭐⭐ |
| Binary Search + variants | `Searching/binary_search.cpp` | ⭐⭐⭐ |
| BFS cơ bản + grid | `graph/bfs.cpp` | ⭐⭐⭐ |
| Two Pointer + Sliding Window | `TwoPointer/two_pointer.cpp` | ⭐⭐⭐ |

**Mục tiêu**: Giải được bài Simulation và bài tìm đường 2D grid.

---

### Tuần 2 – Graph (Đồ thị)
| Topic | File | Độ ưu tiên |
|-------|------|-----------|
| DFS/BFS + Connected components | `graph/dfs.cpp` | ⭐⭐⭐ |
| Dijkstra (weighted graph) | `graph/dijkstra.cpp` | ⭐⭐⭐ |
| Union-Find (DSU) + Kruskal | `graph/union_find.cpp` | ⭐⭐⭐ |
| Topological Sort + DAG | `graph/topological_sort.cpp` | ⭐⭐ |
| Floyd-Warshall | `graph/floyd_warshall.cpp` | ⭐⭐ |

**Mục tiêu**: Giải thành thạo bài đồ thị tìm đường ngắn nhất, MST.

---

### Tuần 3 – Dynamic Programming (DP)
| Topic | File | Độ ưu tiên |
|-------|------|-----------|
| DP cơ bản (memoization, tabulation) | `DynamicProgramming/dp_01_basics.cpp` | ⭐⭐⭐ |
| Knapsack 0/1 + Unbounded | `DynamicProgramming/dp_02_knapsack.cpp` | ⭐⭐⭐ |
| LIS, LCS, Edit Distance | `DynamicProgramming/dp_03_subsequence.cpp` | ⭐⭐⭐ |
| DP trên Grid (matrix path) | `DynamicProgramming/dp_01_basics.cpp` | ⭐⭐ |

**Mục tiêu**: Nhận diện và giải bài DP 1D, 2D.

---

### Tuần 4 – Data Structures nâng cao
| Topic | File | Độ ưu tiên |
|-------|------|-----------|
| Segment Tree (sum, min, max) | `tree/segment_tree.cpp` | ⭐⭐⭐ |
| Fenwick Tree (BIT) | `tree/fenwick_tree.cpp` | ⭐⭐⭐ |
| Trie (prefix tree) | `tree/trie.cpp` | ⭐⭐ |
| Monotonic Stack + Queue | `Stack_Queue/monotonic_stack.cpp` | ⭐⭐⭐ |

**Mục tiêu**: Giải bài range query, frequency count.

---

### Tuần 5 – String & Math
| Topic | File | Độ ưu tiên |
|-------|------|-----------|
| KMP pattern matching | `String/kmp.cpp` | ⭐⭐⭐ |
| Z-function | `String/kmp.cpp` | ⭐⭐ |
| Sieve of Eratosthenes | `Math/number_theory.cpp` | ⭐⭐⭐ |
| GCD/LCM, Fast Power | `Math/number_theory.cpp` | ⭐⭐⭐ |
| nCr mod p | `Math/number_theory.cpp` | ⭐⭐ |

**Mục tiêu**: Xử lý bài chuỗi và số học.

---

### Tuần 6 – Luyện tập tổng hợp
- Làm bài tập trong `Exercise/problems.cpp`
- Giải đề thi cũ LG CodeJam (nếu có)
- Tổng ôn các pattern hay gặp

---

## Các pattern hay gặp trong thi

| Pattern | Nhận diện | Kỹ thuật |
|---------|-----------|----------|
| Tìm đường ngắn nhất | Đồ thị, lưới 2D | BFS (unweighted), Dijkstra (weighted) |
| Tối ưu hóa chọn/không chọn | "chọn tập con tối ưu" | DP Knapsack |
| Đếm số cách | "có bao nhiêu cách" | DP bottom-up |
| Kết nối thành phần | "nhóm/cluster" | Union-Find / BFS |
| Dãy tăng dài nhất | LIS variant | DP + Binary search O(n log n) |
| Xử lý range query | "tổng/max đoạn [l,r]" | Segment Tree / Fenwick |
| Tìm pattern trong string | chuỗi con | KMP |
| Vấn đề hàng xóm | "largest/smallest nearby" | Monotonic Stack |
| Scheduling/Ordering | "thứ tự thực hiện" | Topological Sort |

---

## Template C++ cho thi đấu

```cpp
#include <bits/stdc++.h>
using namespace std;

typedef long long ll;
typedef pair<int,int> pii;
typedef pair<ll,ll> pll;
typedef vector<int> vi;
typedef vector<ll> vll;
typedef vector<pii> vpii;

#define pb push_back
#define mp make_pair
#define fi first
#define se second
#define rep(i,a,b) for(int i=(a);i<(b);i++)
#define all(x) (x).begin(),(x).end()

const int INF = 1e9;
const ll LINF = 1e18;
const int MOD = 1e9 + 7;

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Code here

    return 0;
}
```

---

## Checklist trước khi thi

- [ ] Đọc kỹ đề, xác định input/output format
- [ ] Xác định constraints (n ≤ 10^5 → O(n log n); n ≤ 10^3 → O(n²))
- [ ] Vẽ ví dụ nhỏ bằng tay
- [ ] Nhận diện pattern (graph/DP/greedy/simulation)
- [ ] Code template, test với sample
- [ ] Edge case: n=0, n=1, tất cả giống nhau
- [ ] Submit
