#include <iostream>
#include <set>
using namespace std;
int main()
{

    // int arr[10] =  {1,2,2,3,4,5,5,6,7,8};
    // std::set<int> se;

    // for (int i : arr){
    //     se.insert(i);
    // }

    std::set<int> se = {1,2,2,3,4,5,5,6,7,8};
    std::cout << se.size() << std::endl; // 8

    // for (int j: se){
    //     std::cout << j << ", ";
    // }
    // std::cout << endl;
    // int max_size = se.max_size();
    // std::cout << max_size << std::endl;

    std::set<int>::iterator it;
    it = se.begin();
    std::cout << *it << std::endl;
    --it;
    std::cout << *it << std::endl;
    --it;
    std::cout << *it << std::endl;
    --it;
    std::cout << *it << std::endl;



    return 0;
}