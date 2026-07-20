/**
 * BINARY SEARCH – Competition Reference
 * =======================================
 * Key insight: Binary search works on ANY monotone predicate, not just arrays.
 *
 * Template: find smallest x in [lo, hi] where predicate(x) is true.
 *
 * STL equivalents:
 *   lower_bound(begin, end, val)  → first position >= val
 *   upper_bound(begin, end, val)  → first position >  val
 */

#include <bits/stdc++.h>
using namespace std;

// ─────────────────────────────────────────────
// 1. CLASSIC BINARY SEARCH – find exact value
// ─────────────────────────────────────────────
// Returns index of target, or -1 if not found.
int binarySearch(const vector<int>& a, int target) {
    int lo = 0, hi = (int)a.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;   // avoid overflow (never use (lo+hi)/2)
        if (a[mid] == target) return mid;
        else if (a[mid] < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

// ─────────────────────────────────────────────
// 2. LOWER BOUND – first index where a[i] >= target
// ─────────────────────────────────────────────
// Returns n if all elements < target.
int lowerBound(const vector<int>& a, int target) {
    int lo = 0, hi = (int)a.size();   // hi = n (one past last)
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] < target) lo = mid + 1;
        else                 hi = mid;
    }
    return lo;  // same as std::lower_bound
}

// ─────────────────────────────────────────────
// 3. UPPER BOUND – first index where a[i] > target
// ─────────────────────────────────────────────
int upperBound(const vector<int>& a, int target) {
    int lo = 0, hi = (int)a.size();
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] <= target) lo = mid + 1;
        else                  hi = mid;
    }
    return lo;  // same as std::upper_bound
}

// Count occurrences of target in sorted array
int countOccurrences(const vector<int>& a, int target) {
    return upperBound(a, target) - lowerBound(a, target);
    // Or: upper_bound - lower_bound using STL
}

// ─────────────────────────────────────────────
// 4. BINARY SEARCH ON ANSWER (most important in contests!)
// ─────────────────────────────────────────────
// Pattern: "find minimum/maximum value that satisfies condition"
//
// Example: Given n tasks with times[], k workers.
// Minimize the maximum time assigned to any worker.
bool canFinish(const vector<int>& times, int k, long long maxTime) {
    int workers = 1;
    long long cur = 0;
    for (int t : times) {
        if (cur + t > maxTime) { workers++; cur = t; }
        else cur += t;
    }
    return workers <= k;
}

int minMaxWorkTime(vector<int>& times, int k) {
    long long lo = *max_element(times.begin(), times.end());
    long long hi = 0;
    for (int t : times) hi += t;

    while (lo < hi) {
        long long mid = lo + (hi - lo) / 2;
        if (canFinish(times, k, mid)) hi = mid;
        else lo = mid + 1;
    }
    return lo;
}

// Example 2: "Koko eating bananas" – minimum eating speed
// Given piles[], h hours, find min speed k such that can eat all in h hours.
bool canEat(const vector<int>& piles, long long k, int h) {
    long long hours = 0;
    for (int p : piles) hours += (p + k - 1) / k;  // ceil(p/k)
    return hours <= h;
}

int minEatingSpeed(vector<int>& piles, int h) {
    int lo = 1, hi = *max_element(piles.begin(), piles.end());
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (canEat(piles, mid, h)) hi = mid;
        else lo = mid + 1;
    }
    return lo;
}

// Example 3: Binary search on real numbers (e.g. find sqrt)
double mySqrt(double x) {
    if (x < 1) return x;  // for x in (0,1), sqrt > x
    double lo = 1, hi = x;
    for (int iter = 0; iter < 100; iter++) {  // 100 iterations → 2^-100 precision
        double mid = (lo + hi) / 2;
        if (mid * mid < x) lo = mid;
        else hi = mid;
    }
    return lo;
}

