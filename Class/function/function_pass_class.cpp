#include <iostream>
using namespace std;

// Định nghĩa lớp
class Rectangle
{
private:
    int length;
    int width;

public:
    Rectangle(int l, int w)
    {
        length = l;
        width = w;
    }

    int area()
    {
        return length * width;
    }

    void setLength(int len)
    {
        length = len;
    }

    int getLength()
    {
        return length;
    }
};

// Hàm tính diện tích hình chữ nhật
void calculateArea(Rectangle obj)
{
    cout << "Diện tích của hình chữ nhật là: " << obj.area() << endl;
}

void changeLengthValue(Rectangle obj)
{
    obj.setLength(100);
}

void changeLengthReference(Rectangle &obj)
{
    obj.setLength(100);
}

int main()
{
    // Tạo đối tượng hình chữ nhật
    Rectangle rect(5, 10);

    // Gọi hàm tính diện tích và truyền đối tượng vào
    calculateArea(rect);
    // >>>>>>> Diện tích của hình chữ nhật là: 50

    // Truyền đối tượng dạng giá trị
    changeLengthValue(rect);
    cout << "Length new value: " << rect.getLength() << endl;
    // >>>>>> Length new value: 5

    // Truyền đối tượng dạng tham chiếu
    changeLengthReference(rect);
    cout << "Length new value reference: " << rect.getLength() << endl;
    // >>>>  Length new value reference: 100


    return 0;
}
