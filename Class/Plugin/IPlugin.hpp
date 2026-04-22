// IPlugin.hpp
#pragma once
#include <string>

class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual std::string name() const = 0;
    virtual void run() = 0;
};

// Hàm tạo plugin sẽ được export từ mỗi plugin .so/.dll
extern "C" IPlugin* create_plugin();
