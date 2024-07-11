#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
class Solution {
public:
    int fib(int n) {
        if (n <= 0) return 0;
        if (n == 1) return 1;
        return (fib(n-1) + fib(n-2));

    }

    int fibOptimize(int n) {
        if (n < 2) return n;
        int* FIB = new int[n+1];
        FIB[0] = 0;
        FIB[1] = 1;
        for (int i=2; i <=n; i++) {
            FIB[i] = FIB[i-1] + FIB[i-2];
        }
        int res = FIB[n];
        delete[] FIB;
        return res;
    }
};

int main() {
    Solution sol;
    int N = 45;
    std::cout << "Fibonacy: " << sol.fib(N) << std::endl;
    // std::cout << "Fibonacy: " << sol.fibOptimize(N) << std::endl;
    return 0;
}
