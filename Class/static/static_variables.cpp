#include <iostream>
#include <string>
using namespace std;

/* * Ví dụ về biến tĩnh trong C++
 * Biến tĩnh là biến được chia sẻ giữa tất cả các đối tượng của lớp.
 * Chúng được sử dụng để theo dõi số lượng đối tượng đã được tạo từ lớp đó.
 */

class MyClass {
public:
    static int count;
    MyClass() {
        count++;
    }
};

int MyClass::count = 0; // Khởi tạo biến tĩnh count

/* Ví dụ về công dụng khác của biến tĩnh:
 * Biến tĩnh có thể được sử dụng để lưu trữ các giá trị cấu hình hoặc trạng thái toàn cục.
 * Chúng có thể được truy cập mà không cần tạo đối tượng của lớp.
 */
class Config {
public:
    static string appName;
    static void setAppName(const string& name) {
        appName = name;
    }
    static string getAppName() {
        return appName;
    }
};
string Config::appName = "DefaultApp"; // Khởi tạo biến tĩnh appName

int main() {
    MyClass obj1;
    MyClass obj2;
    cout << "Number of objects: " << MyClass::count << endl; // In ra số lượng đối tượng đã được tạo

    cout << "App Name: " << Config::getAppName() << endl; // In ra tên ứng dụng mặc định
    Config::setAppName("MyApplication");
    cout << "App Name after: " << Config::getAppName() << endl; // In ra tên ứng dụng
    return 0;
}