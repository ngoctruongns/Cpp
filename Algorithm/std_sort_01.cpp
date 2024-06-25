#include <iostream>
#include <algorithm>
#include <set>

struct data_t
{
    int a;
    int b;
    bool operator<(const data_t &x) const {
        return a < x.a;
    }
};

int main()
{
    int arr1[5] = {1,4,3,2,5};
    int arr2[5] = {22,14,12,16,11};
    data_t dt[5] = {0};
    std::set<data_t> setdt;

    for (int i = 0; i < 5; i++)
    {
        dt[i].a = arr1[i];
        dt[i].b = arr2[i];

    }
        setdt.insert(dt[1]);
        setdt.insert(dt[2]);
        if ((*setdt.begin()) < (*setdt.end())) {
            std::cout << "operator OK" << std::endl;

        }

    std::sort(dt, dt+5);
    // std::sort(dt, dt+5, [](data_t x, data_t y) {
    //     return x.b > y.b;
    // });

    for (data_t id : dt) {
        std::cout << id.a << ", "<< id.b << std::endl;
    }

    // print set
    std::cout << "print set" << std::endl;
    for (data_t id : setdt) {
            std::cout << id.a << ", "<< id.b << std::endl;
        }



    return 0;
}