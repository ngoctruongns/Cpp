#include <iostream>
#include <unordered_set>

int main() {
    std::unordered_set<int> mySet = {2, 3, 5, 8, 3};

    // In các phần tử trong unordered_set
    std::cout << "Các phần tử trong unordered_set: ";
    for (const int& num : mySet) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    // Kiểm tra xem phần tử có trong unordered_set hay không
    int searchElement = 3;
    if (mySet.find(searchElement) != mySet.end()) {
        std::cout << searchElement << " có trong unordered_set" << std::endl;
    } else {
        std::cout << searchElement << " không có trong unordered_set" << std::endl;
    }

    return 0;
}
