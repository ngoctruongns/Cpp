#include <iostream>
#include <algorithm>
#include <queue>
using namespace std;

int main()
{
    const int N = 10;
    int arr[N] = {3,5,2,6,1,9,16,19};
    int k = 3;
    arr[8] = 11;
    arr[9] = 21;
    // std::sort(std::begin(arr), std::end(arr));
    // std::partial_sort(std::begin(arr), std::begin(arr) + k, std::end(arr));
    std::partial_sort(arr, arr + k, arr+10);

    // use heap struct
    priority_queue<int, vector<int>, greater<int>> pq;
    for (int i=0; i<N; i++) {
        pq.push(arr[i]);
        if (pq.size() > k) {
            pq.pop();
        }
    }

    vector<int> res;
    for (int i = 0; i < k; i++) {
        res.push_back(pq.top());
        pq.pop();

    }
    std::reverse(res.begin(), res.end());
    // print queue
    for (int it : res) {
        cout << it << ", ";
    }
    cout << endl;


    // print array
    for (auto item : arr)
    {
        std::cout << item << ", ";
    }
    std::cout<< std::endl;

    // std::cout << std::begin(arr) << std::endl;
    // std::cout << (std::begin(arr)+ 1) << std::endl;

    return 0;
}