// ─────────────────────────────────────────────
// 5. STL USAGE – memorize these!
// ─────────────────────────────────────────────
void stlExamples() {
    vector<int> a = {1, 2, 4, 4, 5, 7, 9};

    // lower_bound: first element >= val
    auto lb = lower_bound(a.begin(), a.end(), 4);
    cout << "lower_bound(4) index: " << lb - a.begin() << "\n";  // 2

    // upper_bound: first element > val
    auto ub = upper_bound(a.begin(), a.end(), 4);
    cout << "upper_bound(4) index: " << ub - a.begin() << "\n";  // 4

    // Count 4s
    cout << "Count of 4: " << ub - lb << "\n";  // 2

    // Check if value exists
    bool exists = binary_search(a.begin(), a.end(), 5);  // true
    cout << "5 exists: " << exists << "\n";

    // With set/multiset (internally sorted)
    set<int> s = {1, 3, 5, 7, 9};
    auto it = s.lower_bound(4);  // points to 5
    cout << "set lower_bound(4): " << *it << "\n";  // 5

    // With map
    map<int,int> m = {{1,10},{3,30},{5,50}};
    auto mit = m.lower_bound(2);  // points to {3,30}
    cout << "map lower_bound(2) key: " << mit->first << "\n";
}

// ─────────────────────────────────────────────
// 6. TERNARY SEARCH – find maximum of unimodal function
// ─────────────────────────────────────────────
// Use when the function first increases then decreases (or vice versa).
// Example: f(x) = -(x-3)² + 10  peaks at x=3

double unimodalFunc(double x) {
    return -(x - 3) * (x - 3) + 10;
}

double ternarySearchMax(double lo, double hi) {
    for (int iter = 0; iter < 200; iter++) {
        double m1 = lo + (hi - lo) / 3;
        double m2 = hi - (hi - lo) / 3;
        if (unimodalFunc(m1) < unimodalFunc(m2)) lo = m1;
        else hi = m2;
    }
    return (lo + hi) / 2;
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Given sorted array, find floor and ceiling of x.
 *     Floor = largest element <= x
 *     Ceiling = smallest element >= x
 *
 * [2] Rotated sorted array: find the minimum element.
 *     (array was sorted then rotated at some pivot)
 *
 * [3] Given sorted matrix (each row sorted, row[i][0] > row[i-1][last]),
 *     search for a target value in O(log(m*n)).
 *
 * [4] "Aggressive cows" – classic binary search on answer:
 *     Given stalls[] positions, place k cows to maximize minimum distance.
 *
 * [5] Find median of two sorted arrays in O(log(min(m,n))).
 */

// Exercise [1] solution – floor and ceiling
pair<int,int> floorCeiling(const vector<int>& a, int x) {
    int n = a.size();
    int lb = lowerBound(a, x);  // first >= x

    int ceiling = (lb < n) ? a[lb] : -1;

    // floor = largest <= x → upper_bound(x) - 1
    int ub = upperBound(a, x);  // first > x
    int floor = (ub > 0) ? a[ub - 1] : -1;

    return {floor, ceiling};
}

// Exercise [4] solution – Aggressive cows
bool canPlace(const vector<int>& stalls, int k, int minDist) {
    int count = 1, last = stalls[0];
    for (int i = 1; i < (int)stalls.size(); i++) {
        if (stalls[i] - last >= minDist) {
            count++;
            last = stalls[i];
            if (count >= k) return true;
        }
    }
    return count >= k;
}

int aggressiveCows(vector<int> stalls, int k) {
    sort(stalls.begin(), stalls.end());
    int lo = 1, hi = stalls.back() - stalls.front();
    int ans = 0;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (canPlace(stalls, k, mid)) { ans = mid; lo = mid + 1; }
        else hi = mid - 1;
    }
    return ans;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Test basic binary search
    vector<int> a = {1, 3, 5, 7, 9, 11};
    cout << "Index of 7: " << binarySearch(a, 7) << "\n";        // 3
    cout << "Index of 6: " << binarySearch(a, 6) << "\n";        // -1
    cout << "lower_bound(6): " << lowerBound(a, 6) << "\n";      // 3 (points to 7)
    cout << "upper_bound(7): " << upperBound(a, 7) << "\n";      // 4 (points to 9)

    // Test floor/ceiling
    auto [fl, ce] = floorCeiling(a, 6);
    cout << "Floor of 6: " << fl << ", Ceiling of 6: " << ce << "\n";  // 5, 7

    // Test binary search on answer
    vector<int> times = {3, 2, 3};
    cout << "Min max work time (3 workers, 3 tasks): "
         << minMaxWorkTime(times, 2) << "\n";  // 3

    // Test aggressive cows
    vector<int> stalls = {1, 2, 8, 4, 9};
    cout << "Aggressive cows (3 cows): " << aggressiveCows(stalls, 3) << "\n";  // 3

    // STL examples
    stlExamples();

    // Ternary search
    cout << "Max of unimodal func at x=" << ternarySearchMax(0, 10) << "\n";  // ~3

    return 0;
}
