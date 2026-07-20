/**
 * TOPOLOGICAL SORT
 * =================
 * For Directed Acyclic Graph (DAG).
 * Linear ordering of vertices such that for every edge u→v, u comes before v.
 *
 * Two methods:
 *   1. Kahn's BFS (in-degree method) – easier to implement
 *   2. DFS post-order              – elegant recursion
 *
 * Applications:
 *   - Course scheduling (prerequisites)
 *   - Build systems (compile order)
 *   - Longest path in DAG
 *   - DP on DAG
 */

#include <bits/stdc++.h>
using namespace std;

// ─────────────────────────────────────────────
// 1. KAHN'S ALGORITHM (BFS)  O(V + E)
// ─────────────────────────────────────────────
// Returns topological order, or empty vector if cycle exists.
vector<int> topoSortBFS(int n, const vector<vector<int>>& adj) {
    vector<int> indegree(n, 0);
    for (int u = 0; u < n; u++)
        for (int v : adj[u]) indegree[v]++;

    queue<int> q;
    for (int i = 0; i < n; i++)
        if (indegree[i] == 0) q.push(i);

    vector<int> order;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        order.push_back(u);
        for (int v : adj[u])
            if (--indegree[v] == 0) q.push(v);
    }
    if ((int)order.size() != n) return {};  // cycle detected
    return order;
}

// ─────────────────────────────────────────────
// 2. DFS-based TOPOLOGICAL SORT  O(V + E)
// ─────────────────────────────────────────────
// 0=unvisited, 1=in-progress (detect cycle), 2=done
bool dfs_topo(int u, const vector<vector<int>>& adj,
              vector<int>& state, vector<int>& order) {
    state[u] = 1;
    for (int v : adj[u]) {
        if (state[v] == 1) return false;  // back edge → cycle
        if (state[v] == 0)
            if (!dfs_topo(v, adj, state, order)) return false;
    }
    state[u] = 2;
    order.push_back(u);
    return true;
}

vector<int> topoSortDFS(int n, const vector<vector<int>>& adj) {
    vector<int> state(n, 0), order;
    for (int i = 0; i < n; i++)
        if (state[i] == 0)
            if (!dfs_topo(i, adj, state, order)) return {};  // cycle
    reverse(order.begin(), order.end());
    return order;
}

// ─────────────────────────────────────────────
// 3. DETECT CYCLE IN DIRECTED GRAPH  O(V + E)
// ─────────────────────────────────────────────
bool hasCycleDirected(int n, const vector<vector<int>>& adj) {
    return topoSortBFS(n, adj).empty();
}

// ─────────────────────────────────────────────
// 4. LONGEST PATH IN DAG  O(V + E)
// ─────────────────────────────────────────────
// Process in topological order, relax edges.
int longestPathDAG(int n, const vector<vector<pair<int,int>>>& adj) {
    // adj[u] = {v, weight}
    // First build unweighted adj for topo sort
    vector<vector<int>> plain(n);
    for (int u = 0; u < n; u++)
        for (auto [v, w] : adj[u]) plain[u].push_back(v);

    auto order = topoSortBFS(n, plain);
    if (order.empty()) return -1;  // cycle

    vector<int> dp(n, 0);
    for (int u : order)
        for (auto [v, w] : adj[u])
            dp[v] = max(dp[v], dp[u] + w);
    return *max_element(dp.begin(), dp.end());
}

// ─────────────────────────────────────────────
// 5. COURSE SCHEDULE  O(V + E)
// ─────────────────────────────────────────────
// Can finish all courses given prerequisites?
bool canFinishCourses(int numCourses, vector<vector<int>>& prerequisites) {
    vector<vector<int>> adj(numCourses);
    for (auto& p : prerequisites) adj[p[1]].push_back(p[0]);
    return !topoSortBFS(numCourses, adj).empty();
}

// Return actual order to take courses
vector<int> findOrder(int numCourses, vector<vector<int>>& prerequisites) {
    vector<vector<int>> adj(numCourses);
    for (auto& p : prerequisites) adj[p[1]].push_back(p[0]);
    return topoSortBFS(numCourses, adj);
}

