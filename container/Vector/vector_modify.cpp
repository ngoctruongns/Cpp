// vector assign
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> first;
    std::vector<int> second;
    std::vector<int> third;

    /* Assign */
    first.assign(7, 100); // 7 ints with a value of 100

    std::vector<int>::iterator it;
    it = first.begin() + 1;

    second.assign(it, first.end() - 1); // the 5 central values of first
    // second.assign(0,0);

    /* Indicator */
    second[0] = 1;
    std::cout << second[0] << std::endl;

    int myints[] = {1, 3, 5};
    third.assign(myints, myints + 3); // assigning from array.

    /* Size and Capacity*/

    std::cout << "Size of first: " << int(first.size()) << '\n';
    std::cout << "capacity of first: " << int(first.capacity()) << '\n';
    std::cout << "Size of second: " << int(second.size()) << '\n';
    std::cout << "capacity of second: " << int(second.capacity()) << '\n';
    std::cout << "Size of third: " << int(third.size()) << '\n';
    std::cout << "capacity of third: " << int(third.capacity()) << '\n';

    /* Print vector */
    // // for (auto it=third.end(); it != third.begin(); it--)
    // for (int i=3; i >0; i--)
    // {
    //     std::cout << third[i-1] << ",";
    // }
    // std::cout << std::endl;

    /* Push_back*/
    second.push_back(8);
    second.push_back(9);
    std::cout << "Size of second: " << int(second.size()) << '\n';
    std::cout << "capacity of second: " << int(second.capacity()) << '\n';
    std::cout << "max_size of second: " << int(second.max_size()) << '\n';

    /* Shirk to fit */
    // second.shrink_to_fit();
    // std::cout << "Size of second: " << int(second.size()) << '\n';
    // std::cout << "capacity of second: " << int(second.capacity()) << '\n';

    /* at, indicator*/
    std::cout << "at of second: " << int(second.at(5)) << '\n';
    std::cout << "indicator of second: " << int(second[5]) << '\n';

    /* front back */
    std::cout << "front: " << second.front() << ", back: " << second.back()<< '\n';

    /* begin end */
    std::cout << "begin: " << *second.begin() << ", end: " << *second.end()<< '\n';

    /* insert*/
    second.insert(second.begin() + 2, {3,4,5});
    for (auto it=second.begin(); it != second.end(); it++)
    // for (auto it=second.begin(); it != second.end(); it++)
    {
        std::cout << *it << ",";
    }
    std::cout << std::endl;

    return 0;
}
// Size of first: 7
// Size of second: 5
// Size of third: 3