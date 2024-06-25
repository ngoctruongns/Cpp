#include <iostream>
#include <vector>
#include <stack>

using namespace std;

int main()
{
    vector<int> nums = {2,4,3,3,5,4,9,6,8};
    int k = 4;
    vector<int> stack;
    int to_remove = nums.size() - k;

    for (int num : nums) {
        while (!stack.empty() && to_remove > 0 && stack.back() > num) {
            stack.pop_back();
            to_remove--;
        }
        stack.push_back(num);
    }

    vector<int>(stack.begin(), stack.begin() + k);
    return 0;
}