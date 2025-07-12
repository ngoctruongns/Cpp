
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

    // ~Animal() // Destructor
    virtual ~Animal() // Destructor
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

int main()
{
    Animal *animalPtr;
    animalPtr = new Dog(); // Using a pointer to the base class
    animalPtr->makeSound(); // Outputs: The dog barks.
    delete animalPtr; // Clean up

    // Dog dog;
    // animalPtr = &dog;
    // animalPtr->makeSound();

    return 0;
}
