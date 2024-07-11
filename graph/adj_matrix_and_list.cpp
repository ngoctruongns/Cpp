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

vector<vector<int>> adjList;

vector<vector<int>> adjMx1;
vector<vector<int>> adjList1 = {
    {1, 2},
    {2, 3},
    {3, 4},
    {4},
    {},
};

void convertMatrixToList()
{

    for (int i = 0; i < adjMx.size(); i++)
    {
        vector<int> tmp;
        for (int j = 0; j < adjMx[0].size(); j++)
        {
            if (adjMx[i][j] == 1)
            {
                tmp.push_back(j);
            }
        }
        adjList.push_back(tmp);
    }
}

void convertListToMatrix()
{
    int N = adjList1.size();
    adjMx1 = vector<vector<int>>(N, vector<int>(N, 0));
    for (int i=0; i<N; i++) {
        for (int j:adjList1[i]) {
            adjMx1[i][j] = 1;
        }
    }
}

int main()
{
    convertMatrixToList();
    convertListToMatrix();

    // print adjList
    for (int i = 0; i < adjList.size(); ++i)
    {
        cout << i << ": ";
        for (int val : adjList[i])
        {
            cout << val << ", ";
        }
        cout << endl;
    }

    // print adjMx1
    for (auto rows : adjMx1) {
        for(int j:rows) {
            cout << j << ", ";
        }
        cout << endl;
    }

    return 0;
}