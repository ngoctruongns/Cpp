/**
 * DYNAMIC PROGRAMMING – Part 3: Subsequence Problems
 * ====================================================
 * Classic string/array DP problems.
 *
 * Key problems:
 *   LIS  – Longest Increasing Subsequence   O(n log n)
 *   LCS  – Longest Common Subsequence       O(mn)
 *   SCS  – Shortest Common Supersequence    O(mn)
 *   Edit Distance                           O(mn)
 *   Palindrome subsequence/substring        O(n²)
 */

#include <bits/stdc++.h>
using namespace std;

// ─────────────────────────────────────────────
// 1. LIS – Longest Increasing Subsequence
// ─────────────────────────────────────────────

// O(n²) DP version
int lis_dp(const vector<int>& a) {
    int n = a.size();
    vector<int> dp(n, 1);  // dp[i] = LIS ending at index i
    for (int i = 1; i < n; i++)
        for (int j = 0; j < i; j++)
            if (a[j] < a[i]) dp[i] = max(dp[i], dp[j] + 1);
    return *max_element(dp.begin(), dp.end());
}

// O(n log n) using patience sorting (binary search)
// "tails[i]" = smallest tail of all increasing subsequences of length i+1
int lis(const vector<int>& a) {
    vector<int> tails;
    for (int x : a) {
        auto it = lower_bound(tails.begin(), tails.end(), x);  // strict increase
        // use upper_bound for non-decreasing (allow equal)
        if (it == tails.end()) tails.push_back(x);
        else *it = x;
    }
    return tails.size();
}

// Reconstruct LIS  O(n log n)
vector<int> lisReconstruct(const vector<int>& a) {
    int n = a.size();
    vector<int> tails, parent(n, -1), pos(n);
    vector<int> tailIdx;  // tail indices in original array

    for (int i = 0; i < n; i++) {
        auto it = lower_bound(tails.begin(), tails.end(), a[i]);
        int p = it - tails.begin();
        if (it == tails.end()) { tails.push_back(a[i]); tailIdx.push_back(i); }
        else { tails[p] = a[i]; tailIdx[p] = i; }
        pos[i] = p;
        if (p > 0) parent[i] = tailIdx[p - 1];
    }

    // Backtrack
    int idx = tailIdx[tails.size() - 1];
    vector<int> result;
    while (idx != -1) { result.push_back(a[idx]); idx = parent[idx]; }
    reverse(result.begin(), result.end());
    return result;
}

// ─────────────────────────────────────────────
// 2. LCS – Longest Common Subsequence  O(mn)
// ─────────────────────────────────────────────
int lcs(const string& s, const string& t) {
    int m = s.size(), n = t.size();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++) {
            if (s[i-1] == t[j-1]) dp[i][j] = dp[i-1][j-1] + 1;
            else dp[i][j] = max(dp[i-1][j], dp[i][j-1]);
        }
    return dp[m][n];
}

// Reconstruct LCS
string lcsString(const string& s, const string& t) {
    int m = s.size(), n = t.size();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++) {
            if (s[i-1] == t[j-1]) dp[i][j] = dp[i-1][j-1] + 1;
            else dp[i][j] = max(dp[i-1][j], dp[i][j-1]);
        }
    string result;
    int i = m, j = n;
    while (i > 0 && j > 0) {
        if (s[i-1] == t[j-1]) { result += s[i-1]; i--; j--; }
        else if (dp[i-1][j] > dp[i][j-1]) i--;
        else j--;
    }
    reverse(result.begin(), result.end());
    return result;
}

// ─────────────────────────────────────────────
// 3. EDIT DISTANCE (Levenshtein)  O(mn)
// ─────────────────────────────────────────────
// Min operations (insert, delete, replace) to convert s → t
int editDistance(const string& s, const string& t) {
    int m = s.size(), n = t.size();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));
    for (int i = 0; i <= m; i++) dp[i][0] = i;  // delete all of s
    for (int j = 0; j <= n; j++) dp[0][j] = j;  // insert all of t
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++) {
            if (s[i-1] == t[j-1]) dp[i][j] = dp[i-1][j-1];
            else dp[i][j] = 1 + min({dp[i-1][j-1],  // replace
                                     dp[i-1][j],      // delete
                                     dp[i][j-1]});    // insert
        }
    return dp[m][n];
}

