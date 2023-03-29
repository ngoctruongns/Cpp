#include <iostream>
#include <vector>
#include <algorithm>

int main()
{
    // [capture mode](parameters) -> return type { function body }

    // auto sum = [](int a, int b) -> int { return a + b; };
    auto sum = [](int a, int b) { return a + b; };
    int result = sum(3, 4); // result = 7
    std::cout << "result= "<< result;

    std::vector<int> v = {4, 2, 5, 1, 3};
    std::sort(v.begin(), v.end(), [](int a, int b) { return a < b; });
    // sau khi sắp xếp, mảng v trở thành {1, 2, 3, 4, 5}


    return 0;
}
