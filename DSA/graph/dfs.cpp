#include <iostream>
#include <vector>
#include <list>

using namespace std;
vector<vector<int>> adjMx = {
    {0, 1, 1, 0, 0},
    {0, 0, 1, 1, 0},
    {0, 0, 0, 1, 1},
    {0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0}};

int visited[6] = {0};
vector<vector<int>> adjList = {
    {1, 2}, // 0
    {0, 3},    // 1
    {0, 4, 5}, // 2
    {1},    // 3
    {2},     // 4
    {2},     // 5
};

    // Create a graph given in the diagram
    /*     0
          / \
         1   2
        /   / \
       3   4   5
    */
void dfs(int i)
{
    if (!visited[i]) {
        visited[i] = 1;
        cout << i << endl;
        for (int j : adjList[i]) {
            dfs(j);
        }

    }
}

int main()
{

    dfs(0);
    /*
    0
    1
    3
    2
    4
    5
    */
    return 0;
}