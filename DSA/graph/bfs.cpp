/**
 * BFS – Breadth-First Search
 * ===========================
 * Use BFS when:
 *   - Finding shortest path in UNWEIGHTED graph/grid
 *   - Level-order traversal
 *   - Finding all nodes reachable within k steps
 *
 * Complexity: O(V + E)
 */

#include <bits/stdc++.h>
using namespace std;

// ─────────────────────────────────────────────
// 1. BFS on adjacency list  O(V + E)
// ─────────────────────────────────────────────
// Returns shortest distance from src to all nodes (-1 if unreachable)
vector<int> bfs(const vector<vector<int>>& adj, int src) {
    int n = adj.size();
    vector<int> dist(n, -1);
    queue<int> q;
    dist[src] = 0;
    q.push(src);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : adj[u]) {
            if (dist[v] == -1) {
                dist[v] = dist[u] + 1;
                q.push(v);
            }
        }
    }
    return dist;
}

// ─────────────────────────────────────────────
// 2. BFS on 2D GRID  (most common in contests!)
// ─────────────────────────────────────────────
// Find shortest path from start to end in a grid.
// '.' = passable, '#' = wall

const int dx[] = {0, 0, 1, -1};   // 4-directional
const int dy[] = {1, -1, 0, 0};
// For 8-directional add diagonals:
// const int dx[] = {0,0,1,-1,1,1,-1,-1};
// const int dy[] = {1,-1,0,0,1,-1,1,-1};

int bfsGrid(const vector<string>& grid, pair<int,int> start, pair<int,int> end) {
    int rows = grid.size(), cols = grid[0].size();
    vector<vector<int>> dist(rows, vector<int>(cols, -1));

    auto [sr, sc] = start;
    auto [er, ec] = end;
    if (grid[sr][sc] == '#' || grid[er][ec] == '#') return -1;

    queue<pair<int,int>> q;
    dist[sr][sc] = 0;
    q.push({sr, sc});

    while (!q.empty()) {
        auto [r, c] = q.front(); q.pop();
        if (r == er && c == ec) return dist[r][c];
        for (int d = 0; d < 4; d++) {
            int nr = r + dx[d], nc = c + dy[d];
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols
                && grid[nr][nc] != '#' && dist[nr][nc] == -1) {
                dist[nr][nc] = dist[r][c] + 1;
                q.push({nr, nc});
            }
        }
    }
    return -1;  // unreachable
}

// Reconstruct path in grid BFS
vector<pair<int,int>> bfsGridPath(const vector<string>& grid,
                                   pair<int,int> start, pair<int,int> end) {
    int rows = grid.size(), cols = grid[0].size();
    vector<vector<pair<int,int>>> parent(rows, vector<pair<int,int>>(cols, {-1,-1}));
    vector<vector<bool>> visited(rows, vector<bool>(cols, false));

    auto [sr, sc] = start;
    auto [er, ec] = end;

    queue<pair<int,int>> q;
    visited[sr][sc] = true;
    q.push({sr, sc});

    while (!q.empty()) {
        auto [r, c] = q.front(); q.pop();
        if (r == er && c == ec) break;
        for (int d = 0; d < 4; d++) {
            int nr = r + dx[d], nc = c + dy[d];
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols
                && grid[nr][nc] != '#' && !visited[nr][nc]) {
                visited[nr][nc] = true;
                parent[nr][nc] = {r, c};
                q.push({nr, nc});
            }
        }
    }
    if (!visited[er][ec]) return {};  // no path

    vector<pair<int,int>> path;
    for (pair<int,int> cur = end; cur != make_pair(-1,-1); cur = parent[cur.first][cur.second])
        path.push_back(cur);
    reverse(path.begin(), path.end());
    return path;
}

// ─────────────────────────────────────────────
// 3. MULTI-SOURCE BFS
// ─────────────────────────────────────────────
// Find distance from each cell to nearest '1' (or nearest source).
// Start all sources simultaneously.
vector<vector<int>> multiSourceBFS(const vector<vector<int>>& grid) {
    int rows = grid.size(), cols = grid[0].size();
    vector<vector<int>> dist(rows, vector<int>(cols, -1));
    queue<pair<int,int>> q;
    // Add all sources at distance 0
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            if (grid[r][c] == 1) { dist[r][c] = 0; q.push({r, c}); }
    while (!q.empty()) {
        auto [r, c] = q.front(); q.pop();
        for (int d = 0; d < 4; d++) {
            int nr = r + dx[d], nc = c + dy[d];
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && dist[nr][nc] == -1) {
                dist[nr][nc] = dist[r][c] + 1;
                q.push({nr, nc});
            }
        }
    }
    return dist;
}

