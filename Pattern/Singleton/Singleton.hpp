#ifndef SINGLETON_HPP
#define SINGLETON_HPP

/*
Template-based Singleton for easy reuse
Usage:
    class MyClass : public Singleton<MyClass> {
    private:
        MyClass() { }
        friend class Singleton<MyClass>;
    public:
        void doSomething() { }
    };

    MyClass::getInstance().doSomething();
*/

template<typename T>
class Singleton {
protected:
    Singleton() = default;

    // Prevent copying
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

public:
    // Virtual destructor for polymorphism
    virtual ~Singleton() = default;

    // Get singleton instance
    static T& getInstance() {
        static T instance;
        return instance;
    }
};

#endif // SINGLETON_HPP
