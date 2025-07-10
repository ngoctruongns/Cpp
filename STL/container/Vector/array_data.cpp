// constructing vectors
#include <iostream>
#include <vector>
using namespace std;
int main()
{
    int num_job = 3;
    int temp_field[]= {1,2,3}; // tạo mảng job

    cout << "field: ";
    for ( int j=0; j< num_job; j++)
    {
        printf("temp [%d]= %d; ", j, temp_field[j]);
        // cout << temp_field[j];
    }
    cout << endl;

    std::vector<int8_t> fifth;             // empty vector of ints
    // std::vector<int> fifth(temp_field, temp_field + sizeof(temp_field) / sizeof(int));
    fifth.assign(temp_field, temp_field + sizeof(temp_field) / sizeof(int));
    std::cout << "The contents of fifth are:" << '\n';
    for (std::vector<int8_t>::iterator it = fifth.begin(); it != fifth.end(); ++it)
        std::cout << ' ' << int(*it);

    std::cout << '\n';

    return 0;
}
