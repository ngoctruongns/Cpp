#include <bits/stdc++.h>
using namespace std;

/*
Problem summary:
- N peaks, index 1..N, height A[i].
- We can add edge (i, j) only if j - i >= K.
- Edge cost = max(A[i..j]).
- Need minimum total cost to connect all vertices, or -1 if impossible.

Key idea:
Kruskal by increasing edge weight h.
Let G_h be graph containing all edges with cost <= h.
If C(h) = number of connected components in G_h, then
MST cost = sum over each height h of (C(prev) - C(h)) * h.

How to compute C(h) efficiently:
- Edges with cost <= h are exactly edges inside contiguous blocks where all A <= h.
- For a block length L, graph uses rule |u-v| >= K.
  Number of components in that block is:
    isolated = count of vertices v where v-K < 1 and v+K > L
             = size of intersection [1, L] with [L-K+1, K]
    comp(L) = isolated + (isolated < L ? 1 : 0)

Sweep heights ascending:
- Activate positions with A[pos] == h.
- Maintain active contiguous blocks with ordered set of intervals.
- Maintain sum(comp(length)) over active blocks.
- Inactive vertices are isolated components.
- So C(h) = activeComp + inactiveCount.

Complexity:
- O(N log N) time, O(N) memory.
*/

static inline long long blockComponents(int L, int K) {
    int left = max(1, L - K + 1);
    int right = min(L, K);
    long long isolated = (left <= right) ? (right - left + 1) : 0;
    return isolated + (isolated < L ? 1 : 0);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, K;
    cin >> N >> K;
    vector<int> A(N + 1);
    for (int i = 1; i <= N; i++) cin >> A[i];

    // Graph feasibility depends only on (N, K).
    // Connected iff N >= 2K.
    if (N < 2 * K) {
        cout << -1 << '\n';
        return 0;
    }

    map<int, vector<int>> byHeight;
    for (int i = 1; i <= N; i++) byHeight[A[i]].push_back(i);

    vector<char> active(N + 2, 0);
    set<pair<int, int>> segments; // active disjoint intervals [l, r]

    long long activeComp = 0;      // sum of components of active intervals
    long long inactiveCount = N;   // not activated yet
    long long prevC = N;           // C(previous processed height)
    long long answer = 0;

    auto removeSegment = [&](int l, int r) {
        activeComp -= blockComponents(r - l + 1, K);
        segments.erase({l, r});
    };

    auto addSegment = [&](int l, int r) {
        segments.insert({l, r});
        activeComp += blockComponents(r - l + 1, K);
    };

    for (auto &entry : byHeight) {
        int h = entry.first;
        const vector<int> &positions = entry.second;

        for (int x : positions) {
            bool hasLeft = active[x - 1];
            bool hasRight = active[x + 1];

            if (!hasLeft && !hasRight) {
                addSegment(x, x);
            } else if (hasLeft && !hasRight) {
                auto it = prev(segments.upper_bound({x - 1, INT_MAX}));
                int l = it->first;
                int r = it->second;
                removeSegment(l, r);
                addSegment(l, x);
            } else if (!hasLeft && hasRight) {
                auto it = segments.lower_bound({x + 1, -1});
                int l = it->first;
                int r = it->second;
                removeSegment(l, r);
                addSegment(x, r);
            } else {
                auto itL = prev(segments.upper_bound({x - 1, INT_MAX}));
                auto itR = segments.lower_bound({x + 1, -1});

                int l1 = itL->first, r1 = itL->second;
                int l2 = itR->first, r2 = itR->second;

                removeSegment(l1, r1);
                removeSegment(l2, r2);
                addSegment(l1, r2);
            }

            active[x] = 1;
            inactiveCount--;
        }

        long long curC = activeComp + inactiveCount;
        answer += (prevC - curC) * 1LL * h;
        prevC = curC;
    }

    // For safety; with N >= 2K it should end at 1 component.
    if (prevC != 1) {
        cout << -1 << '\n';
        return 0;
    }

    cout << answer << '\n';
    return 0;
}
