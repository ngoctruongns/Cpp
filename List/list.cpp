#include <iostream>
#include <stdio.h>
#include <list>
using namespace std;

/**
 * @brief   display list
 *
 * @param list_num
 */
void display(list<int> list_num)
{
    /* display the elements of the list */
    cout << "List Elements: ";
    for(int number : list_num) {
        cout << number <<", ";
    }
    cout << endl;
}

int main()
{
    /* create the list */
    // default constructor
    list<int> myList;
    // fill constructor
    // myList1 được khởi tạo chứa 4 phần tử có giá trị 50
    list<int> myList1(4, 50);
    // copy constructor
    //tạo một list mới có 4 phần tử và đều có giá trị là 100
    list<int> myList(4, 100);
    //tạo một list mới với các phần tử và giá trị được chép từ myList vào
    list<int> copyMyList(myList);

    // list <int> numbers;
    list<int> numbers {1, 2, 3, 4};

    /* display the elements of the list */
    display(numbers);

    /* Push back, push front */
    numbers.push_back(100);
    numbers.push_front(10);
    display(numbers);

     // khai báo iterator
    // list<int>::iterator it = numbers.begin();
    list<int> :: iterator it;
    it = numbers.begin();

    cout << "iterator &: " << &it << endl;
    cout << "iterator &*: " << &(*it) << endl;
    printf("iterator: %p \n", &it);

    // di chuyển iterator đến vị trí thứ 2
    advance(it, 2);

    // hiển thị giá trị phần tử tại vị trí thứ 2
    cout << "Giá trị phần tử tại vị trí thứ 2 là: " << *it << endl;

    /* Insert */
    numbers.insert(it, {15,26,11});
    cout << "List insert: ";
    display(numbers);

    // Duyệt các phần tử sử dụng iterator
    for ( it = numbers.begin(); it != numbers.end(); it++)
    {
        cout << "phan tu: " << *it << " co dia chi "<< &(*it) << endl;
    }
    it = numbers.end();
    cout << "end list is:" << *it << " at addr " << &(*it) << endl;

    return 0;
}