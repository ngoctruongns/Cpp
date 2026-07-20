/**
 * EXERCISE PROBLEMS – LG CodeJam Style
 * ======================================
 * Bài tập tổng hợp các dạng thường gặp trong LG CodeJam.
 *
 * Dạng bài:
 *   P1. Simulation / Implementation
 *   P2. Grid BFS / Pathfinding
 *   P3. Dynamic Programming
 *   P4. Graph – Connectivity & Shortest Path
 *   P5. Greedy + Sorting
 *   P6. Data structures (Segment Tree, Fenwick)
 *   P7. String Processing
 *   P8. Combinatorics / Math
 *
 * Cách học:
 *   1. Đọc đề, phân loại dạng bài
 *   2. Viết brute force trước (đúng, chậm)
 *   3. Tối ưu hóa (đúng, nhanh)
 *   4. Test edge cases
 */

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
const int INF = 1e9;
const ll MOD = 1e9 + 7;

// ═══════════════════════════════════════════
// P1. SIMULATION: Robot Movement
// ═══════════════════════════════════════════
/*
 * Đề: Robot trong lưới n×m, bắt đầu từ (0,0), hướng North.
 * Lệnh: 'F' tiến 1 bước, 'L' quay trái 90°, 'R' quay phải 90°.
 * Cho chuỗi lệnh, tìm vị trí cuối sau khi thực hiện k lần toàn bộ chuỗi.
 *
 * Constraint: n,m ≤ 1000, lệnh ≤ 1000, k ≤ 10^9
 * Key insight: sau vài chu kỳ robot có thể lặp → tìm chu kỳ.
 */
pair<int,int> robotMovement(int n, int m, const string& cmds, int k) {
    // Directions: N E S W
    int dr[] = {-1, 0, 1, 0};
    int dc[] = {0, 1, 0, -1};
    int r = 0, c = 0, dir = 0;  // dir: 0=N, 1=E, 2=S, 3=W

    auto simulate = [&](int times) {
        for (int t = 0; t < times; t++) {
            for (char cmd : cmds) {
                if (cmd == 'F') {
                    int nr = r + dr[dir], nc = c + dc[dir];
                    if (nr >= 0 && nr < n && nc >= 0 && nc < m) { r = nr; c = nc; }
                } else if (cmd == 'L') dir = (dir + 3) % 4;
                else dir = (dir + 1) % 4;
            }
        }
    };

    // Detect cycle using Floyd's or just simulate until repeat
    vector<tuple<int,int,int>> states;
    unordered_map<int, int> seen;
    int r0 = r, c0 = c, d0 = dir;
    for (int i = 0; i < k; i++) {
        int key = r * m * 4 + c * 4 + dir;
        if (seen.count(key)) {
            int cycleStart = seen[key];
            int cycleLen = i - cycleStart;
            int remaining = (k - i) % cycleLen;
            simulate(remaining);
            return make_pair(r, c);
        }
        seen[key] = i;
        states.push_back({r, c, dir});
        simulate(1);
    }
    return {r, c};
}

// ═══════════════════════════════════════════
// P2. GRID BFS: Escape the Maze
// ═══════════════════════════════════════════
/*
 * Đề: Mê cung n×m. 'S'=start, 'E'=end, '#'=wall, '.'=empty, 'K'=key.
 * Phải nhặt ALL keys trước khi đến E. Tìm bước đi ngắn nhất.
 *
 * Approach: BFS với state (r, c, keys_collected) dùng bitmask.
 */
