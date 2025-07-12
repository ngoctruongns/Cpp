#include <iostream>
#include <string>

class Animal
{
public:
    virtual void makeSound() const = 0; // Pure virtual function
    virtual ~Animal() = default; // Virtual destructor
};

class Dog : public Animal
{
public:
    void makeSound() const override
    {
        std::cout << "Woof!" << std::endl;
    }

    void bark() const
    {
        std::cout << "Dog is barking!" << std::endl;
    }

    ~Dog() override
    {
        std::cout << "Dog bye bye!" << std::endl;
    }
};

class Cat : public Animal
{
public:
    void makeSound() const override
    {
        std::cout << "Meow!" << std::endl;
    }
};

int main()
{
    Animal *pet  = new Dog(); // Using a pointer to the base class
    pet->makeSound(); // Outputs: Woof!
    delete pet; // Clean up

    Dog dog;
    Cat cat;

    pet = &dog; // Pointing to a Dog object
    pet->makeSound(); // Outputs: Woof!
    dog.bark(); // Outputs: Dog is barking!
    // pet->bark(); // Error: 'class Animal' has no member named 'bark'

    pet = &cat; // Pointing to a Cat object
    pet->makeSound(); // Outputs: Meow!


    return 0;
}
