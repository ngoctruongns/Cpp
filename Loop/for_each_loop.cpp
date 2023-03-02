#include <iostream>
using namespace std;

int main()
{
	int arr[] = { 14, 3, 6, 27, 12 };
	for (int item: arr)
    // for (auto const& item: arr) // Dùng auto để tự động tìm kiểu dữ liệu
	{
		// biến item đại diện cho phần tử mảng ở mỗi vòng lặp
		cout << item << " ";
	}
	cout << endl;

	return 0;
}
// Output: 14 3 6 27 12