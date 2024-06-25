#include <vector>
#include <iostream>
#include <algorithm>

// struct strVect
// {
//     int value;
//     strVect * ptrStr;
// };

int rmDuplicates(std::vector<int> &nums)
{
    for (auto it = nums.begin(); it != nums.end(); ++it)
    {
        if (it != nums.begin())
        {
            if (*it == (*(it - 1)))
            {
                nums.erase(it--);
            }
        }
    }
    return nums.size();
}

int main()
{
    std::vector<int> vect = {1, 5, 5, 2, 8, 2, 3, 4, 4, 4, 5};

    // strVect strVectVal;
    // std::vector<strVect> vectStruct;
    // strVectVal.value = 1;
    // strVectVal.ptrStr = nullptr;
    // vectStruct.push_back(strVectVal);
    // strVectVal.value = 2;
    // strVectVal.ptrStr = &vectStruct[0];

    // vector iterator ( random access iterator)
    std::vector<int>::iterator it;
    // // assign for it
    // it = vect.begin() + 2;
    // // Erase element at it
    // vect.erase(it);  // erase 3
    // it++;
    // vect.erase(it);

    std::sort(vect.begin(), vect.end());

    int afterSize = rmDuplicates(vect);
    std::cout << "after size: " << afterSize << std::endl;

    for (int i : vect)
    {
        std::cout << i << ", ";
    }

    return 0;
}