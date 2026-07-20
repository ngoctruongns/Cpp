#include <iostream>
#include <vector>

using namespace std;

class SegmentTree {
private:
    vector<int> tree;
    int n;

    void build(const vector<int>& arr, int node, int start, int end) {
        if (start == end) {
            tree[node] = arr[start];
        } else {
            int mid = (start + end) / 2;
            build(arr, 2 * node + 1, start, mid);
            build(arr, 2 * node + 2, mid + 1, end);
            tree[node] = tree[2 * node + 1] + tree[2 * node + 2];
        }
    }

    void update(int node, int start, int end, int idx, int val) {
        if (start == end) {
            tree[node] = val;
        } else {
            int mid = (start + end) / 2;
            if (idx <= mid) {
                update(2 * node + 1, start, mid, idx, val);
            } else {
                update(2 * node + 2, mid + 1, end, idx, val);
            }
            // Update current node after updating child
            tree[node] = tree[2 * node + 1] + tree[2 * node +2];
        }
    }

    int query(int node, int start, int end, int l, int r) {
        if (l > end || r < start) return 0;

        if (l <= start && end <= r) return tree[node];

        int mid = (start + end) / 2;
        int p1 = query(2 * node + 1, start, mid, l , r);
        int p2 = query(2 * node + 2, mid + 1, end, l, r);

        cout << node << ": p1=" << p1 << ", p2=" << p2 << "\n"; // debug print
        return (p1 + p2);
    }

public:
    SegmentTree(const vector<int>& arr) {
        n = arr.size();
        tree.resize(4 * n);
        build(arr, 0, 0, n - 1);
    }

    void update(int idx, int val) {
        update(0, 0, n - 1, idx, val);
    }

    int query(int l, int r) {
        return query(0, 0, n - 1, l, r);
    }

    // print tree for debugging
    void printTree() {
        //print as tree structure display
        int level = 0, count = 1;
        for (int i = 0; i < (int)tree.size(); i++) {
            cout << tree[i] << " ";
            if (--count == 0) {
                cout << "\n";
                level++;
                count = 1 << level;
            }
        }
        cout << "\n";
    }

};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    vector<int> arr = {1, 3, 5, 7, 9, 11};
    SegmentTree seg(arr);

    cout << "Sum of values in range [1, 3]: " << seg.query(1, 3) << "\n"; // 15 (3+5+7)
    // cout << "Sum of values in range [0, 5]: " << seg.query(0, 5) << "\n"; // 36 (1+3+5+7+9+11)

    seg.printTree(); // print internal tree structure
    seg.update(2, 10); // arr[2] = 10
    cout << "After update, sum of values in range [1, 3]: " << seg.query(1, 3) << "\n"; // 20 (3+10+7)

    return 0;
}