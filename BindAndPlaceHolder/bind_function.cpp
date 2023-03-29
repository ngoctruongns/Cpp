// C++ code to demonstrate bind() and
// placeholders
#include <iostream>
#include <functional> // for bind()
using namespace std;

// for placeholders
using namespace std::placeholders;

// Driver function to demonstrate bind()
void func(int a, int b, int c)
{
	cout << (a - b - c) << endl;
}

int main()
{
	// for placeholders
	using namespace std::placeholders;

	// Use of bind() to bind the function
	// _1 is for first parameter and assigned
	// to 'a' in above declaration.
	// 2 is assigned to b
	// 3 is assigned to c
	auto fn1 = bind(func, _1, 2, 3);

	// 2 is assigned to a.
	// _1 is for first parameter and assigned
	// to 'b' in above declaration.
	// 3 is assigned to c.
	function<void(int)> fn2 = bind(func, 2, _1, 3);

    // using a function pointer
    void (*func_ptr)(int, int, int) = &func;
    auto fn3 = bind(func_ptr, 2, placeholders::_1, 3);

	// calling of modified functions
	fn1(10);
	fn2(10);
	fn3(10);

	return 0;
}
