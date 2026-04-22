// main.cpp
#include "IPlugin.hpp"
#include <dlfcn.h>      // Trên Linux. Trên Windows thì dùng <windows.h> với LoadLibrary
#include <iostream>

int main() {
    void* handle = dlopen("./libHelloPlugin.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Không thể nạp plugin: " << dlerror() << "\n";
        return 1;
    }

    // Reset lỗi trước khi lấy symbol
    dlerror();
    using CreateFunc = IPlugin*();
    CreateFunc* create = (CreateFunc*) dlsym(handle, "create_plugin");

    const char* err = dlerror();
    if (err) {
        std::cerr << "Không tìm thấy create_plugin: " << err << "\n";
        dlclose(handle);
        return 1;
    }

    IPlugin* plugin = create();
    std::cout << "Đang chạy plugin: " << plugin->name() << "\n";
    plugin->run();

    delete plugin;
    dlclose(handle);
    return 0;
}