// ─────────────────────────────────────────────
// 6. ALIEN DICTIONARY  O(total chars + E)
// ─────────────────────────────────────────────
// Given sorted alien dictionary words, find character ordering.
string alienOrder(vector<string>& words) {
    map<char, set<char>> adj;
    map<char, int> indegree;
    // Initialize all chars
    for (auto& w : words)
        for (char c : w) { adj[c]; indegree[c]; }
    // Build edges from adjacent words
    for (int i = 0; i + 1 < (int)words.size(); i++) {
        string& a = words[i]; string& b = words[i+1];
        bool found = false;
        for (int j = 0; j < (int)min(a.size(), b.size()); j++) {
            if (a[j] != b[j]) {
                if (!adj[a[j]].count(b[j])) {
                    adj[a[j]].insert(b[j]);
                    indegree[b[j]]++;
                }
                found = true; break;
            }
        }
        // Edge case: prefix is larger → invalid
        if (!found && a.size() > b.size()) return "";
    }
    // Kahn's
    queue<char> q;
    for (auto& [c, deg] : indegree)
        if (deg == 0) q.push(c);
    string result;
    while (!q.empty()) {
        char c = q.front(); q.pop();
        result += c;
        for (char next : adj[c])
            if (--indegree[next] == 0) q.push(next);
    }
    return result.size() == indegree.size() ? result : "";  // cycle check
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Parallel courses: min semesters to take all courses.
 *     Process topo layer by layer (BFS level-order).
 *
 * [2] Course schedule III: given (duration, deadline) pairs,
 *     maximize number of courses you can take. (Greedy + heap)
 *
 * [3] Minimum height trees: find root(s) for min-height tree.
 *     Repeatedly remove leaf nodes (like topo sort from outside in).
 *
 * [4] Sequence reconstruction: check if original sequence can be
 *     uniquely reconstructed from seqs. (Topo sort with unique ordering check)
 *
 * [5] Find all possible topological sorts of a DAG.
 *     (Backtracking + topo sort)
 */

// Exercise [1] – Parallel courses (min semesters)
int minSemesters(int n, vector<vector<int>>& relations) {
    vector<vector<int>> adj(n + 1);
    vector<int> indegree(n + 1, 0);
    for (auto& r : relations) { adj[r[0]].push_back(r[1]); indegree[r[1]]++; }
    queue<int> q;
    for (int i = 1; i <= n; i++) if (indegree[i] == 0) q.push(i);
    int semesters = 0, taken = 0;
    while (!q.empty()) {
        semesters++;
        int sz = q.size();
        taken += sz;
        while (sz--) {
            int u = q.front(); q.pop();
            for (int v : adj[u]) if (--indegree[v] == 0) q.push(v);
        }
    }
    return taken == n ? semesters : -1;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Basic topo sort
    // 5 → 0, 5 → 2, 4 → 0, 4 → 1, 2 → 3, 3 → 1
    int n = 6;
    vector<vector<int>> adj(n);
    adj[5].push_back(0); adj[5].push_back(2);
    adj[4].push_back(0); adj[4].push_back(1);
    adj[2].push_back(3); adj[3].push_back(1);

    auto order = topoSortBFS(n, adj);
    cout << "Topo order (BFS): ";
    for (int v : order) cout << v << " ";
    cout << "\n";

    order = topoSortDFS(n, adj);
    cout << "Topo order (DFS): ";
    for (int v : order) cout << v << " ";
    cout << "\n";

    // Cycle detection
    vector<vector<int>> cycleAdj(3);
    cycleAdj[0].push_back(1); cycleAdj[1].push_back(2); cycleAdj[2].push_back(0);
    cout << "Has cycle: " << hasCycleDirected(3, cycleAdj) << "\n";  // 1

    // Longest path in DAG
    vector<vector<pair<int,int>>> wadj(4);
    wadj[0].push_back({1,3}); wadj[0].push_back({2,1});
    wadj[1].push_back({3,2}); wadj[2].push_back({3,5});
    cout << "Longest DAG path: " << longestPathDAG(4, wadj) << "\n";  // 6

    // Course schedule
    vector<vector<int>> prereqs = {{1,0},{2,0},{3,1},{3,2}};
    cout << "Can finish: " << canFinishCourses(4, prereqs) << "\n";  // 1
    auto courseOrder = findOrder(4, prereqs);
    cout << "Course order: ";
    for (int c : courseOrder) cout << c << " ";
    cout << "\n";

    // Min semesters
    vector<vector<int>> rels = {{1,3},{2,3}};
    cout << "Min semesters: " << minSemesters(3, rels) << "\n";  // 2

    return 0;
}
