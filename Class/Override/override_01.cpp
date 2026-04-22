
#include <iostream>
using namespace std;

class Animal
{
public:
    virtual void makeSound()
    {
        cout << "The animal makes a sound.\n";
    }
    // Hàm hủy ảo để đảm bảo rằng khi đối tượng của lớp con bị hủy,
    // hàm hủy của lớp con sẽ được gọi, giúp giải phóng tài nguyên
    // nếu cần thiết.
    // Nếu không có hàm hủy ảo, khi đối tượng của lớp con bị hủy,
    // chỉ hàm hủy của lớp cha sẽ được gọi, có thể dẫn đến rò rỉ bộ nhớ
    // hoặc không giải phóng tài nguyên đúng cách.
    // Điều này rất quan trọng trong lập trình hướng đối tượng,
    // đặc biệt khi làm việc với các lớp trừu tượng hoặc khi có nhiều lớp
    // kế thừa từ một lớp cha.

    ~Animal() // Destructor
    {
        cout << "Animal destroyed.\n";
    }
};

class Dog : public Animal
{
public:
    void makeSound() override
    {
        cout << "The dog barks.\n";
    }

    ~Dog()
    {
        cout << "Dog destroyed.\n";
    }
};

class Cat : public Animal
{
public:
    // từ khóa override để giúp chương trình sáng sửa và trình biên dịch báo lỗi nếu lớp cha không có hàm virtual,
    // bỏ nó đi chương trình vẫn hoạt động.
    void makeSound() override
    {
        cout << "The cat meows.\n";
    }

    void sleep()
    {
        cout << "The cat sleeps.\n";
    }
};

int main()
{
    Animal *animalPtr;
    Animal animal;
    Dog dog;
    Cat cat;

    animalPtr = &animal;
    animalPtr->makeSound();

    animalPtr = &dog;
    animalPtr->makeSound();

    animalPtr = &cat;
    animalPtr->makeSound();
    // animalPtr->sleep(); // Lỗi: lớp cha không có hàm sleep()

    Dog *dogPtr;
    dogPtr = &dog;
    dogPtr->makeSound();

    //! Con trỏ của lớp con không thể trỏ tới lớp cha được
    // dogPtr = &animal;
    // dogPtr->makeSound();

    return 0;
}