int escapeWithKeys(const vector<string>& maze) {
    int rows = maze.size(), cols = maze[0].size();
    int sr = -1, sc = -1, er = -1, ec = -1;
    int totalKeys = 0;
    vector<pair<int,int>> keys;

    for (int r = 0; r < rows; r++) for (int c = 0; c < cols; c++) {
        if (maze[r][c] == 'S') { sr = r; sc = c; }
        if (maze[r][c] == 'E') { er = r; ec = c; }
        if (islower(maze[r][c])) { keys.push_back({r,c}); totalKeys++; }
    }

    int allKeys = (1 << totalKeys) - 1;
    map<pair<int,int>, int> keyId;
    for (int i = 0; i < totalKeys; i++) keyId[keys[i]] = i;

    // State: (r, c, keyMask)
    vector<vector<vector<int>>> dist(rows, vector<vector<int>>(cols, vector<int>(1 << totalKeys, INF)));
    queue<tuple<int,int,int>> q;
    dist[sr][sc][0] = 0;
    q.push({sr, sc, 0});

    int dr[] = {0,0,1,-1}, dc[] = {1,-1,0,0};
    while (!q.empty()) {
        auto [r, c, mask] = q.front(); q.pop();
        if (r == er && c == ec && mask == allKeys) return dist[r][c][mask];
        for (int d = 0; d < 4; d++) {
            int nr = r + dr[d], nc = c + dc[d];
            if (nr < 0 || nr >= rows || nc < 0 || nc >= cols || maze[nr][nc] == '#') continue;
            int newMask = mask;
            if (keyId.count({nr, nc})) newMask |= (1 << keyId[{nr, nc}]);
            if (dist[r][c][mask] + 1 < dist[nr][nc][newMask]) {
                dist[nr][nc][newMask] = dist[r][c][mask] + 1;
                q.push({nr, nc, newMask});
            }
        }
    }
    return -1;
}

// ═══════════════════════════════════════════
// P3. DP: Minimum Cost to Paint Houses
// ═══════════════════════════════════════════
/*
 * Đề: n ngôi nhà, k màu. costs[i][j] = chi phí tô nhà i màu j.
 * Không được tô 2 nhà liền nhau cùng màu.
 * Tìm tổng chi phí nhỏ nhất.
 *
 * dp[i][j] = min cost to paint houses 0..i where house i is color j
 * dp[i][j] = costs[i][j] + min(dp[i-1][j'] for j' != j)
 *
 * Optimization: track min1, min2, min1_idx instead of O(k) inner loop → O(nk)
 */
int minCostPaintHouses(const vector<vector<int>>& costs) {
    int n = costs.size(), k = costs[0].size();
    vector<int> dp = costs[0];
    for (int i = 1; i < n; i++) {
        // Find min and 2nd min of previous row
        int min1 = INF, min2 = INF, min1j = -1;
        for (int j = 0; j < k; j++) {
            if (dp[j] < min1) { min2 = min1; min1 = dp[j]; min1j = j; }
            else if (dp[j] < min2) min2 = dp[j];
        }
        vector<int> newDp(k);
        for (int j = 0; j < k; j++) {
            int prev = (j == min1j) ? min2 : min1;
            newDp[j] = costs[i][j] + prev;
        }
        dp = newDp;
    }
    return *min_element(dp.begin(), dp.end());
}

// ═══════════════════════════════════════════
// P4. GRAPH: Critical Connections (Bridges)
// ═══════════════════════════════════════════
/*
 * Đề: Mạng lưới n server và m kết nối. Tìm tất cả kết nối "critical"
 * – nếu xóa thì mạng bị mất kết nối.
 *
 * Algorithm: Tarjan's bridge finding  O(V + E)
 * disc[u] = time when u was first discovered
 * low[u]  = min disc reachable from subtree rooted at u
 * Edge (u,v) is bridge if low[v] > disc[u]
 */
vector<vector<int>> findBridges(int n, vector<vector<int>>& connections) {
    vector<vector<int>> adj(n);
    for (auto& c : connections) { adj[c[0]].push_back(c[1]); adj[c[1]].push_back(c[0]); }

    vector<int> disc(n, -1), low(n, 0);
    int timer = 0;
    vector<vector<int>> bridges;

    function<void(int, int)> dfs = [&](int u, int parent) {
        disc[u] = low[u] = timer++;
        for (int v : adj[u]) {
            if (v == parent) continue;
            if (disc[v] == -1) {
                dfs(v, u);
                low[u] = min(low[u], low[v]);
                if (low[v] > disc[u]) bridges.push_back({u, v});
            } else {
                low[u] = min(low[u], disc[v]);
            }
        }
    };

    for (int i = 0; i < n; i++) if (disc[i] == -1) dfs(i, -1);
    return bridges;
}

