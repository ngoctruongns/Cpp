#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>

using namespace std;

class SegmentTree {
private:
    vector<int> tree;
    int n;

    void build(vector<int>& arr, int p, int lo, int hi) {
        if (lo == hi) {
            tree[p] = arr[lo];
            return;
        }
        int mid = lo + (hi - lo) / 2;

        build(arr, 2 * p + 1, lo, mid);
        build(arr, 2 * p + 2, mid + 1, hi);
        tree[p] = max(tree[2*p + 1], tree[2*p + 2]);
    }

    int queryTree(int p, int lo, int hi, int qlo, int qhi) {
        if (qhi < lo || hi < qlo) return INT_MIN;
        if (qlo <= lo && hi <= qhi) return tree[p];

        int mid = lo + (hi - lo) / 2;

        int left = queryTree(2 * p + 1, lo, mid, qlo, qhi);
        int right = queryTree(2 * p + 2, mid + 1, hi, qlo, qhi);

        return max(left, right);
    }

    void updateTree(int p, int lo, int hi, int idx, int val) {
        if (lo == hi) {
            tree[p] = val;
            return;
        }
        int mid = lo + (hi - lo) / 2;

        if (idx <= mid) {
            updateTree(2 * p + 1, lo, mid, idx, val);
        } else {
            updateTree(2 * p + 2, mid + 1, hi, idx, val);
        }
        tree[p] = max(tree[2*p + 1], tree[2*p + 2]);
    }

public:
    SegmentTree(vector<int>& arr) {
        n = arr.size();
        tree.resize(n * 4, 0);
        build(arr, 0, 0, n -1);
    }
    
    int query(int le, int ri) {
        return queryTree(0, 0, n - 1, le, ri);
    }

    void update(int idx, int val) {
        updateTree(0, 0, n - 1, idx, val);
    }
};

int main(void) {
    // Mảng ban đầu (chỉ số từ 0 đến 5)
    vector<int> arr = {-1, -7, -8, -2, 9, -5};
    
    // Khởi tạo Segment Tree
    SegmentTree st(arr);

    // Truy vấn giá trị lớn nhất từ chỉ số 1 đến 3 (đoạn {3, 8, 2})
    cout << "Max trong doan: " << st.query(1, 3) << "\n"; // Kết quả: 8

    // Cập nhật phần tử tại chỉ số 1 thành 10 (mảng thành {1, 10, 8, 2, 9, 5})
    st.update(1, -10);
    cout << "Sau khi cap nhat phan tu index 1 thanh 10:\n";

    // Truy vấn lại giá trị lớn nhất từ chỉ số 1 đến 3 (đoạn {10, 8, 2})
    cout << "Max trong doan: " << st.query(1, 3) << "\n"; // Kết quả: 10

    // Truy vấn giá trị lớn nhất trên toàn bộ mảng
    cout << "Max toan bo mang: " << st.query(0, 5) << "\n"; // Kết quả: 10
    
    return 0;
}