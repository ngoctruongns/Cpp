/**
 * DYNAMIC PROGRAMMING – Part 2: Knapsack Variants
 * =================================================
 * Classic knapsack problems – most frequent in competitions.
 *
 * Variants:
 *   0/1 Knapsack  – each item can be taken once
 *   Unbounded     – each item unlimited times
 *   Bounded       – each item up to k times
 *   Fractional    – Greedy (NOT DP), take partial items
 *   Multi-dim     – two constraints (weight + volume)
 */

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

// ─────────────────────────────────────────────
// 1. 0/1 KNAPSACK  O(n * W)
// ─────────────────────────────────────────────
// n items, each with weight w[i] and value v[i].
// Knapsack capacity W. Maximize total value.
//
// dp[j] = max value using capacity j
// Space-optimized: iterate j from W down to w[i]
int knapsack01(const vector<int>& w, const vector<int>& v, int W) {
    int n = w.size();
    vector<int> dp(W + 1, 0);
    for (int i = 0; i < n; i++)
        for (int j = W; j >= w[i]; j--)  // MUST iterate backward for 0/1
            dp[j] = max(dp[j], dp[j - w[i]] + v[i]);
    return dp[W];
}

// With item tracking (which items were selected)
pair<int, vector<int>> knapsack01Track(const vector<int>& w, const vector<int>& v, int W) {
    int n = w.size();
    vector<vector<int>> dp(n + 1, vector<int>(W + 1, 0));
    for (int i = 1; i <= n; i++)
        for (int j = 0; j <= W; j++) {
            dp[i][j] = dp[i-1][j];
            if (w[i-1] <= j)
                dp[i][j] = max(dp[i][j], dp[i-1][j - w[i-1]] + v[i-1]);
        }
    // Backtrack to find selected items
    vector<int> selected;
    int j = W;
    for (int i = n; i >= 1; i--) {
        if (dp[i][j] != dp[i-1][j]) {
            selected.push_back(i - 1);
            j -= w[i-1];
        }
    }
    return {dp[n][W], selected};
}

// ─────────────────────────────────────────────
// 2. UNBOUNDED KNAPSACK  O(n * W)
// ─────────────────────────────────────────────
// Same as 0/1 but each item can be used unlimited times.
// Iterate j FORWARD (allows reuse).
int knapsackUnbounded(const vector<int>& w, const vector<int>& v, int W) {
    int n = w.size();
    vector<int> dp(W + 1, 0);
    for (int i = 0; i < n; i++)
        for (int j = w[i]; j <= W; j++)  // MUST iterate forward for unbounded
            dp[j] = max(dp[j], dp[j - w[i]] + v[i]);
    return dp[W];
}

// ─────────────────────────────────────────────
// 3. BOUNDED KNAPSACK  O(n * W * log(maxK))
// ─────────────────────────────────────────────
// Each item i can be taken at most cnt[i] times.
// Binary grouping trick: split cnt into groups of 1,2,4,8,...
int knapsackBounded(const vector<int>& w, const vector<int>& v,
                    const vector<int>& cnt, int W) {
    int n = w.size();
    vector<int> dp(W + 1, 0);

    for (int i = 0; i < n; i++) {
        // Binary grouping of item i
        int k = cnt[i];
        for (int group = 1; k > 0; group *= 2) {
            int take = min(group, k);
            k -= take;
            // Now treat (take copies of item i) as a single new item
            int newW = take * w[i];
            int newV = take * v[i];
            // 0/1 knapsack step for this grouped item
            for (int j = W; j >= newW; j--)
                dp[j] = max(dp[j], dp[j - newW] + newV);
        }
    }
    return dp[W];
}

// ─────────────────────────────────────────────
// 4. SUBSET SUM  O(n * target)
// ─────────────────────────────────────────────
// Can we pick a subset that sums to target?
// Special case of 0/1 knapsack (v[i] = w[i] = nums[i])
bool subsetSum(const vector<int>& nums, int target) {
    vector<bool> dp(target + 1, false);
    dp[0] = true;
    for (int x : nums)
        for (int j = target; j >= x; j--)
            dp[j] = dp[j] || dp[j - x];
    return dp[target];
}

// Count subsets with given sum
int countSubsets(const vector<int>& nums, int target) {
    vector<int> dp(target + 1, 0);
    dp[0] = 1;
    for (int x : nums)
        for (int j = target; j >= x; j--)
            dp[j] += dp[j - x];
    return dp[target];
}

// ─────────────────────────────────────────────
// 5. PARTITION EQUAL SUBSET SUM  O(n * sum/2)
// ─────────────────────────────────────────────
// Can we split array into two subsets with equal sum?
bool canPartition(const vector<int>& nums) {
    int total = 0;
    for (int x : nums) total += x;
    if (total % 2 != 0) return false;
    return subsetSum(nums, total / 2);
}

