#include <iostream>
#include <map>
#include <unordered_map>
using namespace std;

int main()
{
    unordered_map<int, int> mp;
    mp[1] = 11;
    mp[2] = 12;
    mp[3] = 13;
    mp[4] = 14;

    for (pair<int, int> it : mp) {
        cout << it.first << ", " << it.second << endl;
    }

    cout << mp[1] << endl;
    cout << mp[3] << endl;
    cout << mp[5] << endl;
    return 0;
}