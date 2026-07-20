/**
 * UNION-FIND (Disjoint Set Union – DSU)
 * ======================================
 * Operations: find(x), union(x, y), connected(x, y)
 * With path compression + union by rank: ~O(α(n)) per operation
 *
 * Applications:
 *   - Kruskal's MST
 *   - Detect cycle in undirected graph
 *   - Connected components
 *   - Online connectivity queries
 */

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
const ll INF = 1e18;

// ─────────────────────────────────────────────
// 1. BASIC DSU  O(α(n)) per operation
// ─────────────────────────────────────────────
struct DSU {
    vector<int> parent, rank_;
    int components;

    DSU(int n) : parent(n), rank_(n, 0), components(n) {
        iota(parent.begin(), parent.end(), 0);  // parent[i] = i
    }

    int find(int x) {
        if (parent[x] != x)
            parent[x] = find(parent[x]);  // path compression
        return parent[x];
    }

    bool unite(int x, int y) {
        x = find(x); y = find(y);
        if (x == y) return false;  // already same component
        if (rank_[x] < rank_[y]) swap(x, y);
        parent[y] = x;
        if (rank_[x] == rank_[y]) rank_[x]++;
        components--;
        return true;
    }

    bool connected(int x, int y) { return find(x) == find(y); }
    int count() { return components; }
    void printRank() {
        for (int i = 0; i < (int)rank_.size(); i++) {
            cout << "Node " << i << ": rank = " << rank_[i] << "\n";
        }
    }

    void printParents() {
        for (int i = 0; i < (int)parent.size(); i++) {
            cout << "Node " << i << ": parent = " << parent[i] << "\n";
        }
    }
};

// ─────────────────────────────────────────────
// 2. DSU WITH SIZE (useful for "size of component" queries)
// ─────────────────────────────────────────────
struct DSUSize {
    vector<int> parent, sz;

    DSUSize(int n) : parent(n), sz(n, 1) {
        iota(parent.begin(), parent.end(), 0);
    }

    int find(int x) {
        return parent[x] == x ? x : parent[x] = find(parent[x]);
    }

    bool unite(int x, int y) {
        x = find(x); y = find(y);
        if (x == y) return false;
        if (sz[x] < sz[y]) swap(x, y);  // attach smaller to larger
        parent[y] = x;
        sz[x] += sz[y];
        return true;
    }

    int size(int x) { return sz[find(x)]; }
};

// ─────────────────────────────────────────────
// 3. KRUSKAL'S MST  O(E log E)
// ─────────────────────────────────────────────rank_
// Minimum Spanning Tree using Union-Find.
// Sort edges by weight, greedily add non-cycle edges.

struct Edge {
    int u, v;
    ll w;
    bool operator<(const Edge& o) const { return w < o.w; }
};

// Returns {MST weight, edges in MST}
pair<ll, vector<Edge>> kruskal(int n, vector<Edge> edges) {
    sort(edges.begin(), edges.end());
    DSU dsu(n);
    ll total = 0;
    vector<Edge> mst;
    for (auto& e : edges) {
        if (dsu.unite(e.u, e.v)) {
            total += e.w;
            mst.push_back(e);
            if ((int)mst.size() == n - 1) break;  // MST complete
        }
    }
    // If mst.size() < n-1, graph is disconnected
    return {total, mst};
}

// ─────────────────────────────────────────────
// 4. DETECT CYCLE in undirected graph
// ─────────────────────────────────────────────
bool hasCycle(int n, const vector<pair<int,int>>& edges) {
    DSU dsu(n);
    for (auto [u, v] : edges) {
        if (!dsu.unite(u, v)) return true;  // already connected → cycle
    }
    return false;
}

// ─────────────────────────────────────────────
// 5. DYNAMIC CONNECTIVITY (offline)
// ─────────────────────────────────────────────
// Process queries: add edge, query if two nodes connected.
// Note: Standard DSU doesn't support edge deletion.

