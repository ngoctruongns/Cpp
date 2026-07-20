/**
 * STRING ALGORITHMS
 * ==================
 * KMP, Z-function, Rabin-Karp hashing.
 *
 * Pattern matching complexity:
 *   Naive:       O(n*m)
 *   KMP:         O(n + m)
 *   Z-function:  O(n + m)
 *   Rabin-Karp:  O(n + m) average, O(n*m) worst (hash collision)
 */

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

// ─────────────────────────────────────────────
// 1. KMP – Knuth-Morris-Pratt  O(n + m)
// ─────────────────────────────────────────────
// Builds "failure function" (partial match table):
// fail[i] = length of longest proper prefix of pattern[0..i] that is also a suffix.

vector<int> buildKMPFail(const string& pattern) {
    int m = pattern.size();
    vector<int> fail(m, 0);
    int j = 0;
    for (int i = 1; i < m; i++) {
        while (j > 0 && pattern[i] != pattern[j]) j = fail[j - 1];
        if (pattern[i] == pattern[j]) j++;
        fail[i] = j;
    }
    return fail;
}

// Returns all starting indices (0-based) where pattern occurs in text.
vector<int> kmpSearch(const string& text, const string& pattern) {
    int n = text.size(), m = pattern.size();
    if (m == 0) return {};
    auto fail = buildKMPFail(pattern);
    vector<int> matches;
    int j = 0;
    for (int i = 0; i < n; i++) {
        while (j > 0 && text[i] != pattern[j]) j = fail[j - 1];
        if (text[i] == pattern[j]) j++;
        if (j == m) {
            matches.push_back(i - m + 1);
            j = fail[j - 1];  // continue searching
        }
    }
    return matches;
}

// Count occurrences
int countOccurrences(const string& text, const string& pattern) {
    return kmpSearch(text, pattern).size();
}

// ─────────────────────────────────────────────
// 2. Z-FUNCTION  O(n)
// ─────────────────────────────────────────────
// z[i] = length of longest substring starting from s[i] that matches a prefix of s.
// z[0] = 0 by convention.
//
// To find pattern in text: compute Z for (pattern + '$' + text)
// Any i in the text part with z[i] == m means pattern starts at (i - m - 1).

vector<int> zFunction(const string& s) {
    int n = s.size();
    vector<int> z(n, 0);
    int l = 0, r = 0;
    for (int i = 1; i < n; i++) {
        if (i < r) z[i] = min(r - i, z[i - l]);
        while (i + z[i] < n && s[z[i]] == s[i + z[i]]) z[i]++;
        if (i + z[i] > r) { l = i; r = i + z[i]; }
    }
    return z;
}

vector<int> zSearch(const string& text, const string& pattern) {
    string combined = pattern + "$" + text;
    auto z = zFunction(combined);
    int m = pattern.size(), offset = m + 1;
    vector<int> matches;
    for (int i = offset; i < (int)combined.size(); i++)
        if (z[i] == m) matches.push_back(i - offset);
    return matches;
}

// ─────────────────────────────────────────────
// 3. RABIN-KARP – rolling hash  O(n + m) average
// ─────────────────────────────────────────────
// Good for multiple pattern matching or 2D pattern matching.
const ll BASE = 31;
const ll MOD1 = 1e9 + 7;
const ll MOD2 = 1e9 + 9;  // double hashing to reduce collisions

// Compute polynomial hash of string
pair<ll,ll> strHash(const string& s) {
    ll h1 = 0, h2 = 0, pw1 = 1, pw2 = 1;
    for (char c : s) {
        h1 = (h1 * BASE + (c - 'a' + 1)) % MOD1;
        h2 = (h2 * BASE + (c - 'a' + 1)) % MOD2;
    }
    return {h1, h2};
}

vector<int> rabinKarp(const string& text, const string& pattern) {
    int n = text.size(), m = pattern.size();
    if (m > n) return {};

    // Precompute powers
    ll pw1 = 1, pw2 = 1;
    for (int i = 0; i < m - 1; i++) {
        pw1 = pw1 * BASE % MOD1;
        pw2 = pw2 * BASE % MOD2;
    }

    auto [ph1, ph2] = strHash(pattern);
    ll h1 = 0, h2 = 0;
    vector<int> matches;

    for (int i = 0; i < n; i++) {
        h1 = (h1 * BASE + (text[i] - 'a' + 1)) % MOD1;
        h2 = (h2 * BASE + (text[i] - 'a' + 1)) % MOD2;

        if (i >= m) {  // remove leftmost character
            h1 = (h1 - pw1 * (text[i-m] - 'a' + 1) % MOD1 + MOD1) % MOD1;
            h2 = (h2 - pw2 * (text[i-m] - 'a' + 1) % MOD2 + MOD2) % MOD2;
        }

        if (i >= m - 1 && h1 == ph1 && h2 == ph2) {
            // Verify to avoid hash collision
            if (text.substr(i - m + 1, m) == pattern)
                matches.push_back(i - m + 1);
        }
    }
    return matches;
}

// ─────────────────────────────────────────────
// 4. STRING HASHING for arbitrary comparisons
// ─────────────────────────────────────────────
struct StringHash {
    string s;
    vector<ll> h, pw;
    ll base, mod;

    StringHash(const string& s, ll base = 131, ll mod = 1e9 + 7)
        : s(s), base(base), mod(mod), h(s.size() + 1, 0), pw(s.size() + 1, 1) {
        for (int i = 0; i < (int)s.size(); i++) {
            h[i+1] = (h[i] * base + s[i]) % mod;
            pw[i+1] = pw[i] * base % mod;
        }
    }

    // Hash of s[l..r] (0-indexed, inclusive)
    ll get(int l, int r) {
        return (h[r+1] - h[l] * pw[r-l+1] % mod + mod * mod) % mod;
    }

