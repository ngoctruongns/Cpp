/**
 * DYNAMIC PROGRAMMING – Part 1: Basics & Grid DP
 * ================================================
 * Core patterns:
 *   1. Top-down (memoization)  – recursive + cache
 *   2. Bottom-up (tabulation)  – iterative, usually faster
 *
 * Thinking steps:
 *   a. Define dp[state] = answer for that state
 *   b. Write recurrence (how to compute from smaller states)
 *   c. Identify base cases
 *   d. Determine evaluation order
 */

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
const int INF = 1e9;

// ─────────────────────────────────────────────
// 1. FIBONACCI  O(n)
// ─────────────────────────────────────────────
// dp[i] = dp[i-1] + dp[i-2]

// Top-down
vector<ll> memo_fib;
ll fib_memo(int n) {
    if (n <= 1) return n;
    if (memo_fib[n] != -1) return memo_fib[n];
    return memo_fib[n] = fib_memo(n - 1) + fib_memo(n - 2);
}

// Bottom-up (space optimized)
ll fib(int n) {
    if (n <= 1) return n;
    ll a = 0, b = 1;
    for (int i = 2; i <= n; i++) { ll c = a + b; a = b; b = c; }
    return b;
}

// ─────────────────────────────────────────────
// 2. CLIMBING STAIRS  O(n)
// ─────────────────────────────────────────────
// You can climb 1 or 2 steps. How many ways to reach step n?
// dp[i] = dp[i-1] + dp[i-2]  (same as Fibonacci!)
int climbStairs(int n) {
    if (n <= 2) return n;
    int a = 1, b = 2;
    for (int i = 3; i <= n; i++) { int c = a + b; a = b; b = c; }
    return b;
}

// ─────────────────────────────────────────────
// 3. COIN CHANGE – minimum coins  O(n * amount)
// ─────────────────────────────────────────────
// dp[j] = min coins to make amount j
int coinChange(vector<int>& coins, int amount) {
    vector<int> dp(amount + 1, INF);
    dp[0] = 0;
    for (int j = 1; j <= amount; j++) {
        for (int c : coins) {
            if (c <= j && dp[j - c] != INF)
                dp[j] = min(dp[j], dp[j - c] + 1);
        }
    }
    return dp[amount] == INF ? -1 : dp[amount];
}

// Coin change – number of ways  (unbounded knapsack variant)
int coinWays(vector<int>& coins, int amount) {
    vector<ll> dp(amount + 1, 0);
    dp[0] = 1;
    for (int c : coins)
        for (int j = c; j <= amount; j++)
            dp[j] += dp[j - c];
    return dp[amount];
}

// ─────────────────────────────────────────────
// 4. MAX SUBARRAY SUM (Kadane's)  O(n)
// ─────────────────────────────────────────────
// dp[i] = max subarray ending at i
int maxSubarray(const vector<int>& a) {
    int maxSum = a[0], cur = a[0];
    for (int i = 1; i < (int)a.size(); i++) {
        cur = max(a[i], cur + a[i]);
        maxSum = max(maxSum, cur);
    }
    return maxSum;
}

// With indices (start, end)
tuple<int,int,int> maxSubarrayWithIndex(const vector<int>& a) {
    int maxSum = a[0], cur = a[0];
    int start = 0, end = 0, tempStart = 0;
    for (int i = 1; i < (int)a.size(); i++) {
        if (a[i] > cur + a[i]) { cur = a[i]; tempStart = i; }
        else cur += a[i];
        if (cur > maxSum) { maxSum = cur; start = tempStart; end = i; }
    }
    return {maxSum, start, end};
}

// ─────────────────────────────────────────────
// 5. HOUSE ROBBER  O(n)
// ─────────────────────────────────────────────
// Cannot rob two adjacent houses. Max amount?
// dp[i] = max(dp[i-1], dp[i-2] + nums[i])
int houseRobber(const vector<int>& nums) {
    int n = nums.size();
    if (n == 0) return 0;
    if (n == 1) return nums[0];
    int prev2 = nums[0], prev1 = max(nums[0], nums[1]);
    for (int i = 2; i < n; i++) {
        int cur = max(prev1, prev2 + nums[i]);
        prev2 = prev1; prev1 = cur;
    }
    return prev1;
}

// ─────────────────────────────────────────────
// 6. GRID PATHS (2D DP)  O(m*n)
// ─────────────────────────────────────────────
// Count paths from (0,0) to (m-1,n-1) moving only right/down
int countPaths(int m, int n) {
    vector<vector<int>> dp(m, vector<int>(n, 0));
    for (int i = 0; i < m; i++) dp[i][0] = 1;
    for (int j = 0; j < n; j++) dp[0][j] = 1;
    for (int i = 1; i < m; i++)
        for (int j = 1; j < n; j++)
            dp[i][j] = dp[i-1][j] + dp[i][j-1];
    return dp[m-1][n-1];
}

