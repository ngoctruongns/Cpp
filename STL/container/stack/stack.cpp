/**
 * empty() – Returns whether the stack is empty – Time Complexity : O(1)
 * size() – Returns the size of the stack – Time Complexity : O(1)
 * top() – Returns a reference to the top most element of the stack – Time Complexity : O(1)
 * push(g) – Adds the element ‘g’ at the top of the stack – Time Complexity : O(1)
 * pop() – Deletes the most recent entered element of the stack – Time Complexity : O(1)
 */

#include <iostream>
#include <stack>
#include <vector>
#include <list>

using namespace std;

int main()
{
    // stack with underlying container default is deque
    stack<int> st1;
    stack<int> st2;
    // stack with underlying container is vector
    std::stack<int, std::vector<int>> stackWithVector;
    // stack with underlying container is vector
    std::stack<int, std::list<int>> stackWithList;
    st1.push(10);
    st1.push(15);
    st1.push(20);

    cout << st1.size() << endl;
    cout << st1.top() << endl;
    st1.pop();
    cout << st1.size() << endl;
    cout << st1.top() << endl;

    st2.push(11);
    st2.push(12);
    st2.push(13);
    st2.push(14);

    // swap stack
    st1.swap(st2);
    cout << st1.size() << endl;
    cout << st1.top() << endl;

    cout << st1.empty() << endl;
    st1.emplace(5);
    cout << st1.size() << endl;




    return 0;
}