
// constructing vectors
#include <iostream>
#include <vector>

int main()
{
    // constructors used in the same order as described above:
    std::vector<int> first;                               // empty vector of ints
    std::vector<int> five(5);                       // vector with 5 element init value is 0
    std::vector<int> second(4, 100);                      // four ints with value 100
    std::vector<int> third(second.begin(), second.end()); // iterating through second
    std::vector<int> fourth(third);                       // a copy of third

    for (int it : five) {
        std::cout<< it <<", ";
    }

    // the iterator constructor can also be used to construct from arrays:
    int myints[] = {16, 2, 77, 29};
    std::vector<int> fifth(myints, myints + sizeof(myints) / sizeof(int));

    std::cout << "The contents of fifth are:" << '\n';
    for (std::vector<int>::iterator it = fifth.begin(); it != fifth.end(); ++it)
        {
            std::cout << ' ' << *it;
            std::cout << " dia chi &it: " << &it ;
            std::cout << " &*it: " << &(*it) << std::endl;
        }

    std::cout << '\n';
    for (size_t i = 0; i < 4; i++)
    {
        std::cout << second[i] << '\n';
        /* code */
    }

    std::vector<int> *field_data;
    field_data = &fifth;
    std::vector<int> temp(3,6);
    /* Chỉ mục */
    temp[0] = 1;
    temp[1] = 2;

    /* At */
    std::cout << "At" <<  temp.at(2) << std::endl;

    *field_data = temp;

    std::cout << "The contents of fifth after are:" << '\n';
    for (std::vector<int>::iterator it = fifth.begin(); it != fifth.end(); ++it)
        std::cout << ' ' << *it;

    return 0;
}