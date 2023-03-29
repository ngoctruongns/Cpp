#include <iostream>
#include <algorithm>

int main()
{
    int arr[10] = {3,5,2,6,1,9};
    arr[8] = 11;
    arr[9] = 21;
    std::sort(std::begin(arr), std::end(arr));
    std::cout << std::begin(arr) << std::endl;
    std::cout << (std::begin(arr)+ 1) << std::endl;

    for (auto item : arr)
    {
        std::cout << item ;
    }
    std::cout<< std::endl;
    return 0;
}