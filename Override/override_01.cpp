
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
    void makeSound() override
    {
        cout << "The cat meows.\n";
    }
};

int main()
{
    Animal *animalPtr;
    Dog dog;
    Cat cat;

    animalPtr = &dog;
    animalPtr->makeSound();

    animalPtr = &cat;
    animalPtr->makeSound();

    return 0;
}
