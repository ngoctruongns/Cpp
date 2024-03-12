#include <iostream>
#include <string>

using namespace std;

int main()
{
    // Khởi tạo chuỗi
    string str = "Hello, world!";
    string str2("Xin chao, Viet nam");
    string str3 = "Xin chao, viet nam";
    string str4(str2);
    string str5 = str3;

    string *ptrStr;
    ptrStr = &str4;
    printf("Vi tri chuoi str3: %p \n", &str3);
    printf("Vi tri chuoi str5: %p \n", &str5);

    cout << "Chuoi ban dau: " << str << " at " << &str << endl;
    cout << "Chuoi thu 2: " << str2 << " at " << &str2 << endl;
    cout << "Chuoi thu 3: " << str3 << " at " << &str3 << endl;
    cout << "Chuoi thu 4: " << str4 << " at " << &str4 << endl;
    cout << "Chuoi thu 5: " << str4 << " at " << &str5 << endl;

    // Lay do dai chuoi
    int len = str.length();
    cout << "Do dai chuoi: " << len << endl;

    // Tìm ký tự
    string delimiter = ",";
    size_t pos = str.find(delimiter);
    cout << "Vi tri tim duoc: " << pos << endl;

    // Truy cap phan tu dau tien, cuoi cung
    cout << "Iterator dau tien: " << *str.begin() << endl;
    // cout << "Iterator cuoi cung: " << *str.end() << endl;
    cout << "Char cuoi cung: " << str.back() << endl;

    // Tach chuoi
    string token = str.substr(0, pos);  // (start, num_of_char)
    cout << "Token dau tien: " << token << endl;
    string token2 = str.substr(0);  // Mac dinh lay den het chuoi
    cout << "Token thu 2: " << token2 << endl;
    string token3 = str.substr(3, 5);
    cout << "Token thu 3: " << token3 << endl;


    // Them chuoi
    str.append(" Goodbye!");
    cout << "Chuoi sau khi them: " << str << endl;

    // So sanh chuoi
    if (str2 == str3)
    {
        cout << "str2 bang str3" << endl;
    }
    else
    {
        cout << "str2 khac str3" << endl;
    }

    // So sanh chuoi
    if (str2.compare(str3) == 0)
    {
        cout << "str2 bang str3" << endl;
    }
    else
    {
        cout << "str2 khac str3" << endl;
    }

    return 0;
}
