/**
 * FENWICK TREE (Binary Indexed Tree – BIT)
 * =========================================
 * Efficient prefix sum with point updates.
 * O(log n) per query and update. Space O(n).
 *
 * Simpler than Segment Tree for prefix sum/frequency problems.
 *
 * Applications:
 *   - Prefix sum with updates
 *   - Count inversions
 *   - Order statistics (rank, kth element)
 *   - 2D queries
 */

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

// ─────────────────────────────────────────────
// 1. BASIC FENWICK TREE – prefix sum
// ─────────────────────────────────────────────
struct Fenwick {
    int n;
    vector<ll> bit;

    Fenwick(int n) : n(n), bit(n + 1, 0) {}

    // Add val to index i (1-indexed)
    void update(int i, ll val) {
        for (; i <= n; i += i & (-i))  // i += lowest set bit
            bit[i] += val;
    }

    // Query prefix sum [1, i]
    ll query(int i) {
        ll sum = 0;
        for (; i > 0; i -= i & (-i))  // i -= lowest set bit
            sum += bit[i];
        return sum;
    }

    // Query range sum [l, r]
    ll query(int l, int r) { return query(r) - query(l - 1); }

    // Point update: set index i to val (requires knowing previous value)
    void set(int i, ll val, ll prev) { update(i, val - prev); }
};

// ─────────────────────────────────────────────
// 2. FENWICK TREE – range update, point query
// ─────────────────────────────────────────────
// Trick: store difference array. update(l,r,val) = update(l,val), update(r+1,-val)
struct FenwickRangeUpdate {
    int n;
    vector<ll> bit;

    FenwickRangeUpdate(int n) : n(n), bit(n + 2, 0) {}

    void update(int i, ll val) {
        for (; i <= n; i += i & (-i)) bit[i] += val;
    }

    // Add val to all elements in [l, r]
    void rangeUpdate(int l, int r, ll val) {
        update(l, val);
        update(r + 1, -val);
    }

    // Query value at index i
    ll pointQuery(int i) {
        ll sum = 0;
        for (; i > 0; i -= i & (-i)) sum += bit[i];
        return sum;
    }
};

// ─────────────────────────────────────────────
// 3. 2D FENWICK TREE  O(log n * log m)
// ─────────────────────────────────────────────
struct Fenwick2D {
    int n, m;
    vector<vector<ll>> bit;

    Fenwick2D(int n, int m) : n(n), m(m), bit(n + 1, vector<ll>(m + 1, 0)) {}

    void update(int r, int c, ll val) {
        for (int i = r; i <= n; i += i & (-i))
            for (int j = c; j <= m; j += j & (-j))
                bit[i][j] += val;
    }

    ll query(int r, int c) {
        ll sum = 0;
        for (int i = r; i > 0; i -= i & (-i))
            for (int j = c; j > 0; j -= j & (-j))
                sum += bit[i][j];
        return sum;
    }

    // Sum of submatrix [r1,c1] to [r2,c2]
    ll query(int r1, int c1, int r2, int c2) {
        return query(r2, c2) - query(r1-1, c2) - query(r2, c1-1) + query(r1-1, c1-1);
    }
};

// ─────────────────────────────────────────────
// 4. COUNT INVERSIONS using Fenwick  O(n log n)
// ─────────────────────────────────────────────
// Number of pairs (i,j) where i<j but a[i]>a[j]
ll countInversions(vector<int> a) {
    // Coordinate compress
    vector<int> sorted = a;
    sort(sorted.begin(), sorted.end());
    sorted.erase(unique(sorted.begin(), sorted.end()), sorted.end());
    for (int& x : a) x = lower_bound(sorted.begin(), sorted.end(), x) - sorted.begin() + 1;

    int n = a.size();
    Fenwick fen(n);
    ll inversions = 0;
    for (int i = n - 1; i >= 0; i--) {
        inversions += fen.query(a[i] - 1);  // count elements smaller than a[i] seen so far
        fen.update(a[i], 1);
    }
    return inversions;
}

// ─────────────────────────────────────────────
// 5. K-TH SMALLEST using Fenwick  O(log² n)
// ─────────────────────────────────────────────
// Find k-th smallest element in a frequency array.
// O(log n) with binary lifting on Fenwick.
int kthSmallest(Fenwick& fen, int k) {
    int pos = 0, n = fen.n;
    for (int pw = 1 << (int)log2(n); pw > 0; pw >>= 1) {
        if (pos + pw <= n && fen.bit[pos + pw] < k) {
            pos += pw;
            k -= fen.bit[pos];
        }
    }
    return pos + 1;
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Range sum query with updates.
 *     Input: array, then queries of type (l,r) or update(i, val).
 *
 * [2] Count smaller numbers after self: for each element, count
 *     how many elements to its right are smaller.
 *     (Process right to left, use Fenwick for count queries)
 *
 * [3] Queue reconstruction by height: people with (h, k) pairs,
 *     h=height, k=number of taller people in front.
 *
 * [4] Minimum operations to make array sorted using Fenwick.
 *
 * [5] 2D range sum: given a matrix, answer range sum queries
 *     and handle point updates.
 */

// Exercise [2] – Count smaller numbers after self
vector<int> countSmaller(vector<int>& nums) {
    // Coordinate compress
    vector<int> sorted = nums;
    sort(sorted.begin(), sorted.end());
    sorted.erase(unique(sorted.begin(), sorted.end()), sorted.end());
    int n = nums.size(), m = sorted.size();
    auto rank = [&](int x) {
        return lower_bound(sorted.begin(), sorted.end(), x) - sorted.begin() + 1;
    };
    Fenwick fen(m);
    vector<int> result(n);
    for (int i = n - 1; i >= 0; i--) {
        int r = rank(nums[i]);
        result[i] = fen.query(r - 1);
        fen.update(r, 1);
    }
    return result;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Basic Fenwick
    vector<int> arr = {1, 7, 3, 0, 7, 8, 3, 2, 6, 2};
    int n = arr.size();
    Fenwick fen(n);
    for (int i = 0; i < n; i++) fen.update(i + 1, arr[i]);

    cout << "Sum [1,5]: " << fen.query(1, 5) << "\n";   // 18 (1+7+3+0+7)
    cout << "Sum [3,7]: " << fen.query(3, 7) << "\n";   // 21 (3+0+7+8+3)

    // Update
    fen.update(3, 5);  // arr[2] += 5, now arr[2] = 8
    cout << "After update, Sum [1,5]: " << fen.query(1, 5) << "\n";  // 23

    // Range update, point query
    FenwickRangeUpdate fru(n);
    fru.rangeUpdate(2, 5, 3);  // add 3 to indices 2..5
    cout << "Point query at 3: " << fru.pointQuery(3) << "\n";  // 3
    cout << "Point query at 1: " << fru.pointQuery(1) << "\n";  // 0

    // Count inversions
    vector<int> a = {3, 1, 2, 4};
    cout << "Inversions in [3,1,2,4]: " << countInversions(a) << "\n";  // 2

    // Count smaller after self
    vector<int> b = {5, 2, 6, 1};
    auto cs = countSmaller(b);
    cout << "Count smaller after self: ";
    for (int x : cs) cout << x << " ";  // 2 1 1 0
    cout << "\n";

    // 2D Fenwick
    Fenwick2D f2d(4, 4);
    f2d.update(1, 1, 3); f2d.update(2, 2, 5); f2d.update(3, 3, 2);
    cout << "2D sum [1,1,3,3]: " << f2d.query(1, 1, 3, 3) << "\n";  // 10

    return 0;
}
