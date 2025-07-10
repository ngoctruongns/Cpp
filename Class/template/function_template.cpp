#include <iostream>

// void Print(int value)
// {
//     std::cout << value << std::endl;
// }

// void Print(float value)
// {
//     std::cout << value << std::endl;
// }

template <typename T>

void Print(T value)
{
    std::cout << value << std::endl;
}

// function template
template <typename X>
X max(X a, X b)
{
    return (a>b) ? a : b;
}

// Use class 
template <class T> void bubbleSort(T a[], int n)
{
    for (int i = 0; i < n - 1; i++)
        for (int j = n - 1; i < j; j--)
            if (a[j] < a[j - 1])
                std::swap(a[j], a[j - 1]);

    std::cout << " Sorted array : ";
    for (int i = 0; i < n; i++)
        std::cout << a[i] << " ";
    std::cout << std::endl;
}
int main(void)
{
    int x = 10;
    Print(5);
    Print(x);
    Print(6.3f);
    Print('A');
    std::cout << max(5,7) << std::endl;
    // calls template function
    int a[5] = { 10, 5, 30, 40, 20 };
    int n = sizeof(a) / sizeof(a[0]);
    bubbleSort<int>(a, n);

    return 0;
}