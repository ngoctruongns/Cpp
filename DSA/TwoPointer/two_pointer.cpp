/**
 * TWO POINTER & SLIDING WINDOW
 * =============================
 * Technique to reduce O(n²) brute force to O(n).
 *
 * Two Pointer: two indices moving toward each other or same direction.
 * Sliding Window: maintain a window [l, r] over array/string.
 *
 * Pattern recognition:
 *   "subarray/substring with condition" → Sliding window
 *   "pair in sorted array" → Two pointer
 *   "partition/rearrange" → Two pointer
 */

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;

// ─────────────────────────────────────────────
// TWO POINTER
// ─────────────────────────────────────────────

// 1. TWO SUM in sorted array  O(n)
// Find pair summing to target.
pair<int,int> twoSum(const vector<int>& a, int target) {
    int l = 0, r = a.size() - 1;
    while (l < r) {
        int sum = a[l] + a[r];
        if (sum == target) return {l, r};
        else if (sum < target) l++;
        else r--;
    }
    return {-1, -1};
}

// 2. THREE SUM  O(n²)
// Find all unique triplets summing to 0.
vector<vector<int>> threeSum(vector<int> nums) {
    sort(nums.begin(), nums.end());
    int n = nums.size();
    vector<vector<int>> result;
    for (int i = 0; i < n - 2; i++) {
        if (i > 0 && nums[i] == nums[i-1]) continue;  // skip duplicates
        int l = i + 1, r = n - 1;
        while (l < r) {
            int sum = nums[i] + nums[l] + nums[r];
            if (sum == 0) {
                result.push_back({nums[i], nums[l], nums[r]});
                while (l < r && nums[l] == nums[l+1]) l++;
                while (l < r && nums[r] == nums[r-1]) r--;
                l++; r--;
            } else if (sum < 0) l++;
            else r--;
        }
    }
    return result;
}

// 3. CONTAINER WITH MOST WATER  O(n)
// Heights h[], find two lines forming container with max water.
int maxWater(const vector<int>& h) {
    int l = 0, r = h.size() - 1, maxVol = 0;
    while (l < r) {
        maxVol = max(maxVol, min(h[l], h[r]) * (r - l));
        if (h[l] < h[r]) l++;
        else r--;
    }
    return maxVol;
}

// 4. TRAPPING RAIN WATER  O(n)
int trapRain(const vector<int>& h) {
    int l = 0, r = h.size() - 1, leftMax = 0, rightMax = 0, water = 0;
    while (l < r) {
        if (h[l] < h[r]) {
            if (h[l] >= leftMax) leftMax = h[l];
            else water += leftMax - h[l];
            l++;
        } else {
            if (h[r] >= rightMax) rightMax = h[r];
            else water += rightMax - h[r];
            r--;
        }
    }
    return water;
}

// 5. REMOVE DUPLICATES IN-PLACE  O(n)
// Returns new length; modifies array in place.
int removeDuplicates(vector<int>& a) {
    if (a.empty()) return 0;
    int slow = 0;
    for (int fast = 1; fast < (int)a.size(); fast++)
        if (a[fast] != a[slow]) a[++slow] = a[fast];
    return slow + 1;
}

// 6. SORT COLORS (Dutch flag)  O(n)
// Partition array into 0s, 1s, 2s in one pass.
void sortColors(vector<int>& nums) {
    int lo = 0, mid = 0, hi = nums.size() - 1;
    while (mid <= hi) {
        if (nums[mid] == 0) swap(nums[lo++], nums[mid++]);
        else if (nums[mid] == 1) mid++;
        else swap(nums[mid], nums[hi--]);
    }
}

// ─────────────────────────────────────────────
// SLIDING WINDOW
// ─────────────────────────────────────────────

