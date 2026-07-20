/**
 * SORTING ALGORITHMS – Competition Reference
 * ============================================
 * Covers: Bubble, Selection, Insertion, Merge, Quick, Counting, Radix
 *
 * Complexity summary:
 *   Bubble:    O(n²)      stable    – educational only
 *   Selection: O(n²)      unstable  – educational only
 *   Insertion: O(n²)      stable    – good for small / nearly-sorted
 *   Merge:     O(n log n) stable    – use when stability matters
 *   Quick:     O(n log n) unstable  – fastest in practice (avg)
 *   Counting:  O(n + k)   stable    – when range k is small
 *   Radix:     O(d*(n+k)) stable    – integers with many digits
 *   STL sort:  O(n log n) unstable  – USE THIS in competition
 */

#include <bits/stdc++.h>
using namespace std;

// ─────────────────────────────────────────────
// 1. BUBBLE SORT  O(n²)
// ─────────────────────────────────────────────
void bubbleSort(vector<int>& a) {
    int n = a.size();
    for (int i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (int j = 0; j < n - i - 1; j++) {
            if (a[j] > a[j + 1]) {
                swap(a[j], a[j + 1]);
                swapped = true;
            }
        }
        if (!swapped) break;  // early exit if already sorted
    }
}

// ─────────────────────────────────────────────
// 2. INSERTION SORT  O(n²) – good for small arrays
// ─────────────────────────────────────────────
void insertionSort(vector<int>& a) {
    int n = a.size();
    for (int i = 1; i < n; i++) {
        int key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

// ─────────────────────────────────────────────
// 3. MERGE SORT  O(n log n) – stable
// ─────────────────────────────────────────────
void merge(vector<int>& a, int l, int m, int r) {
    vector<int> left(a.begin() + l, a.begin() + m + 1);
    vector<int> right(a.begin() + m + 1, a.begin() + r + 1);
    int i = 0, j = 0, k = l;
    while (i < (int)left.size() && j < (int)right.size()) {
        if (left[i] <= right[j]) a[k++] = left[i++];
        else                     a[k++] = right[j++];
    }
    while (i < (int)left.size())  a[k++] = left[i++];
    while (j < (int)right.size()) a[k++] = right[j++];
}

void mergeSort(vector<int>& a, int l, int r) {
    if (l >= r) return;
    int m = l + (r - l) / 2;
    mergeSort(a, l, m);
    mergeSort(a, m + 1, r);
    merge(a, l, m, r);
}

// Count inversions using merge sort  O(n log n)
// An inversion is a pair (i,j) where i<j but a[i]>a[j]
long long countInversions(vector<int>& a, int l, int r) {
    if (l >= r) return 0;
    int m = l + (r - l) / 2;
    long long cnt = 0;
    cnt += countInversions(a, l, m);
    cnt += countInversions(a, m + 1, r);

    vector<int> tmp;
    int i = l, j = m + 1;
    while (i <= m && j <= r) {
        if (a[i] <= a[j]) { tmp.push_back(a[i++]); }
        else {
            cnt += (m - i + 1);   // all remaining left elements > a[j]
            tmp.push_back(a[j++]);
        }
    }
    while (i <= m)  tmp.push_back(a[i++]);
    while (j <= r)  tmp.push_back(a[j++]);
    for (int k = l; k <= r; k++) a[k] = tmp[k - l];
    return cnt;
}

// ─────────────────────────────────────────────
// 4. QUICK SORT  O(n log n) avg – in-place
// ─────────────────────────────────────────────
int partition(vector<int>& a, int l, int r) {
    // Random pivot to avoid worst-case O(n²) on sorted input
    int randIdx = l + rand() % (r - l + 1);
    swap(a[randIdx], a[r]);
    int pivot = a[r];
    int i = l - 1;
    for (int j = l; j < r; j++) {
        if (a[j] <= pivot) swap(a[++i], a[j]);
    }
    swap(a[i + 1], a[r]);
    return i + 1;
}

void quickSort(vector<int>& a, int l, int r) {
    if (l >= r) return;
    int p = partition(a, l, r);
    quickSort(a, l, p - 1);
    quickSort(a, p + 1, r);
}

// ─────────────────────────────────────────────
// 5. COUNTING SORT  O(n + k) – for small integer range
// ─────────────────────────────────────────────
void countingSort(vector<int>& a, int maxVal) {
    vector<int> cnt(maxVal + 1, 0);
    for (int x : a) cnt[x]++;
    int idx = 0;
    for (int v = 0; v <= maxVal; v++)
        while (cnt[v]--) a[idx++] = v;
}

// ─────────────────────────────────────────────
// 6. RADIX SORT  O(d * (n + 10)) – for non-negative integers
// ─────────────────────────────────────────────
void countingSortByDigit(vector<int>& a, int exp) {
    int n = a.size();
    vector<int> output(n), cnt(10, 0);
    for (int x : a) cnt[(x / exp) % 10]++;
    for (int i = 1; i < 10; i++) cnt[i] += cnt[i - 1];
    for (int i = n - 1; i >= 0; i--) {
        int digit = (a[i] / exp) % 10;
        output[--cnt[digit]] = a[i];
    }
    a = output;
}

void radixSort(vector<int>& a) {
    int maxVal = *max_element(a.begin(), a.end());
    for (int exp = 1; maxVal / exp > 0; exp *= 10)
        countingSortByDigit(a, exp);
}

// ─────────────────────────────────────────────
// 7. STL SORT – Use this in competition!
// ─────────────────────────────────────────────
void stlSortExamples() {
    vector<int> a = {5, 2, 8, 1, 9, 3};

    // Ascending (default)
    sort(a.begin(), a.end());

    // Descending
    sort(a.begin(), a.end(), greater<int>());

    // Custom comparator: sort pairs by second element
    vector<pair<int,int>> pairs = {{1,3},{2,1},{3,2}};
    sort(pairs.begin(), pairs.end(), [](const pair<int,int>& x, const pair<int,int>& y){
        return x.second < y.second;
    });

    // Partial sort: find top-k smallest
    vector<int> b = {5, 2, 8, 1, 9, 3};
    int k = 3;
    partial_sort(b.begin(), b.begin() + k, b.end());
    // b[0..k-1] are the k smallest elements, sorted

    // nth_element: O(n) average – puts correct element at position k
    vector<int> c = {5, 2, 8, 1, 9, 3};
    nth_element(c.begin(), c.begin() + k, c.end());
    // c[k] is the k-th smallest element
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Sort array of strings by length, then lexicographically.
 * [2] Count number of inversions in array (use merge sort above).
 * [3] Given array, find k-th largest element in O(n) average.
 * [4] Sort events by start time; if equal, by end time.
 * [5] Stable sort: given student (name, score), sort desc by score
 *     keeping original order for equal scores.
 */

// ─────────────────────────────────────────────
// HELPER: print vector
// ─────────────────────────────────────────────
void print(const vector<int>& a, const string& label = "") {
    if (!label.empty()) cout << label << ": ";
    for (int x : a) cout << x << " ";
    cout << "\n";
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    srand(42);

    vector<int> a = {64, 25, 12, 22, 11};
    print(a, "Original");

    auto test = a;
    bubbleSort(test); print(test, "Bubble");

    test = a;
    insertionSort(test); print(test, "Insertion");

    test = a;
    mergeSort(test, 0, test.size() - 1); print(test, "Merge");

    test = a;
    quickSort(test, 0, test.size() - 1); print(test, "Quick");

    test = a;
    countingSort(test, 100); print(test, "Counting");

    test = a;
    radixSort(test); print(test, "Radix");

    // Count inversions
    vector<int> inv = {3, 1, 2, 4};
    long long inversions = countInversions(inv, 0, inv.size() - 1);
    cout << "Inversions in [3,1,2,4]: " << inversions << "\n";  // expected: 2

    return 0;
}
