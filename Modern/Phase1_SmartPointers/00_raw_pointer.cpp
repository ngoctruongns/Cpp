#include <iostream>
#include <memory>

using namespace std;

int main(void)
{
    int arr[10] = {0,};
    auto ptr = make_unique<int[]>(10);
    for (int i =0; i<10; i++) {
        ptr[i] = i + 1;
    }

    cout << ptr[0] << endl;

    int *raw = ptr.get();
    cout << raw[1] << endl;

    return 0;
}