// 7. MAX SUM SUBARRAY OF SIZE K  O(n)
int maxSumSubarrayK(const vector<int>& a, int k) {
    int n = a.size();
    int windowSum = 0, maxSum = INT_MIN;
    for (int i = 0; i < n; i++) {
        windowSum += a[i];
        if (i >= k) windowSum -= a[i - k];
        if (i >= k - 1) maxSum = max(maxSum, windowSum);
    }
    return maxSum;
}

// 8. LONGEST SUBSTRING WITHOUT REPEATING CHARS  O(n)
int lengthOfLongestSubstring(const string& s) {
    unordered_map<char, int> lastSeen;
    int maxLen = 0, l = 0;
    for (int r = 0; r < (int)s.size(); r++) {
        if (lastSeen.count(s[r]) && lastSeen[s[r]] >= l)
            l = lastSeen[s[r]] + 1;
        lastSeen[s[r]] = r;
        maxLen = max(maxLen, r - l + 1);
    }
    return maxLen;
}

// 9. MINIMUM WINDOW SUBSTRING  O(n + m)
// Find minimum window in s containing all chars of t.
string minWindow(const string& s, const string& t) {
    unordered_map<char, int> need, window;
    for (char c : t) need[c]++;
    int have = 0, required = need.size();
    int minLen = INT_MAX, start = 0;
    int l = 0;
    for (int r = 0; r < (int)s.size(); r++) {
        window[s[r]]++;
        if (need.count(s[r]) && window[s[r]] == need[s[r]]) have++;
        while (have == required) {
            if (r - l + 1 < minLen) { minLen = r - l + 1; start = l; }
            window[s[l]]--;
            if (need.count(s[l]) && window[s[l]] < need[s[l]]) have--;
            l++;
        }
    }
    return minLen == INT_MAX ? "" : s.substr(start, minLen);
}

// 10. LONGEST SUBARRAY WITH SUM <= K  O(n)
int longestSubarraySum(const vector<int>& a, int k) {
    int l = 0, sum = 0, maxLen = 0;
    for (int r = 0; r < (int)a.size(); r++) {
        sum += a[r];
        while (sum > k) sum -= a[l++];
        maxLen = max(maxLen, r - l + 1);
    }
    return maxLen;
}

// 11. LONGEST SUBARRAY WITH AT MOST K DISTINCT ELEMENTS  O(n)
int longestSubarrayKDistinct(const string& s, int k) {
    unordered_map<char, int> cnt;
    int l = 0, maxLen = 0;
    for (int r = 0; r < (int)s.size(); r++) {
        cnt[s[r]]++;
        while ((int)cnt.size() > k) {
            if (--cnt[s[l]] == 0) cnt.erase(s[l]);
            l++;
        }
        maxLen = max(maxLen, r - l + 1);
    }
    return maxLen;
}

// 12. SUBARRAY PRODUCT LESS THAN K  O(n)
// Count subarrays where product < k.
int numSubarrayProductLessThanK(const vector<int>& nums, int k) {
    if (k <= 1) return 0;
    int product = 1, l = 0, count = 0;
    for (int r = 0; r < (int)nums.size(); r++) {
        product *= nums[r];
        while (product >= k) product /= nums[l++];
        count += r - l + 1;  // subarrays ending at r: [l..r], [l+1..r], ..., [r..r]
    }
    return count;
}

