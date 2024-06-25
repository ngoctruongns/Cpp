#include <iostream>
#include <vector>

int main(int argc, char *argv[])
{
    std::cout << "Hello World!" << std::endl;
    std::vector<int> vect;
    vect.push_back(1);
    vect.push_back(2);
    vect.push_back(3);
    vect.push_back(4);
    // print vector
    for (int i : vect)
    {
        std::cout << i << ", ";
    }
    std::cout << std::endl;
    return 0;
}