// ─────────────────────────────────────────────
// 6. PARTITION INTO K EQUAL SUBSETS  O(2^n * n)
// ─────────────────────────────────────────────
// Bitmask DP – each bit represents whether item is used
bool partitionKSubsets(vector<int>& nums, int k) {
    int total = 0;
    for (int x : nums) total += x;
    if (total % k != 0) return false;
    int target = total / k;
    int n = nums.size();
    vector<int> dp(1 << n, -1);  // -1 = not visited
    dp[0] = 0;
    // dp[mask] = current sum in the current group (mod target)
    for (int mask = 0; mask < (1 << n); mask++) {
        if (dp[mask] == -1) continue;
        for (int i = 0; i < n; i++) {
            if (mask & (1 << i)) continue;  // already used
            int next = mask | (1 << i);
            if (dp[mask] + nums[i] <= target)
                dp[next] = (dp[mask] + nums[i]) % target;
        }
    }
    return dp[(1 << n) - 1] == 0;
}

// ─────────────────────────────────────────────
// 7. TARGET SUM (+ or - signs)  O(n * sum)
// ─────────────────────────────────────────────
// Assign + or - to each element to reach target.
// Count number of ways.
// Math insight: P - N = target, P + N = sum → P = (sum + target) / 2
int findTargetSumWays(const vector<int>& nums, int target) {
    int sum = 0;
    for (int x : nums) sum += x;
    if ((sum + target) % 2 != 0 || sum + target < 0) return 0;
    int s = (sum + target) / 2;
    return countSubsets(nums, s);
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Last stone weight II: split array into two groups minimizing |sum1 - sum2|.
 *     (Hint: find largest P <= total/2 using subset sum)
 *
 * [2] Minimum number of elements to cover sum S.
 *     (Unbounded knapsack where v[i]=1, minimize total)
 *
 * [3] Rod cutting: rod of length n, cut into pieces with prices p[].
 *     Maximize revenue. (Unbounded knapsack)
 *
 * [4] 2D knapsack: each item has weight AND volume. Two constraints.
 *     dp[j][k] = max value with weight capacity j and volume k.
 *
 * [5] Knapsack with dependency: item i can only be taken if parent[i] is taken.
 *     (Tree DP variant – group knapsack)
 */

// Exercise [1] – Last stone weight II
int lastStoneWeightII(const vector<int>& stones) {
    int total = 0;
    for (int x : stones) total += x;
    int half = total / 2;
    vector<int> dp(half + 1, 0);
    for (int x : stones)
        for (int j = half; j >= x; j--)
            dp[j] = max(dp[j], dp[j - x] + x);
    return total - 2 * dp[half];
}

// Exercise [3] – Rod cutting (unbounded knapsack)
int rodCutting(const vector<int>& prices, int n) {
    // prices[i] = value of piece of length i+1
    vector<int> dp(n + 1, 0);
    for (int len = 1; len <= n; len++)
        for (int j = len; j <= n; j++)
            dp[j] = max(dp[j], dp[j - len] + prices[len - 1]);
    return dp[n];
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // 0/1 Knapsack
    vector<int> w = {2, 3, 4, 5};
    vector<int> v = {3, 4, 5, 6};
    cout << "0/1 Knapsack (W=5): " << knapsack01(w, v, 5) << "\n";  // 7

    auto [val, items] = knapsack01Track(w, v, 5);
    cout << "Selected items: ";
    for (int i : items) cout << i << " ";
    cout << "\n";

    // Unbounded knapsack
    cout << "Unbounded (W=5): " << knapsackUnbounded(w, v, 5) << "\n";

    // Bounded knapsack
    vector<int> cnt = {1, 2, 3, 2};
    cout << "Bounded (W=10): " << knapsackBounded(w, v, cnt, 10) << "\n";

    // Subset sum
    vector<int> nums = {3, 34, 4, 12, 5, 2};
    cout << "Subset sum 9: " << subsetSum(nums, 9) << "\n";  // 1 (4+5)
    cout << "Count subsets 9: " << countSubsets(nums, 9) << "\n";

    // Partition
    vector<int> part = {1, 5, 11, 5};
    cout << "Can partition: " << canPartition(part) << "\n";  // 1 (1+5+5 = 11)

    // Target sum
    vector<int> ts = {1, 1, 1, 1, 1};
    cout << "Target sum ways (target=3): " << findTargetSumWays(ts, 3) << "\n";  // 5

    // Last stone weight II
    vector<int> stones = {2, 7, 4, 1, 8, 1};
    cout << "Last stone II: " << lastStoneWeightII(stones) << "\n";  // 1

    // Rod cutting
    vector<int> prices = {1, 5, 8, 9, 10, 17, 17, 20};
    cout << "Rod cutting (n=8): " << rodCutting(prices, 8) << "\n";  // 22

    return 0;
}
