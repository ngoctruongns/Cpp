// pointer to classes example
// Toán tử thành viên dot (.) và arrow (->)
// được sử dụng để tham chiếu các thành viên riêng lẻ của lớp,
// cấu trúc struct và union trong C++.
// Toán tử dot được áp dụng cho đối tượng thực sự.
// Toán tử arrow được sử dụng với một con trỏ tới một đối tượng.

#include <iostream>
using namespace std;

class Rectangle
{
public:
    int width, height;

    Rectangle(int x, int y) : width(x), height(y) {}
    int area(void) { return width * height; }
};

int main()
{
    Rectangle obj(3, 4);
    Rectangle *foo, *bar, *baz;
    foo = &obj;
    bar = new Rectangle(5, 6);
    baz = new Rectangle[2]{{2, 5}, {3, 6}};
    cout << "obj's area: " << obj.area() << '\n';
    cout << "*foo's area: " << foo->area() << '\n';
    cout << "*bar's area: " << bar->area() << '\n';
    cout << "baz[0]'s area:" << baz[0].area() << '\n';
    cout << "baz[1]'s area:" << baz[1].area() << '\n';

    // thay đổi giá trị cho thành viên trong lớp
    obj.height = 5;     // truy cập đối tương thực sự
    obj.width  = 8;
    cout << "obj's area: " << obj.area() << '\n';
    bar->height = 7;    // truy cập bằng con trỏ
    bar->width  = 6;
    cout << "*bar's area: " << bar->area() << '\n';


    delete bar;
    delete[] baz;
    return 0;
}

// #include <iostream>

// using namespace std;

// class Box
// {
// public:
//     // Constructor definition
//     Box(double l = 2.0, double b = 2.0, double h = 2.0)
//     {
//         cout << "Constructor called." << endl;
//         length = l;
//         breadth = b;
//         height = h;
//     }
//     double Volume()
//     {
//         return length * breadth * height;
//     }

// private:
//     double length;  // Length of a box
//     double breadth; // Breadth of a box
//     double height;  // Height of a box
// };

// int main(void)
// {
//     Box Box1(3.3, 1.2, 1.5); // Declare box1
//     Box Box2(8.5, 6.0, 2.0); // Declare box2
//     Box *ptrBox;             // Declare pointer to a class.

//     // Save the address of first object
//     ptrBox = &Box1;

//     // Now try to access a member using member access operator
//     cout << "Volume of Box1: " << ptrBox->Volume() << endl;

//     // Save the address of second object
//     ptrBox = &Box2;

//     // Now try to access a member using member access operator
//     cout << "Volume of Box2: " << ptrBox->Volume() << endl;

//     return 0;
// }