#include <iostream>
using namespace std;

struct Student {
	string name;
	string address;
	int rollNo;
};

// If we remove & in below function, a new
// copy of the student object is created.
// We use const to avoid accidental updates
// in the function as the purpose of the function
// is to print s only.
void print(const Student &s)
{
	cout << s.name << " " << s.address << " " << s.rollNo
		<< '\n';
}

int main()
{
	Student Truong;
    Truong.address = "Thanh Hoa";
    Truong.name = "Vu Ngoc Truong";
    Truong.rollNo = 10;
    print(Truong);
    Student *ptrT;
    ptrT = &Truong;
    cout << ptrT->address << endl;
	return 0;
}
