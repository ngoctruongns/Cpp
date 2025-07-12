
#include <iostream>
using namespace std;

class Animal
{
public:
    void makeSound()
    {
        cout << "The animal makes a sound.\n";
    }

    ~Animal() // Destructor
    {
        cout << "Animal destroyed.\n";
    }
};

class Dog : public Animal
{
public:
    void makeSound() // override từ lớp cha
    // Nếu không có từ khóa override thì chương trình vẫn chạy bình thường,
    // nhưng nếu lớp cha có hàm virtual thì chương trình sẽ không chạy đúng.

    // void makeSound() override
    // Nếu lớp cha không có hàm virtual thì chương trình sẽ báo lỗi.
    // Từ khóa override giúp trình biên dịch kiểm tra xem hàm này có thực sự ghi đè (override) một hàm ảo (virtual) từ lớp cha hay không

    {
        cout << "The dog Gow.\n";
    }

    void bark()
    {
        cout << "The dog is barking.\n";
    }
};

class Cat : public Animal
{
public:
    // từ khóa override để giúp chương trình sáng sửa và trình biên dịch báo lỗi nếu lớp cha không có hàm virtual,
    // bỏ nó đi chương trình vẫn hoạt động.
    void makeSound()
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
    animalPtr->makeSound(); // Outputs: "the Animal makes a sound." because makeSound() is not virtual in Animal class

    animalPtr = &cat;
    animalPtr->makeSound();

    Dog *dogPtr;
    dogPtr = &dog;
    dogPtr->makeSound(); // Outputs: "The dog Gow."

    return 0;
}
