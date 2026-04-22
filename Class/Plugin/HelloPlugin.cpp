// HelloPlugin.cpp
#include "IPlugin.hpp"
#include <iostream>

class HelloPlugin : public IPlugin
{
public:
    std::string name() const override
    {
        return "HelloPlugin";
    }

    void run() override
    {
        std::cout << "Hello from plugin!\n";
    }
};

extern "C" IPlugin *create_plugin()
{
    return new HelloPlugin();
}