// ─────────────────────────────────────────────
// 4. 0-1 BFS (deque BFS)
// ─────────────────────────────────────────────
// Edge weights are 0 or 1. Use deque: weight-0 edges → front, weight-1 → back.
// O(V + E) – faster than Dijkstra for 0/1 weights.
vector<int> bfs01(const vector<vector<pair<int,int>>>& adj, int src) {
    int n = adj.size();
    vector<int> dist(n, INT_MAX);
    deque<int> dq;
    dist[src] = 0;
    dq.push_front(src);
    while (!dq.empty()) {
        int u = dq.front(); dq.pop_front();
        for (auto [v, w] : adj[u]) {
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                if (w == 0) dq.push_front(v);
                else dq.push_back(v);
            }
        }
    }
    return dist;
}

// ─────────────────────────────────────────────
// 5. BFS – COUNT CONNECTED COMPONENTS  O(V + E)
// ─────────────────────────────────────────────
int countComponents(const vector<vector<int>>& adj) {
    int n = adj.size(), components = 0;
    vector<bool> visited(n, false);
    for (int i = 0; i < n; i++) {
        if (!visited[i]) {
            components++;
            queue<int> q;
            q.push(i); visited[i] = true;
            while (!q.empty()) {
                int u = q.front(); q.pop();
                for (int v : adj[u])
                    if (!visited[v]) { visited[v] = true; q.push(v); }
            }
        }
    }
    return components;
}

// ─────────────────────────────────────────────
// 6. WORD LADDER BFS
// ─────────────────────────────────────────────
// Transform beginWord → endWord by changing one letter at a time.
// Each intermediate word must be in wordList.
int ladderLength(string beginWord, string endWord, vector<string>& wordList) {
    unordered_set<string> wordSet(wordList.begin(), wordList.end());
    if (!wordSet.count(endWord)) return 0;
    queue<pair<string,int>> q;
    q.push({beginWord, 1});
    wordSet.erase(beginWord);
    while (!q.empty()) {
        auto [word, steps] = q.front(); q.pop();
        for (int i = 0; i < (int)word.size(); i++) {
            string next = word;
            for (char c = 'a'; c <= 'z'; c++) {
                next[i] = c;
                if (wordSet.count(next)) {
                    if (next == endWord) return steps + 1;
                    wordSet.erase(next);
                    q.push({next, steps + 1});
                }
            }
        }
    }
    return 0;
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Number of islands: grid of '0'(water)/'1'(land).
 *     Count number of islands (connected components of '1').
 *
 * [2] Rotting oranges: grid has 0(empty), 1(fresh), 2(rotten).
 *     Each minute, fresh oranges adjacent to rotten become rotten.
 *     Find minimum minutes until all rot. (Multi-source BFS)
 *
 * [3] Maze with keys: '@' start, '#' wall, '.' floor, 'a'-'f' keys, 'A'-'F' locks.
 *     Find shortest path collecting all keys. (BFS with bitmask state)
 *
 * [4] Jump game III: can reach any index with arr[i]=0 starting from start?
 *     From index i, can jump to i+arr[i] or i-arr[i].
 *
 * [5] Minimum knight moves on infinite chess board from (0,0) to (x,y).
 */

// Exercise [1] – Number of islands (BFS version)
int numIslands(vector<vector<char>>& grid) {
    int rows = grid.size(), cols = grid[0].size(), count = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (grid[r][c] == '1') {
                count++;
                queue<pair<int,int>> q;
                q.push({r, c}); grid[r][c] = '0';
                while (!q.empty()) {
                    auto [nr, nc] = q.front(); q.pop();
                    for (int d = 0; d < 4; d++) {
                        int rr = nr + dx[d], cc = nc + dy[d];
                        if (rr >= 0 && rr < rows && cc >= 0 && cc < cols && grid[rr][cc] == '1') {
                            grid[rr][cc] = '0';
                            q.push({rr, cc});
                        }
                    }
                }
            }
        }
    }
    return count;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // BFS on graph
    vector<vector<int>> adj = {{1,2},{0,3},{0,4,5},{1},{2},{2}};
    auto dist = bfs(adj, 0);
    cout << "BFS distances from 0: ";
    for (int d : dist) cout << d << " ";  // 0 1 1 2 2 2
    cout << "\n";

    // BFS on grid
    vector<string> grid = {
        "......",
        "..##..",
        "......",
        "...##.",
        "......"
    };
    cout << "Grid shortest path: "
         << bfsGrid(grid, {0,0}, {4,5}) << "\n";

    // Multi-source BFS
    vector<vector<int>> g2 = {{0,0,0},{0,1,0},{1,0,1}};
    auto d2 = multiSourceBFS(g2);
    cout << "Multi-source BFS distances:\n";
    for (auto& row : d2) { for (int x : row) cout << x << " "; cout << "\n"; }

    // Count components
    vector<vector<int>> adj2 = {{1},{0},{3},{2},{5},{4}};
    cout << "Components: " << countComponents(adj2) << "\n";  // 3

    // Number of islands
    vector<vector<char>> islands = {
        {'1','1','0','0','0'},
        {'1','1','0','0','0'},
        {'0','0','1','0','0'},
        {'0','0','0','1','1'}
    };
    cout << "Islands: " << numIslands(islands) << "\n";  // 3

    return 0;
}
