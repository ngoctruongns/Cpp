# Practice Set: Related Knowledge for Gondola Lifts

## A. MST / Kruskal Foundation

1. Bài A1 (Easy)
- Cho đồ thị vô hướng có trọng số, tính tổng MST bằng Kruskal.
- Mục tiêu: thuần DSU + sort cạnh.

2. Bài A2 (Medium)
- Có nhiều cạnh trùng trọng số. Hãy đếm số lần giảm thành phần ở mỗi mức trọng số.
- Mục tiêu: hiểu sâu công thức Σ merges * weight.

3. Bài A3 (Medium)
- Với mỗi ngưỡng W, đếm số thành phần của đồ thị chỉ dùng cạnh <= W.
- Mục tiêu: kết nối tư duy Kruskal với connectivity-by-threshold.

## B. Sweep by Value trên mảng

4. Bài B1 (Easy)
- Cho mảng A. Với mỗi ngưỡng h, đánh dấu vị trí A[i] <= h và đếm số block liên tiếp active.
- Mục tiêu: active theo thứ tự tăng dần giá trị.

5. Bài B2 (Medium)
- Cập nhật online: bật một vị trí, hỏi tổng bình phương độ dài các block active.
- Mục tiêu: luyện merge interval bằng set/map.

## C. Interval Data Structure

6. Bài C1 (Easy)
- Quản lý tập đoạn rời nhau. Hỗ trợ add điểm x và trả về độ dài block chứa x.
- Mục tiêu: thao tác prev/next, upper_bound/lower_bound an toàn.

7. Bài C2 (Medium)
- Khi add điểm x, in ra số block active hiện tại.
- Mục tiêu: bắt đúng 4 case merge: none/left/right/both.

8. Bài C3 (Hard)
- Mỗi block [l, r] có giá trị F(r-l+1). Add điểm và duy trì tổng F toàn cục.
- Mục tiêu: pattern y hệt bài Gondola (thay F = comp).

## D. Đồ thị đặc biệt theo khoảng cách

9. Bài D1 (Medium)
- Đồ thị trên 1..L, có cạnh giữa u-v nếu |u-v| >= K. Đếm số thành phần.
- Mục tiêu: tự chứng minh công thức comp(L, K).

10. Bài D2 (Hard)
- Mở rộng: cạnh nếu |u-v| thuộc [K1, K2]. Đếm thành phần.
- Mục tiêu: luyện suy luận cấu trúc đồ thị theo chỉ số.

## E. End-to-End giống bài ảnh

11. Bài E1 (Hard)
- Giữ nguyên đề gốc, nhưng yêu cầu thêm in ra số merges tại mỗi độ cao.
- Mục tiêu: nhìn rõ dòng Kruskal theo threshold.

12. Bài E2 (Hard)
- Giữ nguyên đề gốc, thêm truy vấn thay đổi một A[i] rồi hỏi lại đáp án.
- Mục tiêu: tư duy cấu trúc dữ liệu nâng cao (offline / divide-and-conquer).

---

## Cách luyện đề hiệu quả

1. Luôn có 2 bản code:
- brute chậm để kiểm chứng.
- optimize để nộp.

2. Test ngẫu nhiên:
- Sinh N nhỏ (<= 10), so brute và optimize 10k lần.

3. Bộ test biên bắt buộc:
- N = 2, K = 1
- K = N-1
- N = 2K
- N = 2K-1
- Mọi A bằng nhau
- A tăng dần / giảm dần / zig-zag