// ═══════════════════════════════════════════
// P5. GREEDY: Interval Scheduling
// ═══════════════════════════════════════════
/*
 * Đề: Cho n công việc mỗi việc có (start, end). Tìm số lượng tối đa
 * công việc không chồng chéo.
 *
 * Classic greedy: sort by end time, greedily pick.
 */
int maxNonOverlapping(vector<pair<int,int>> intervals) {
    sort(intervals.begin(), intervals.end(),
         [](const pair<int,int>& a, const pair<int,int>& b){ return a.second < b.second; });
    int count = 0, lastEnd = INT_MIN;
    for (auto [s, e] : intervals) {
        if (s >= lastEnd) { count++; lastEnd = e; }
    }
    return count;
}

// Minimum meeting rooms needed  O(n log n)
int minMeetingRooms(vector<pair<int,int>>& intervals) {
    vector<int> starts, ends;
    for (auto [s, e] : intervals) { starts.push_back(s); ends.push_back(e); }
    sort(starts.begin(), starts.end());
    sort(ends.begin(), ends.end());
    int rooms = 0, maxRooms = 0, i = 0, j = 0;
    while (i < (int)starts.size()) {
        if (starts[i] < ends[j]) { rooms++; i++; }
        else { rooms--; j++; }
        maxRooms = max(maxRooms, rooms);
    }
    return maxRooms;
}

// ═══════════════════════════════════════════
// P6. SEGMENT TREE PROBLEM: Range Max Update, Point Query
// ═══════════════════════════════════════════
/*
 * Đề: Mảng a[1..n]. Xử lý 2 loại query:
 *   1. update(l, r, v): a[i] = max(a[i], v) for all i in [l,r]
 *   2. query(i): return a[i]
 *
 * Approach: Segment tree with lazy propagation (max update).
 */
struct LazySegTree {
    int n;
    vector<int> lazy;  // lazy[node] = pending max update

    LazySegTree(int n) : n(n), lazy(4 * n, 0) {}

    void push(int node) {
        if (lazy[node]) {
            lazy[2*node] = max(lazy[2*node], lazy[node]);
            lazy[2*node+1] = max(lazy[2*node+1], lazy[node]);
            lazy[node] = 0;
        }
    }

    void update(int node, int start, int end, int l, int r, int val) {
        if (r < start || end < l) return;
        if (l <= start && end <= r) { lazy[node] = max(lazy[node], val); return; }
        push(node);
        int mid = (start + end) / 2;
        update(2*node, start, mid, l, r, val);
        update(2*node+1, mid+1, end, l, r, val);
    }

    int query(int node, int start, int end, int idx) {
        if (start == end) return lazy[node];
        push(node);
        int mid = (start + end) / 2;
        if (idx <= mid) return query(2*node, start, mid, idx);
        return query(2*node+1, mid+1, end, idx);
    }

    void update(int l, int r, int val) { update(1, 1, n, l, r, val); }
    int query(int idx) { return query(1, 1, n, idx); }
};

// ═══════════════════════════════════════════
// P7. STRING: Restore IP Addresses
// ═══════════════════════════════════════════
/*
 * Đề: Chuỗi chỉ gồm chữ số. Trả về tất cả IP hợp lệ có thể tạo.
 * IP hợp lệ: 4 phần, mỗi phần 0-255, không leading zeros.
 */
vector<string> restoreIPAddresses(const string& s) {
    vector<string> result;
    function<void(int, int, string)> bt = [&](int start, int part, string cur) {
        if (part == 4 && start == (int)s.size()) { result.push_back(cur.substr(1)); return; }
        if (part == 4 || start == (int)s.size()) return;
        for (int len = 1; len <= 3 && start + len <= (int)s.size(); len++) {
            string seg = s.substr(start, len);
            if (seg.size() > 1 && seg[0] == '0') break;  // no leading zeros
            if (stoi(seg) > 255) break;
            bt(start + len, part + 1, cur + "." + seg);
        }
    };
    bt(0, 0, "");
    return result;
}

