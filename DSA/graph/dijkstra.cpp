/**
 * DIJKSTRA – Shortest Path (Weighted Graph)
 * ==========================================
 * Use when: edges have non-negative weights.
 * Complexity: O((V + E) log V) with priority_queue
 *
 * For negative weights → Bellman-Ford
 * For all-pairs → Floyd-Warshall
 */

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
typedef pair<ll, int> pli;   // {dist, node}
const ll INF = 1e18;

// ─────────────────────────────────────────────
// 1. DIJKSTRA – Single source shortest path
// ─────────────────────────────────────────────
// adj[u] = list of {v, weight}
vector<ll> dijkstra(const vector<vector<pair<int,ll>>>& adj, int src) {
    int n = adj.size();
    vector<ll> dist(n, INF);
    priority_queue<pli, vector<pli>, greater<pli>> pq;  // min-heap
    dist[src] = 0;
    pq.push({0, src});
    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > dist[u]) continue;  // stale entry, skip
        for (auto [v, w] : adj[u]) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                pq.push({dist[v], v});
            }
        }
    }
    return dist;
}

// Dijkstra with path reconstruction
pair<vector<ll>, vector<int>> dijkstraPath(const vector<vector<pair<int,ll>>>& adj, int src) {
    int n = adj.size();
    vector<ll> dist(n, INF);
    vector<int> prev(n, -1);
    priority_queue<pli, vector<pli>, greater<pli>> pq;
    dist[src] = 0;
    pq.push({0, src});
    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > dist[u]) continue;
        for (auto [v, w] : adj[u]) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                prev[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
    return {dist, prev};
}

vector<int> getPath(const vector<int>& prev, int src, int dst) {
    vector<int> path;
    for (int v = dst; v != -1; v = prev[v]) path.push_back(v);
    if (path.back() != src) return {};  // unreachable
    reverse(path.begin(), path.end());
    return path;
}

// ─────────────────────────────────────────────
// 2. BELLMAN-FORD – handles negative weights  O(VE)
// ─────────────────────────────────────────────
// Also detects negative cycles.
struct Edge { int u, v; ll w; };

pair<vector<ll>, bool> bellmanFord(int n, const vector<Edge>& edges, int src) {
    vector<ll> dist(n, INF);
    dist[src] = 0;
    // Relax all edges V-1 times
    for (int i = 0; i < n - 1; i++) {
        bool updated = false;
        for (auto& [u, v, w] : edges) {
            if (dist[u] != INF && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                updated = true;
            }
        }
        if (!updated) break;  // early termination
    }
    // Check for negative cycles (n-th relaxation)
    bool hasNegCycle = false;
    for (auto& [u, v, w] : edges)
        if (dist[u] != INF && dist[u] + w < dist[v]) { hasNegCycle = true; break; }
    return {dist, hasNegCycle};
}

// ─────────────────────────────────────────────
// 3. FLOYD-WARSHALL – all pairs shortest path  O(V³)
// ─────────────────────────────────────────────
// Works with negative weights (but not negative cycles).
// Use for small graphs (V ≤ 500).
vector<vector<ll>> floydWarshall(int n, const vector<Edge>& edges) {
    vector<vector<ll>> dist(n, vector<ll>(n, INF));
    for (int i = 0; i < n; i++) dist[i][i] = 0;
    for (auto& [u, v, w] : edges) {
        dist[u][v] = min(dist[u][v], w);
        dist[v][u] = min(dist[v][u], w);  // remove if directed
    }
    for (int k = 0; k < n; k++)      // intermediate node
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                if (dist[i][k] != INF && dist[k][j] != INF)
                    dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j]);
    return dist;
}

// ─────────────────────────────────────────────
// 4. DIJKSTRA ON GRID with variable movement cost
// ─────────────────────────────────────────────
// Grid where moving to cell (r,c) costs grid[r][c]
struct State { ll dist; int r, c; };
auto cmp = [](State a, State b){ return a.dist > b.dist; };