// 13. MAXIMUM CONSECUTIVE ONES WITH K FLIPS  O(n)
// Binary array, can flip at most k 0s to 1. Max length of consecutive 1s.
int maxConsecutiveOnes(const vector<int>& nums, int k) {
    int l = 0, zeros = 0, maxLen = 0;
    for (int r = 0; r < (int)nums.size(); r++) {
        if (nums[r] == 0) zeros++;
        while (zeros > k) { if (nums[l++] == 0) zeros--; }
        maxLen = max(maxLen, r - l + 1);
    }
    return maxLen;
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Four sum: find all unique quadruplets summing to target.
 *     (Extend 3Sum: fix two, use two pointer for remaining two)
 *
 * [2] Longest repeating character replacement: can change at most k chars.
 *     What is max length of substring with same character after changes?
 *     (Sliding window: maxFreq * len + k >= len - maxFreq)
 *
 * [3] Fruit into basket: two baskets, each holds one fruit type.
 *     Max fruits = longest subarray with at most 2 distinct values.
 *
 * [4] Max consecutive sum subarray with at most k size.
 *
 * [5] Subarrays with exactly k different integers.
 *     = atMost(k) - atMost(k-1) where atMost = count subarrays with <= k distinct.
 */

// Exercise [2] – Character replacement
int characterReplacement(const string& s, int k) {
    vector<int> cnt(26, 0);
    int l = 0, maxFreq = 0, maxLen = 0;
    for (int r = 0; r < (int)s.size(); r++) {
        cnt[s[r] - 'A']++;
        maxFreq = max(maxFreq, cnt[s[r] - 'A']);
        while (r - l + 1 - maxFreq > k) cnt[s[l++] - 'A']--;
        maxLen = max(maxLen, r - l + 1);
    }
    return maxLen;
}

// Exercise [5] – Subarrays with exactly k distinct
int atMostK(const vector<int>& nums, int k) {
    unordered_map<int,int> cnt;
    int l = 0, result = 0;
    for (int r = 0; r < (int)nums.size(); r++) {
        if (++cnt[nums[r]] == 1) k--;
        while (k < 0) { if (--cnt[nums[l]] == 0) { cnt.erase(nums[l]); k++; } l++; }
        result += r - l + 1;
    }
    return result;
}
int subarraysWithKDistinct(const vector<int>& nums, int k) {
    return atMostK(nums, k) - atMostK(nums, k - 1);
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Two pointer
    vector<int> sorted = {1, 2, 3, 4, 6};
    auto [l, r] = twoSum(sorted, 6);
    cout << "Two sum (target=6): indices " << l << " " << r << "\n";  // 1 3

    vector<int> ts = {-1, 0, 1, 2, -1, -4};
    auto triplets = threeSum(ts);
    cout << "Three sum: ";
    for (auto& t : triplets) cout << "[" << t[0] << "," << t[1] << "," << t[2] << "] ";
    cout << "\n";  // [-1,-1,2] [-1,0,1]

    vector<int> h = {1, 8, 6, 2, 5, 4, 8, 3, 7};
    cout << "Max water: " << maxWater(h) << "\n";  // 49

    vector<int> trap = {0, 1, 0, 2, 1, 0, 1, 3, 2, 1, 2, 1};
    cout << "Trapped water: " << trapRain(trap) << "\n";  // 6

    // Sliding window
    vector<int> a = {2, 1, 5, 1, 3, 2};
    cout << "Max sum subarray k=3: " << maxSumSubarrayK(a, 3) << "\n";  // 9

    cout << "Longest no-repeat substr 'abcabcbb': "
         << lengthOfLongestSubstring("abcabcbb") << "\n";  // 3

    cout << "Min window ('ADOBECODEBANC', 'ABC'): "
         << minWindow("ADOBECODEBANC", "ABC") << "\n";  // BANC

    vector<int> prod = {10, 5, 2, 6};
    cout << "Subarrays product < 100: "
         << numSubarrayProductLessThanK(prod, 100) << "\n";  // 8

    vector<int> ones = {1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0};
    cout << "Max consecutive 1s (k=2): " << maxConsecutiveOnes(ones, 2) << "\n";  // 6

    cout << "Char replacement ('AABABBA', k=1): "
         << characterReplacement("AABABBA", 1) << "\n";  // 4

    vector<int> sd = {1, 2, 1, 2, 3};
    cout << "Subarrays with exactly 2 distinct: "
         << subarraysWithKDistinct(sd, 2) << "\n";  // 7

    return 0;
}
