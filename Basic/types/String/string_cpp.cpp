#include <iostream>
#include <cstring>
#include <vector>


using namespace std;

int main()
{
    string st = "That is sample string";
    string st1 = "that";


    cout << st.erase(2, 3) << endl;
    cout << st.insert(2, "at ") << endl;
    cout << (st > st1) << endl;
    cout << st << endl;

    // revert string
    // for ( auto it = st.rbegin(); it !=st.rend(); ++it) {
    //     cout << (*it) ;
    // }

    // revert token
    for (int i = 0; i < st.length(); i ++) {
        cout << st[i];
    }
    cout << endl;

    // cout << st.substr(2, 4);

    int idSta = -1, idEnd = -1;
    vector<string>  rs;
    for ( int i = 0; i < st.length(); i++) {
        if (st[i] == ' ') {
            if (idSta != -1) {
                idEnd = i;
                cout << i;
                rs.push_back(st.substr(idSta, idEnd - idSta));
                idSta = idEnd = -1;

            }
        } else {
            if (( idEnd == -1) && (idSta == -1))
            idSta = i;
        }
    }

    for (string sub: rs) {
        cout << sub << " ";
    }
    cout << endl;

    return 0;
}