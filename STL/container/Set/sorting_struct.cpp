#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

struct data_t
{
    int param1; // start
    int param2; // duration
    int param3; // order
};

bool compare(const data_t &a, const data_t &b)
{
    if (a.param2 != b.param2)
    {
        return a.param2 < b.param2;
    }
    else
    {
        return a.param1 < b.param1;
    }
}

int main()
{
    int N = 0;
    std::vector<data_t> vect;
    cin >> N;
    // when n=0, n=1
    if (N <= 0)
    {
        cout << "Please enter N>0" << endl;
        return 1;
    }

    int Ei[N], Ti[N];
    for (int i = 0; i < N; i++)
    {
        cin >> Ei[i];
    }

    for (int i = 0; i < N; i++)
    {
        cin >> Ti[i];
    }

    for (int i = 0; i < N; i++)
    {
        data_t value;
        value.param1 = Ei[i];
        value.param2 = Ti[i];
        value.param3 = i + 1;
        vect.push_back(value);
    }

    // sort vector
    sort(vect.begin(), vect.end(), [](const data_t &a, const data_t &b)
         { return a.param1 < b.param1; });

    // print vector input
    // for (const auto& item : vect) {
    //     std::cout  << item.param1 << "," << item.param2 << std::endl;
    // }

    // result array
    std::vector<data_t> waitingVect;
    std::vector<int> result;
    // init value
    result.push_back(vect[0].param3);
    int current_time = vect[0].param1 + vect[0].param2;
    vect.erase(vect.begin());

    // Duyet tung phan tu
    for (int id = 1; id < N; id++)
    {
        // check vect to waiting room
        if (vect.empty())
        {
            // chi xu ly trong wating list
        }
        else
        {
            // duyet tung phan tu de cho vao waiting list
            for (auto it = vect.begin(); it != vect.end(); ++it)
            {
                if ((*it).param1 <= current_time)
                {
                    waitingVect.push_back(*it);
                    vect.erase(it);
                    it--;
                }
            }
        }
        // xet waiting list
        if (waitingVect.empty())
        {
            // neu trong thi lay phan tu tiep theo cua vect
            result.push_back(vect.front().param3);
            current_time = vect.front().param1 + vect.front().param2;
            vect.erase(vect.begin());
        }
        else
        {
            if (waitingVect.size() > 1) {
                // Neu dang co hang cho thi uu tien nguoi trong hang cho truoc
                std::sort(waitingVect.begin(), waitingVect.end(), compare);

            }
            // lay phan tu thu nhat trong hang cho
            result.push_back(waitingVect.front().param3);
            current_time += waitingVect.front().param2;
            waitingVect.erase(waitingVect.begin());
        }
    }

    for (const auto &item : result)
    {
        std::cout << item << " ";
    }

    return 0;
}