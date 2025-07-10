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
    Dog dog;
    Cat cat;

    // Animal* animals[] = { &dog, &cat };

    // for (const auto& animal : animals)
    // {
    //     animal->makeSound();
    // }

    dog.makeSound(); // Outputs: Woof!
    cat.makeSound(); // Outputs: Meow!

    return 0;
}
