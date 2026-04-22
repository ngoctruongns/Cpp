#include <iostream>
using namespace std;

template <size_t N>
class classArray
{
    public:
    classArray() = default;
    classArray(initializer_list<int> lst)
    {
        size_t i = 0;
        for (auto v : lst)
        {
            if (i < N)
                _arr[i++] = v;
        }
    }
    ~classArray() = default;

    int& operator[] (size_t index)
    {
        return _arr[index];
    }

    int at(size_t index)
    {
        if (index >= N)
            throw out_of_range("Index out of range");
        return _arr[index];
    }

private:
        int _arr[N];
};

int main()
{
    classArray<5> arr = {0,1,2,3,4};

    // Testing operator[]
    for (int i = 0; i < 5; i++)
        cout << arr[i] << " ";
    cout << endl;

    // Testing at() method
    cout << arr.at(2) << endl;

    // This will throw an exception
    try {
        cout << arr.at(10) << endl;
    } catch (const out_of_range& e) {
        cerr << e.what() << endl;
    }

    return 0;
}