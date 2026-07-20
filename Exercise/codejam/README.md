# CodeJam Study Pack: Gondola Lifts Between Peaks

## 1) Bài toán trong ảnh thuộc nhóm nào?

Bài này là tổ hợp của 4 mảng kiến thức quan trọng:

1. Minimum Spanning Tree (MST) / Kruskal tư duy theo ngưỡng.
2. Quét ngưỡng (sweep by value) trên mảng.
3. Quản lý đoạn liên tiếp động (dynamic contiguous intervals).
4. Đếm số thành phần liên thông theo công thức cho đồ thị đặc biệt.

---

## 2) Ý tưởng lời giải tối ưu

Ta coi mỗi đỉnh là một peak. Cạnh hợp lệ: (i, j), j - i >= K.
Trọng số cạnh: max(A[i..j]).

Nếu làm Kruskal trực tiếp sẽ có O(N^2) cạnh, không chạy được.

Đổi góc nhìn:

- Với ngưỡng h, xét đồ thị G_h gồm các cạnh có cost <= h.
- Gọi C(h) là số thành phần liên thông của G_h.
- Khi tăng h theo thứ tự tăng dần các độ cao, số thành phần chỉ giảm.
- Tổng MST chính là:

MST = Σ (C(prev) - C(h)) * h

Vì mỗi lần giảm thành phần là một lần Kruskal chọn cạnh ở mức trọng số h.

### Mấu chốt để tính C(h)

Cạnh có cost <= h khi và chỉ khi toàn bộ đoạn giữa 2 đỉnh đều có A <= h.
Nghĩa là cạnh chỉ nằm bên trong các block liên tiếp đã kích hoạt (A <= h).

Với một block có độ dài L, đồ thị nội bộ có cạnh giữa 2 vị trí nếu khoảng cách >= K.
Số thành phần liên thông của block có thể tính O(1):

- Các đỉnh bị cô lập là các đỉnh không có hàng xóm nào cách >= K.
- Số đỉnh cô lập là kích thước giao của đoạn [1..L] với [L-K+1..K].
- Gọi isolated là số đó, thì:

comp(L) = isolated + (isolated < L ? 1 : 0)

Giải thích:
- isolated đỉnh cô lập, mỗi đỉnh là 1 thành phần.
- Tất cả đỉnh còn lại (nếu có) nằm chung 1 thành phần.

Khi quét h tăng dần:
- Kích hoạt các vị trí có A[i] = h.
- Cập nhật các block liên tiếp bằng set interval.
- activeComp = tổng comp(L) trên các block active.
- inactiveCount = số đỉnh chưa active (mỗi đỉnh là 1 thành phần).
- C(h) = activeComp + inactiveCount.

Độ phức tạp: O(N log N), bộ nhớ O(N).

---

## 3) Điều kiện bất khả thi

Bài toán không thể kết nối toàn bộ nếu N < 2K.

Vì khi đó tồn tại ít nhất một vị trí không có đỉnh nào đủ xa để nối, nên bị cô lập vĩnh viễn.

---

## 4) File lời giải chính

- Lời giải đầy đủ: [Exercise/codejam/gondola_lifts.cpp](Exercise/codejam/gondola_lifts.cpp)

Build nhanh:

- g++ -std=c++17 -O2 Exercise/codejam/gondola_lifts.cpp -o /tmp/gondola

Run:

- /tmp/gondola

---

## 5) Checklist kiến thức cần nắm

1. Kruskal và ý nghĩa thành phần liên thông theo ngưỡng.
2. Kỹ thuật sweep theo giá trị (offline by value).
3. Quản lý các đoạn [l, r] bằng ordered set.
4. Chứng minh công thức đếm comp(L).
5. Kiểm tra biên: N nhỏ, K lớn, nhiều giá trị trùng nhau.

---

## 6) Lộ trình luyện ngắn cho đúng bài này

1. Viết brute-force Kruskal O(N^2 log N) cho N <= 200.
2. Viết hàm comp(L, K) và test ngẫu nhiên.
3. Viết module quản lý merge/split interval khi active điểm mới.
4. Ghép toàn bộ sweep + công thức MST.
5. Dùng brute để đối chiếu random test nhỏ trước khi submit.
