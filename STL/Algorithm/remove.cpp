// remove algorithm example
#include <iostream>  // std::cout
#include <algorithm> // std::remove
#include <vector>   // std::vector

int main()
{
    // Array
    int myints[] = {10, 20, 30, 30, 20, 10, 10, 20}; // 10 20 30 30 20 10 10 20

    // bounds of range:
    int *pbegin = myints;                              // ^
    int *pend = myints + sizeof(myints) / sizeof(int); // ^                       ^

    pend = std::remove(pbegin, pend, 20); // 10 30 30 10 10 ?  ?  ?
                                          // ^              ^
    std::cout << "range contains:";
    for (int *p = pbegin; p != pend; ++p)
        std::cout << ' ' << *p;
    std::cout << '\n';

    // Vector
    std::vector<int> myvector = {10, 20, 30, 30, 20, 10, 10, 20}; // 10 20 30 30 20 10 10 20

    auto itend = std::remove(myvector.begin(), myvector.end(), 20); // 10 30 30 10 10 ?  ?  ?
                                                                    // ^              ^
    myvector.erase(itend, myvector.end()); // remove the "removed" elements
    std::cout << "vector contains:";
    for (const auto &value : myvector)
        std::cout << ' ' << value;
    std::cout << '\n';

    return 0;
}