// ═══════════════════════════════════════════
// P8. MATH: Count Beautiful Numbers
// ═══════════════════════════════════════════
/*
 * Đề: Đếm số nguyên trong [1, n] mà tổng các chữ số chia hết cho k.
 * Digit DP approach.
 *
 * dp[pos][sum][tight][started]
 * pos: vị trí chữ số hiện tại
 * sum: tổng chữ số mod k đến nay
 * tight: có bị ràng buộc bởi n không
 * started: đã bắt đầu đặt chữ số chưa (để bỏ leading zeros)
 */
ll countBeautifulNumbers(ll n, int k) {
    string s = to_string(n);
    int len = s.size();
    // memo[pos][sum][tight] – started always true after first digit
    vector<vector<array<ll,2>>> dp(len, vector<array<ll,2>>(k, {-1LL, -1LL}));

    function<ll(int,int,bool,bool)> solve = [&](int pos, int sum, bool tight, bool started) -> ll {
        if (pos == len) return started && (sum == 0) ? 1 : 0;
        ll& ref = dp[pos][sum][tight];
        if (!started && ref == -1) ref = 0;  // use ref carefully
        // Actually use full memoization with started flag
        int limit = tight ? (s[pos] - '0') : 9;
        ll result = 0;
        for (int d = 0; d <= limit; d++) {
            bool newTight = tight && (d == limit);
            if (!started && d == 0) result += solve(pos+1, 0, newTight, false);
            else result += solve(pos+1, (sum + d) % k, newTight, true);
        }
        return result;
    };

    return solve(0, 0, true, false);
}

// ─────────────────────────────────────────────
// TEST ALL PROBLEMS
// ─────────────────────────────────────────────
int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    cout << "=== P1. Robot Movement ===" << "\n";
    auto [r, c] = robotMovement(10, 10, "FFLFF", 3);
    cout << "Final position: (" << r << ", " << c << ")\n";

    cout << "\n=== P2. Escape with Keys ===" << "\n";
    vector<string> maze = {
        "S.a.",
        "####",
        "..b.",
        "..E."
    };
    // No path through wall – expect -1
    cout << "Steps: " << escapeWithKeys(maze) << "\n";

    cout << "\n=== P3. Min Cost Paint Houses ===" << "\n";
    vector<vector<int>> costs = {{17,2,17},{16,16,5},{14,3,19}};
    cout << "Min cost: " << minCostPaintHouses(costs) << "\n";  // 10

    cout << "\n=== P4. Critical Connections ===" << "\n";
    vector<vector<int>> conns = {{0,1},{1,2},{2,0},{1,3}};
    auto bridges = findBridges(4, conns);
    cout << "Bridges: ";
    for (auto& b : bridges) cout << "(" << b[0] << "," << b[1] << ") ";
    cout << "\n";  // (1,3)

    cout << "\n=== P5. Interval Scheduling ===" << "\n";
    vector<pair<int,int>> intervals = {{1,3},{2,4},{3,5},{0,6},{5,7},{3,9},{6,10}};
    cout << "Max non-overlapping: " << maxNonOverlapping(intervals) << "\n";  // 4

    cout << "\n=== P6. Lazy Segment Tree ===" << "\n";
    LazySegTree seg(10);
    seg.update(1, 5, 10);
    seg.update(3, 7, 15);
    cout << "Query(3): " << seg.query(3) << "\n";   // 15
    cout << "Query(6): " << seg.query(6) << "\n";   // 15
    cout << "Query(8): " << seg.query(8) << "\n";   // 0

    cout << "\n=== P7. Restore IP ===" << "\n";
    auto ips = restoreIPAddresses("25525511135");
    for (auto& ip : ips) cout << ip << "\n";  // 255.255.11.135 / 255.255.111.35

    cout << "\n=== P8. Beautiful Numbers ===" << "\n";
    cout << "Count [1,100] divisible digit sum by 3: "
         << countBeautifulNumbers(100, 3) << "\n";

    return 0;
}
