#include <iostream>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

int main()
{
    // string s;
    string s = "Ngon ngu lap trinh \nC++";
    string word;
    vector<string> v;
    int count = 0;
    string ls;

    cout << "string s: " << s << endl;

    // sub string
    string sub = s.substr(3); // from 3 to end of string
    cout << sub << endl;
    sub = s.substr(3, 6);  // from 3 with 6 character
    cout << sub << endl;


    // creat string stream
    stringstream ss(s);

    // get line
    getline(ss, ls);
    cout << "get line: " << ls << endl;

    // count word
    while (ss >> word) {
        count++;
        cout << word << endl;
        v.push_back(word);
    }
    cout << "Word count: " << count << endl;

    // revert char
    for (auto it=s.rbegin(); it != s.rend(); ++it) {
        cout << (*it) ;
    }
    cout << endl;
    // revert word
    for (auto it=v.rbegin(); it != v.rend(); ++it) {
        cout << (*it);
        if (it !=--v.rend()) {
            cout << "_";
        }
    }
    cout << endl;
    return 0;
}