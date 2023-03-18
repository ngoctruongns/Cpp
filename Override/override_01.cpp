
#include <iostream>
using namespace std;

class Animal
{
public:
    virtual void makeSound()
    {
        cout << "The animal makes a sound.\n";
    }
};

class Dog : public Animal
{
public:
    void makeSound() override
    {
        cout << "The dog barks.\n";
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

    Dog *dogPtr;
    dogPtr = &dog;
    dogPtr->makeSound();

    //! Con trỏ của lớp con không thể trỏ tới lớp cha được
    // dogPtr = &animal;
    // dogPtr->makeSound();

    return 0;
}
