#include <iostream>
#include <algorithm>
#include <vector>
#include <numeric>

using namespace std;
using ll = long long;

// Edge data struct
struct Edge {
    int u;
    int v;
    ll w;
    bool operator<(const Edge& other) {
        return w < other.w;
    };
};

// Union-find impl
struct DSU {
    vector<int> p;
    vector<int> sz;

    DSU(int n) {
        sz.resize(n+1, 1);
        p.resize(n+1);
        iota(p.begin(), p.end(), 0);
    }
    
    int find(int u) {
        if (u == p[u]) return u;
        return p[u] = find(p[u]);
    }
    
    bool join(int u, int v) {
        int x = p[u];
        int y = p[v];
        if (x == y) return false;
    
        // Union u, v
        if (sz[x] < sz[y]) swap(x, y);
        p[y] = x;
        sz[x] += sz[y];
        return true;
    }

};

int main(void)
{
    ios_base::sync_with_stdio(0);
    cin.tie(0);
    
    // Get testcase 
    freopen("input.txt", "r", stdin);

    int n, m; // n dinh, m canh
    cin >> n >> m;

    vector<Edge> edges(m);

    for (auto& dt : edges) {
        cin >> dt.u >> dt.v >> dt.w;
    }

    // Sort edge by w
    sort(edges.begin(), edges.end(), greate<Edge>);

    // Print to debug
    cout << n << ", " << m << endl;
    for (const auto& dt : edges) {
        cout << dt.u << ", " << dt.v << ", " << dt.w << endl;
    }

    // Kruskal algorithm:
    int cnt = 0;
    ll msum = 0;
    DSU dsu(n);
    for (auto& edge : edges) {
        // calc
        if (dsu.join(edge.u, edge.v)) {
            cout << edge.u << " <-> " << edge.v << endl;
            msum += edge.w;
            cnt++;
            if (cnt == n-1) break;
        }
    }

    // Check mst or not
    if (cnt == n - 1) {
        cout << msum << endl;
    } else {
        cout << "No valid MST! \n";
    }

    return 0;
}