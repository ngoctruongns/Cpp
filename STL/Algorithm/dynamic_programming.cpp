#include <iostream>
#include <vector>
#include <algorithm>

class Solution {
public:
    int maxProfit(std::vector<int>& prices) {
        if (prices.empty()) return 0;

        int firstBuy = INT_MIN, firstSell = 0;
        int secondBuy = INT_MIN, secondSell = 0;

        for (int price : prices) {
            firstBuy = std::max(firstBuy, -price);
            firstSell = std::max(firstSell, firstBuy + price);
            secondBuy = std::max(secondBuy, firstSell - price);
            secondSell = std::max(secondSell, secondBuy + price);
            std::cout << secondSell << std::endl;
        }

        return secondSell;
    }
};

int main() {
    Solution sol;
    std::vector<int> prices = {1,2,4,2,5,7,2,4,9,0};
    std::cout << "Maximum Profit: " << sol.maxProfit(prices) << std::endl;
    return 0;
}