// ─────────────────────────────────────────────
// 4. SHORTEST COMMON SUPERSEQUENCE  O(mn)
// ─────────────────────────────────────────────
// SCS length = m + n - LCS(s,t)
// Construct SCS string
string shortestCommonSupersequence(const string& s, const string& t) {
    int m = s.size(), n = t.size();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++) {
            if (s[i-1] == t[j-1]) dp[i][j] = dp[i-1][j-1] + 1;
            else dp[i][j] = max(dp[i-1][j], dp[i][j-1]);
        }
    string result;
    int i = m, j = n;
    while (i > 0 && j > 0) {
        if (s[i-1] == t[j-1]) { result += s[i-1]; i--; j--; }
        else if (dp[i-1][j] > dp[i][j-1]) { result += s[i-1]; i--; }
        else { result += t[j-1]; j--; }
    }
    while (i > 0) { result += s[i-1]; i--; }
    while (j > 0) { result += t[j-1]; j--; }
    reverse(result.begin(), result.end());
    return result;
}

// ─────────────────────────────────────────────
// 5. LONGEST PALINDROMIC SUBSEQUENCE  O(n²)
// ─────────────────────────────────────────────
// = LCS(s, reverse(s))
int longestPalindromicSubseq(const string& s) {
    string rev = s;
    reverse(rev.begin(), rev.end());
    return lcs(s, rev);
}

// ─────────────────────────────────────────────
// 6. LONGEST PALINDROMIC SUBSTRING  O(n²)
// ─────────────────────────────────────────────
// Expand around center
string longestPalindromeSubstring(const string& s) {
    int n = s.size(), start = 0, maxLen = 1;
    auto expand = [&](int l, int r) {
        while (l >= 0 && r < n && s[l] == s[r]) { l--; r++; }
        if (r - l - 1 > maxLen) { maxLen = r - l - 1; start = l + 1; }
    };
    for (int i = 0; i < n; i++) { expand(i, i); expand(i, i+1); }
    return s.substr(start, maxLen);
}

// ─────────────────────────────────────────────
// 7. COUNT PALINDROMIC SUBSTRINGS  O(n²)
// ─────────────────────────────────────────────
int countPalindromicSubstrings(const string& s) {
    int n = s.size(), count = 0;
    auto expand = [&](int l, int r) {
        while (l >= 0 && r < n && s[l] == s[r]) { l--; r++; count++; }
    };
    for (int i = 0; i < n; i++) { expand(i, i); expand(i, i+1); }
    return count;
}

// ─────────────────────────────────────────────
// 8. DISTINCT SUBSEQUENCES  O(mn)
// ─────────────────────────────────────────────
// Count distinct subsequences of s that equal t.
int distinctSubsequences(const string& s, const string& t) {
    int m = s.size(), n = t.size();
    vector<vector<long long>> dp(m + 1, vector<long long>(n + 1, 0));
    for (int i = 0; i <= m; i++) dp[i][0] = 1;  // empty t is always subseq
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++) {
            dp[i][j] = dp[i-1][j];  // don't use s[i-1]
            if (s[i-1] == t[j-1]) dp[i][j] += dp[i-1][j-1];
        }
    return dp[m][n];
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Maximum sum increasing subsequence: LIS where we maximize sum, not length.
 *
 * [2] Number of LIS: count how many subsequences achieve the LIS length.
 *
 * [3] Interleaving strings: check if s3 is interleaving of s1 and s2.
 *     dp[i][j] = can s3[0..i+j-1] be formed by interleaving s1[0..i-1] and s2[0..j-1]
 *
 * [4] Wildcard matching: '?' matches any single char, '*' matches any sequence.
 *
 * [5] Regular expression matching: '.' matches any single, '*' matches 0+ of previous.
 */

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // LIS
    vector<int> a = {10, 9, 2, 5, 3, 7, 101, 18};
    cout << "LIS O(n²): " << lis_dp(a) << "\n";   // 4
    cout << "LIS O(nlogn): " << lis(a) << "\n";    // 4

    auto seq = lisReconstruct(a);
    cout << "LIS sequence: ";
    for (int x : seq) cout << x << " ";
    cout << "\n";  // 2 3 7 18 (or 2 5 7 18)

    // LCS
    string s = "ABCBDAB", t = "BDCAB";
    cout << "LCS length: " << lcs(s, t) << "\n";          // 4
    cout << "LCS string: " << lcsString(s, t) << "\n";    // BCAB or BDAB

    // Edit distance
    cout << "Edit distance (horse→ros): " << editDistance("horse", "ros") << "\n";  // 3

    // SCS
    cout << "SCS (abac, cab): " << shortestCommonSupersequence("abac", "cab") << "\n";

    // Palindromes
    cout << "Longest palindromic subseq (bbbab): "
         << longestPalindromicSubseq("bbbab") << "\n";  // 4

    cout << "Longest palindromic substring (babad): "
         << longestPalindromeSubstring("babad") << "\n";  // bab or aba

    cout << "Count palindromic substrings (abc): "
         << countPalindromicSubstrings("abc") << "\n";  // 3

    // Distinct subsequences
    cout << "Distinct subseq (rabbbit, rabbit): "
         << distinctSubsequences("rabbbit", "rabbit") << "\n";  // 3

    return 0;
}
