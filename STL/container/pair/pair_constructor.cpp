

// CPP program to illustrate Pair in STL
#include <iostream>

using namespace std;

// Driver Code
int main()
{
    // defining a pair
    pair<int, char> PAIR1;

    // first part of the pair
    PAIR1.first = 100;
    // second part of the pair
    PAIR1.second = 'G';

    cout << PAIR1.first << " ";
    cout << PAIR1.second << endl;

    pair<int, char> pair2(10, 'a');
    pair<int, char> pair3 = {2, 'b'};
    pair<int, char> pair4;
    pair4 = {3, 'c'};

    pair<int, char> pair5;
    pair5 = make_pair(4, 'd');
    cout << pair2.first << ", "<< pair2.second << endl;
    cout << pair3.first << ", "<< pair3.second << endl;
    cout << pair4.first << ", "<< pair4.second << endl;
    cout << pair5.first << ", "<< pair5.second << endl;
    // cout << pair6.first << ", "<< pair6.second << endl;


    return 0;
}