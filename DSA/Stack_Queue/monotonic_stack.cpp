/**
 * MONOTONIC STACK & MONOTONIC QUEUE
 * ===================================
 * Monotonic Stack: elements in stack are always increasing or decreasing.
 * Monotonic Queue (deque): supports range max/min in O(1).
 *
 * Key pattern:
 *   "Find next/previous greater/smaller element" → Monotonic Stack
 *   "Sliding window maximum/minimum"             → Monotonic Deque
 *   "Largest rectangle / Maximal square"         → Monotonic Stack
 */

#include <bits/stdc++.h>
using namespace std;

// ─────────────────────────────────────────────
// 1. NEXT GREATER ELEMENT  O(n)
// ─────────────────────────────────────────────
// For each element, find the first element to the RIGHT that is greater.
// -1 if no such element.
vector<int> nextGreater(const vector<int>& a) {
    int n = a.size();
    vector<int> result(n, -1);
    stack<int> stk;  // stores indices, stack is decreasing (top is smallest)
    for (int i = 0; i < n; i++) {
        while (!stk.empty() && a[stk.top()] < a[i]) {
            result[stk.top()] = a[i];
            stk.pop();
        }
        stk.push(i);
    }
    return result;
}

// Next greater in CIRCULAR array
vector<int> nextGreaterCircular(const vector<int>& a) {
    int n = a.size();
    vector<int> result(n, -1);
    stack<int> stk;
    for (int i = 0; i < 2 * n; i++) {   // traverse twice
        while (!stk.empty() && a[stk.top()] < a[i % n]) {
            result[stk.top()] = a[i % n];
            stk.pop();
        }
        if (i < n) stk.push(i);
    }
    return result;
}

// ─────────────────────────────────────────────
// 2. PREVIOUS SMALLER ELEMENT  O(n)
// ─────────────────────────────────────────────
// For each element, find nearest SMALLER element to the LEFT.
// Returns index (-1 if none).
vector<int> prevSmaller(const vector<int>& a) {
    int n = a.size();
    vector<int> result(n, -1);
    stack<int> stk;  // stores indices, increasing (top is largest)
    for (int i = 0; i < n; i++) {
        while (!stk.empty() && a[stk.top()] >= a[i]) stk.pop();
        result[i] = stk.empty() ? -1 : stk.top();
        stk.push(i);
    }
    return result;
}

// ─────────────────────────────────────────────
// 3. LARGEST RECTANGLE IN HISTOGRAM  O(n)
// ─────────────────────────────────────────────
// Classic problem: given heights[], find max area rectangle.
int largestRectangle(const vector<int>& heights) {
    int n = heights.size(), maxArea = 0;
    stack<int> stk;  // indices of bars in increasing height order
    for (int i = 0; i <= n; i++) {
        int h = (i == n) ? 0 : heights[i];
        while (!stk.empty() && heights[stk.top()] > h) {
            int height = heights[stk.top()]; stk.pop();
            int width = stk.empty() ? i : i - stk.top() - 1;
            maxArea = max(maxArea, height * width);
        }
        stk.push(i);
    }
    return maxArea;
}

// ─────────────────────────────────────────────
// 4. MAXIMAL RECTANGLE IN BINARY MATRIX  O(m*n)
// ─────────────────────────────────────────────
// Use histogram approach row by row.
int maximalRectangle(vector<vector<char>>& matrix) {
    if (matrix.empty()) return 0;
    int rows = matrix.size(), cols = matrix[0].size();
    vector<int> heights(cols, 0);
    int maxArea = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++)
            heights[c] = (matrix[r][c] == '1') ? heights[c] + 1 : 0;
        maxArea = max(maxArea, largestRectangle(heights));
    }
    return maxArea;
}

// ─────────────────────────────────────────────
// 5. DAILY TEMPERATURES  O(n)
// ─────────────────────────────────────────────
// For each day, how many days until warmer temperature?
vector<int> dailyTemperatures(const vector<int>& T) {
    int n = T.size();
    vector<int> result(n, 0);
    stack<int> stk;
    for (int i = 0; i < n; i++) {
        while (!stk.empty() && T[stk.top()] < T[i]) {
            result[stk.top()] = i - stk.top();
            stk.pop();
        }
        stk.push(i);
    }
    return result;
}

// ─────────────────────────────────────────────
// 6. SUM OF SUBARRAY MINIMUMS  O(n)
// ─────────────────────────────────────────────
// Sum of min(a[i..j]) for all subarrays.
// Contribution of a[i] as minimum = (i - left[i]) * (right[i] - i)
int sumSubarrayMins(const vector<int>& a) {
    const int MOD = 1e9 + 7;
    int n = a.size();
    vector<int> left(n), right(n);  // distances to prev/next smaller

    // Previous smaller or equal (left boundary)
    stack<int> stk;
    for (int i = 0; i < n; i++) {
        while (!stk.empty() && a[stk.top()] >= a[i]) stk.pop();
        left[i] = stk.empty() ? i + 1 : i - stk.top();
        stk.push(i);
    }

    // Next strictly smaller (right boundary)
    while (!stk.empty()) stk.pop();
    for (int i = n - 1; i >= 0; i--) {
        while (!stk.empty() && a[stk.top()] > a[i]) stk.pop();
        right[i] = stk.empty() ? n - i : stk.top() - i;
        stk.push(i);
    }

    long long sum = 0;
    for (int i = 0; i < n; i++)
        sum = (sum + (long long)a[i] * left[i] % MOD * right[i]) % MOD;
    return sum;
}