// Minimum path sum in grid (move right/down)
int minPathSum(vector<vector<int>>& grid) {
    int m = grid.size(), n = grid[0].size();
    vector<vector<int>> dp(m, vector<int>(n, INF));
    dp[0][0] = grid[0][0];
    for (int i = 1; i < m; i++) dp[i][0] = dp[i-1][0] + grid[i][0];
    for (int j = 1; j < n; j++) dp[0][j] = dp[0][j-1] + grid[0][j];
    for (int i = 1; i < m; i++)
        for (int j = 1; j < n; j++)
            dp[i][j] = min(dp[i-1][j], dp[i][j-1]) + grid[i][j];
    return dp[m-1][n-1];
}

// ─────────────────────────────────────────────
// 7. JUMP GAME  O(n)
// ─────────────────────────────────────────────
// Can you reach the last index? nums[i] = max jump length from i
bool canJump(const vector<int>& nums) {
    int reach = 0;
    for (int i = 0; i < (int)nums.size(); i++) {
        if (i > reach) return false;
        reach = max(reach, i + nums[i]);
    }
    return true;
}

// Minimum jumps to reach end  O(n)
int jumpGame2(const vector<int>& nums) {
    int jumps = 0, curEnd = 0, farthest = 0;
    for (int i = 0; i < (int)nums.size() - 1; i++) {
        farthest = max(farthest, i + nums[i]);
        if (i == curEnd) { jumps++; curEnd = farthest; }
    }
    return jumps;
}

// ─────────────────────────────────────────────
// 8. MATRIX CHAIN MULTIPLICATION  O(n³)
// ─────────────────────────────────────────────
// Given dims[0..n] where matrix i has size dims[i]*dims[i+1],
// find min cost to multiply all matrices.
int matrixChain(const vector<int>& dims) {
    int n = dims.size() - 1;  // number of matrices
    vector<vector<int>> dp(n, vector<int>(n, 0));
    // dp[i][j] = min cost to multiply matrices i..j
    for (int len = 2; len <= n; len++) {
        for (int i = 0; i <= n - len; i++) {
            int j = i + len - 1;
            dp[i][j] = INF;
            for (int k = i; k < j; k++) {
                int cost = dp[i][k] + dp[k+1][j] + dims[i] * dims[k+1] * dims[j+1];
                dp[i][j] = min(dp[i][j], cost);
            }
        }
    }
    return dp[0][n-1];
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Decode ways: string of digits like "226" → "2 26"=BZ or "22 6"=VF or "2 2 6"=BBF
 *     Count number of decodings. (dp[i] = ways to decode s[0..i-1])
 *
 * [2] Unique paths with obstacles: same as countPaths but some cells blocked.
 *
 * [3] Pascal's Triangle: dp[i][j] = dp[i-1][j-1] + dp[i-1][j].
 *
 * [4] Perfect squares: least number of perfect squares summing to n.
 *     (similar to coin change, coins = [1,4,9,16,...])
 *
 * [5] House robber on circular array (first and last house are adjacent).
 *     Hint: solve twice – exclude first, exclude last. Take max.
 */

// Exercise [1] – Decode Ways
int numDecodings(const string& s) {
    int n = s.size();
    if (s[0] == '0') return 0;
    vector<int> dp(n + 1, 0);
    dp[0] = 1; dp[1] = 1;
    for (int i = 2; i <= n; i++) {
        int one = s[i-1] - '0';
        int two = stoi(s.substr(i-2, 2));
        if (one >= 1) dp[i] += dp[i-1];
        if (two >= 10 && two <= 26) dp[i] += dp[i-2];
    }
    return dp[n];
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Fibonacci
    memo_fib.assign(50, -1);
    cout << "fib(10): " << fib_memo(10) << "\n";  // 55
    cout << "fib(10): " << fib(10) << "\n";        // 55

    // Climbing stairs
    cout << "Stairs(5): " << climbStairs(5) << "\n";  // 8

    // Coin change
    vector<int> coins = {1, 5, 6, 9};
    cout << "Min coins for 11: " << coinChange(coins, 11) << "\n";  // 2 (5+6)
    vector<int> c2 = {1, 2, 5};
    cout << "Ways to make 5: " << coinWays(c2, 5) << "\n";  // 4

    // Max subarray
    vector<int> arr = {-2, 1, -3, 4, -1, 2, 1, -5, 4};
    cout << "Max subarray: " << maxSubarray(arr) << "\n";  // 6

    // House robber
    vector<int> houses = {2, 7, 9, 3, 1};
    cout << "Max rob: " << houseRobber(houses) << "\n";  // 12

    // Grid paths
    cout << "Paths 3x3: " << countPaths(3, 3) << "\n";  // 6

    // Min path sum
    vector<vector<int>> grid = {{1,3,1},{1,5,1},{4,2,1}};
    cout << "Min path: " << minPathSum(grid) << "\n";  // 7

    // Jump game
    vector<int> jumps = {2, 3, 1, 1, 4};
    cout << "Can jump: " << canJump(jumps) << "\n";        // 1
    cout << "Min jumps: " << jumpGame2(jumps) << "\n";     // 2

    // Matrix chain
    vector<int> dims = {10, 30, 5, 60};
    cout << "Matrix chain cost: " << matrixChain(dims) << "\n";  // 4500

    // Decode ways
    cout << "Decode '226': " << numDecodings("226") << "\n";  // 3

    return 0;
}