    // Compare substrings s[l1..r1] == s[l2..r2]
    bool equal(int l1, int r1, int l2, int r2) {
        return get(l1, r1) == get(l2, r2);
    }
};

// ─────────────────────────────────────────────
// 5. USEFUL STRING TRICKS
// ─────────────────────────────────────────────

// Minimum rotation of string (lexicographically smallest)
// Booth's algorithm O(n)
string minRotation(const string& s) {
    string ss = s + s;
    int n = s.size();
    int f[2*n];
    fill(f, f + 2*n, -1);
    int k = 0;
    for (int j = 1; j < 2*n; j++) {
        int i = f[j - k - 1];
        while (i != -1 && ss[j] != ss[k + i + 1]) {
            if (ss[j] < ss[k + i + 1]) k = j - i - 1;
            i = f[i];
        }
        if (ss[j] != ss[k + i + 1]) {
            if (ss[j] < ss[k]) k = j;
            f[j - k] = -1;
        } else f[j - k] = i + 1;
    }
    return s.substr(k) + s.substr(0, k);
}

// Check if s is rotation of t
bool isRotation(const string& s, const string& t) {
    if (s.size() != t.size()) return false;
    return !kmpSearch(s + s, t).empty();
}

// Longest repeated substring using hashing + binary search  O(n log n)
string longestRepeatedSubstring(const string& s) {
    int n = s.size(), lo = 0, hi = n;
    string result = "";
    while (lo < hi) {
        int mid = lo + (hi - lo + 1) / 2;
        // Check if any substring of length mid appears twice
        unordered_set<ll> seen;
        StringHash sh(s);
        bool found = false;
        string candidate;
        for (int i = 0; i + mid <= n; i++) {
            ll h = sh.get(i, i + mid - 1);
            if (seen.count(h)) { found = true; candidate = s.substr(i, mid); break; }
            seen.insert(h);
        }
        if (found) { lo = mid; result = candidate; }
        else hi = mid - 1;
    }
    return result;
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Find the smallest period of a string.
 *     Period p: s[i] == s[i % p] for all i.
 *     Using KMP fail: if (n % (n - fail[n-1])) == 0, then period = n - fail[n-1]
 *
 * [2] String compression: "aaabcc" → "a3bc2" (only compress if shorter).
 *
 * [3] Anagram in string: find all starting indices where p's anagram appears in s.
 *     (Sliding window + frequency count)
 *
 * [4] Repeated DNA sequences: find all 10-letter sequences occurring more than once.
 *     (Rolling hash or substring hashing)
 *
 * [5] Longest duplicate substring. (Binary search + rolling hash)
 */

// Exercise [1] – Smallest period
int smallestPeriod(const string& s) {
    int n = s.size();
    auto fail = buildKMPFail(s);
    int period = n - fail[n - 1];
    return (n % period == 0) ? period : n;
}

// Exercise [3] – Find all anagram positions
vector<int> findAnagrams(const string& s, const string& p) {
    int ns = s.size(), np = p.size();
    if (np > ns) return {};
    vector<int> cnt(26, 0), window(26, 0);
    for (char c : p) cnt[c - 'a']++;
    for (int i = 0; i < np; i++) window[s[i] - 'a']++;
    vector<int> result;
    if (window == cnt) result.push_back(0);
    for (int i = np; i < ns; i++) {
        window[s[i] - 'a']++;
        window[s[i - np] - 'a']--;
        if (window == cnt) result.push_back(i - np + 1);
    }
    return result;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // KMP
    string text = "AABAACAADAABAABA", pattern = "AABA";
    auto matches = kmpSearch(text, pattern);
    cout << "KMP matches of '" << pattern << "' in text: ";
    for (int i : matches) cout << i << " ";  // 0 9 12
    cout << "\n";

    // Failure function
    auto fail = buildKMPFail("AAACAAAA");
    cout << "KMP fail: ";
    for (int f : fail) cout << f << " ";  // 0 1 2 0 1 2 3 3
    cout << "\n";

    // Z-function
    auto z = zFunction("aabxaa");
    cout << "Z-function: ";
    for (int zi : z) cout << zi << " ";  // 0 1 0 0 2 1
    cout << "\n";

    // Z-search
    auto zm = zSearch("aabxaa", "aa");
    cout << "Z-search matches: ";
    for (int i : zm) cout << i << " ";  // 0 4
    cout << "\n";

    // Rabin-Karp
    auto rkm = rabinKarp("abcabcabc", "abc");
    cout << "Rabin-Karp matches: ";
    for (int i : rkm) cout << i << " ";  // 0 3 6
    cout << "\n";

    // String hashing comparison
    string s = "abcabc";
    StringHash sh(s);
    cout << "s[0..2] == s[3..5]: " << sh.equal(0, 2, 3, 5) << "\n";  // 1
    cout << "s[0..2] == s[1..3]: " << sh.equal(0, 2, 1, 3) << "\n";  // 0

    // Rotation
    cout << "isRotation(abcde, cdeab): " << isRotation("abcde", "cdeab") << "\n";  // 1
    cout << "Min rotation of 'dcba': " << minRotation("dcba") << "\n";  // abcd

    // Smallest period
    cout << "Period of 'abababab': " << smallestPeriod("abababab") << "\n";  // 2
    cout << "Period of 'abcabc': " << smallestPeriod("abcabc") << "\n";  // 3

    // Find anagrams
    auto anagrams = findAnagrams("cbaebabacd", "abc");
    cout << "Anagram positions of 'abc' in 'cbaebabacd': ";
    for (int i : anagrams) cout << i << " ";  // 0 6
    cout << "\n";

    return 0;
}