// ─────────────────────────────────────────────
// 7. SLIDING WINDOW MAXIMUM  O(n)  (Monotonic Deque)
// ─────────────────────────────────────────────
// Find max in every window of size k.
vector<int> slidingWindowMax(const vector<int>& nums, int k) {
    int n = nums.size();
    vector<int> result;
    deque<int> dq;  // stores indices, decreasing values (front = max)
    for (int i = 0; i < n; i++) {
        // Remove elements outside window
        while (!dq.empty() && dq.front() <= i - k) dq.pop_front();
        // Maintain decreasing order (remove smaller elements from back)
        while (!dq.empty() && nums[dq.back()] <= nums[i]) dq.pop_back();
        dq.push_back(i);
        if (i >= k - 1) result.push_back(nums[dq.front()]);
    }
    return result;
}

// ─────────────────────────────────────────────
// 8. REMOVE K DIGITS TO GET SMALLEST NUMBER  O(n)
// ─────────────────────────────────────────────
string removeKDigits(string num, int k) {
    stack<char> stk;
    for (char c : num) {
        while (k > 0 && !stk.empty() && stk.top() > c) { stk.pop(); k--; }
        stk.push(c);
    }
    while (k--) stk.pop();  // remove from end if still have removals left
    string result;
    while (!stk.empty()) { result += stk.top(); stk.pop(); }
    reverse(result.begin(), result.end());
    // Remove leading zeros
    int start = 0;
    while (start < (int)result.size() - 1 && result[start] == '0') start++;
    return result.substr(start);
}

// ─────────────────────────────────────────────
// 9. STOCK SPAN PROBLEM  O(n)
// ─────────────────────────────────────────────
// For each day, find max span = number of consecutive days (including today)
// where price was <= today's price.
vector<int> stockSpan(const vector<int>& prices) {
    int n = prices.size();
    vector<int> span(n, 1);
    stack<int> stk;  // stack of indices with decreasing prices
    for (int i = 0; i < n; i++) {
        while (!stk.empty() && prices[stk.top()] <= prices[i]) stk.pop();
        span[i] = stk.empty() ? i + 1 : i - stk.top();
        stk.push(i);
    }
    return span;
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Asteroid collision: array of asteroids (positive=right, negative=left).
 *     Simulate collisions (equal masses both explode, larger survives).
 *
 * [2] 132 pattern: find triplet where a[i] < a[k] < a[j] (i<j<k).
 *     Process from right, maintain candidates for a[k] using monotonic stack.
 *
 * [3] Largest rectangle in histogram with coordinate compression.
 *
 * [4] Maximum width ramp: max j-i where nums[i] <= nums[j].
 *     Build decreasing stack from left, then scan from right.
 *
 * [5] Online stock span: implement class that processes stock prices one
 *     at a time and returns span for each day.
 */

// Exercise [1] – Asteroid collision
vector<int> asteroidCollision(vector<int>& asteroids) {
    stack<int> stk;
    for (int a : asteroids) {
        bool alive = true;
        while (alive && a < 0 && !stk.empty() && stk.top() > 0) {
            if (stk.top() < -a) stk.pop();
            else if (stk.top() == -a) { stk.pop(); alive = false; }
            else alive = false;
        }
        if (alive) stk.push(a);
    }
    vector<int> result;
    while (!stk.empty()) { result.push_back(stk.top()); stk.pop(); }
    reverse(result.begin(), result.end());
    return result;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Next greater
    vector<int> a = {4, 5, 2, 10, 8};
    auto ng = nextGreater(a);
    cout << "Next greater: ";
    for (int x : ng) cout << x << " ";  // 5 10 10 -1 -1
    cout << "\n";

    // Previous smaller
    auto ps = prevSmaller(a);
    cout << "Prev smaller idx: ";
    for (int x : ps) cout << x << " ";  // -1 -1 1 -1 3
    cout << "\n";

    // Largest rectangle in histogram
    vector<int> h = {2, 1, 5, 6, 2, 3};
    cout << "Largest rectangle: " << largestRectangle(h) << "\n";  // 10

    // Maximal rectangle in matrix
    vector<vector<char>> mat = {
        {'1','0','1','0','0'},
        {'1','0','1','1','1'},
        {'1','1','1','1','1'},
        {'1','0','0','1','0'}
    };
    cout << "Maximal rectangle: " << maximalRectangle(mat) << "\n";  // 6

    // Daily temperatures
    vector<int> T = {73, 74, 75, 71, 69, 72, 76, 73};
    auto dt = dailyTemperatures(T);
    cout << "Daily temps: ";
    for (int x : dt) cout << x << " ";  // 1 1 4 2 1 1 0 0
    cout << "\n";

    // Sum of subarray minimums
    vector<int> sm = {3, 1, 2, 4};
    cout << "Sum of subarray mins: " << sumSubarrayMins(sm) << "\n";  // 17

    // Sliding window max
    vector<int> sw = {1, 3, -1, -3, 5, 3, 6, 7};
    auto wmax = slidingWindowMax(sw, 3);
    cout << "Sliding window max (k=3): ";
    for (int x : wmax) cout << x << " ";  // 3 3 5 5 6 7
    cout << "\n";

    // Remove k digits
    cout << "Remove 2 digits from '1432219': "
         << removeKDigits("1432219", 3) << "\n";  // 1219

    // Stock span
    vector<int> prices = {100, 80, 60, 70, 60, 75, 85};
    auto span = stockSpan(prices);
    cout << "Stock span: ";
    for (int x : span) cout << x << " ";  // 1 1 1 2 1 4 6
    cout << "\n";

    // Asteroid collision
    vector<int> ast = {5, 10, -5};
    auto res = asteroidCollision(ast);
    cout << "Asteroid collision: ";
    for (int x : res) cout << x << " ";  // 5 10
    cout << "\n";

    return 0;
}