// ─────────────────────────────────────────────
// 6. ACCOUNTS MERGE (string-based DSU)
// ─────────────────────────────────────────────
// Given accounts where each account has a name and emails,
// merge accounts sharing at least one email.
vector<vector<string>> accountsMerge(vector<vector<string>>& accounts) {
    unordered_map<string, int> emailToId;
    unordered_map<string, string> emailToName;
    int id = 0;
    for (auto& acc : accounts) {
        string name = acc[0];
        for (int i = 1; i < (int)acc.size(); i++) {
            if (!emailToId.count(acc[i])) emailToId[acc[i]] = id++;
            emailToName[acc[i]] = name;
        }
    }
    DSU dsu(id);
    for (auto& acc : accounts) {
        int first = emailToId[acc[1]];
        for (int i = 2; i < (int)acc.size(); i++)
            dsu.unite(first, emailToId[acc[i]]);
    }
    unordered_map<int, vector<string>> groups;
    for (auto& [email, eid] : emailToId)
        groups[dsu.find(eid)].push_back(email);
    vector<vector<string>> result;
    for (auto& [root, emails] : groups) {
        sort(emails.begin(), emails.end());
        emails.insert(emails.begin(), emailToName[emails[0]]);
        result.push_back(emails);
    }
    return result;
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Number of provinces: n cities, connectivity matrix. Count components.
 *
 * [2] Redundant connection: find the edge that creates first cycle.
 *     (Unite edges one by one; when unite returns false, that's the answer)
 *
 * [3] Most stones removed with same row or column:
 *     Stones can be removed if another stone shares row or column.
 *     Max removable = n - (number of connected components in row-col graph)
 *
 * [4] Satisfiability of equality equations: a==b, b!=c type queries.
 *     First process all ==, then check != for contradiction.
 *
 * [5] Find critical connections (bridges) in graph.
 *     (Tarjan's algorithm – not DSU, but related to connectivity)
 */

// Exercise [2] – Redundant Connection
vector<int> findRedundantConnection(vector<vector<int>>& edges) {
    int n = edges.size();
    DSU dsu(n + 1);
    for (auto& e : edges) {
        if (!dsu.unite(e[0], e[1])) return e;
    }
    return {};
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Basic DSU
    DSU dsu(8);
    dsu.unite(0, 1);
    dsu.unite(1, 2);
    dsu.unite(3, 4);
    dsu.unite(1, 3);
    dsu.unite(6, 5);
    dsu.unite(6, 7);
    dsu.unite(1, 7);
    cout << "Connected(0,2): " << dsu.connected(0, 2) << "\n";  // 1
    cout << "Connected(0,3): " << dsu.connected(0, 3) << "\n";  // 1
    cout << "Components: " << dsu.count() << "\n";               // 3
    dsu.printParents();
    dsu.printRank();

    // DSU with size
    DSUSize dsz(5);
    dsz.unite(0, 1); dsz.unite(0, 2); dsz.unite(3, 4);
    cout << "Size of component 0: " << dsz.size(0) << "\n";  // 3
    cout << "Size of component 3: " << dsz.size(3) << "\n";  // 2

    // Kruskal's MST
    //   0 ---4--- 1
    //   |  \      |
    //   2   8     7
    //   |      \  |
    //   3 ---9--- 4
    vector<Edge> edges = {
        {0,1,4},{0,2,2},{1,3,7},{2,3,9},{0,3,8}
    };
    auto [mstWeight, mstEdges] = kruskal(4, edges);
    cout << "MST weight: " << mstWeight << "\n";  // 13
    cout << "MST edges: ";
    for (auto& e : mstEdges) cout << "(" << e.u << "," << e.v << "," << e.w << ") ";
    cout << "\n";

    // Detect cycle
    vector<pair<int,int>> cycleEdges = {{0,1},{1,2},{2,0}};
    cout << "Has cycle: " << hasCycle(3, cycleEdges) << "\n";  // 1

    // Redundant connection
    vector<vector<int>> redEdges = {{1,2},{1,3},{2,3}};
    auto red = findRedundantConnection(redEdges);
    cout << "Redundant edge: " << red[0] << " " << red[1] << "\n";  // 2 3

    return 0;
}