ll dijkstraGrid(const vector<vector<int>>& grid) {
    int rows = grid.size(), cols = grid[0].size();
    vector<vector<ll>> dist(rows, vector<ll>(cols, INF));
    priority_queue<State, vector<State>, decltype(cmp)> pq(cmp);
    dist[0][0] = grid[0][0];
    pq.push({grid[0][0], 0, 0});
    int dr[] = {0,0,1,-1};
    int dc[] = {1,-1,0,0};
    while (!pq.empty()) {
        auto [d, r, c] = pq.top(); pq.pop();
        if (d > dist[r][c]) continue;
        if (r == rows-1 && c == cols-1) return d;
        for (int i = 0; i < 4; i++) {
            int nr = r + dr[i], nc = c + dc[i];
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                ll nd = d + grid[nr][nc];
                if (nd < dist[nr][nc]) {
                    dist[nr][nc] = nd;
                    pq.push({nd, nr, nc});
                }
            }
        }
    }
    return dist[rows-1][cols-1];
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Network delay time: given directed weighted graph, find min time
 *     for signal from node k to reach ALL nodes. Return -1 if impossible.
 *     (Dijkstra, return max of all distances)
 *
 * [2] Cheapest flights within k stops: find min cost from src to dst
 *     using at most k stops. (Modified Dijkstra or Bellman-Ford with k iterations)
 *
 * [3] Path with minimum effort: grid where effort = max |diff| between adjacent cells.
 *     Find path from top-left to bottom-right minimizing max effort.
 *     (Dijkstra where edge weight = |height diff|)
 *
 * [4] Find city with smallest number of neighbors at threshold distance.
 *     (Floyd-Warshall then count neighbors within threshold per city)
 *
 * [5] Detect negative cycle in directed graph using Bellman-Ford.
 */

// Exercise [1] – Network delay time
int networkDelayTime(vector<vector<int>>& times, int n, int k) {
    vector<vector<pair<int,ll>>> adj(n + 1);
    for (auto& t : times) adj[t[0]].push_back({t[1], t[2]});
    auto dist = dijkstra(adj, k);
    ll maxDist = 0;
    for (int i = 1; i <= n; i++) {
        if (dist[i] == INF) return -1;
        maxDist = max(maxDist, dist[i]);
    }
    return maxDist;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Dijkstra example
    //  0 --1--> 1 --2--> 3
    //  |                 ^
    //  4                 |
    //  v                 |
    //  2 ----1---------> 3
    int n = 4;
    vector<vector<pair<int,ll>>> adj(n);
    adj[0].push_back({1, 1}); adj[0].push_back({2, 4});
    adj[1].push_back({2, 2}); adj[1].push_back({3, 5});
    adj[2].push_back({3, 1});

    auto dist = dijkstra(adj, 0);
    cout << "Dijkstra distances from 0: ";
    for (ll d : dist) cout << (d == INF ? -1 : d) << " ";  // 0 1 3 4
    cout << "\n";

    auto [d2, prev] = dijkstraPath(adj, 0);
    auto path = getPath(prev, 0, 3);
    cout << "Path 0→3: ";
    for (int v : path) cout << v << " ";  // 0 1 2 3
    cout << "\n";

    // Bellman-Ford
    vector<Edge> edges = {{0,1,1},{0,2,4},{1,2,2},{1,3,5},{2,3,1}};
    auto [bf_dist, neg] = bellmanFord(n, edges, 0);
    cout << "Bellman-Ford from 0: ";
    for (ll d : bf_dist) cout << (d == INF ? -1 : d) << " ";
    cout << (neg ? "(neg cycle)" : "") << "\n";

    // Floyd-Warshall
    auto fw = floydWarshall(n, edges);
    cout << "Floyd-Warshall 0→3: " << fw[0][3] << "\n";  // 4

    // Dijkstra on grid
    vector<vector<int>> grid = {{1,3,1},{1,5,1},{4,2,1}};
    cout << "Grid min cost: " << dijkstraGrid(grid) << "\n";  // 7

    return 0;
}
