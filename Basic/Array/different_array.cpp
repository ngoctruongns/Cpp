#include <iostream>
#include <vector>

using namespace std;
int arr[5] ={ 1, 2, 3, 4, 5};
int res[5] ={ 1, 2, 3, 4, 5};

int sufix[5] ={ 0};

void printArr(int *aprt, int num) {
    for (int id = 0; id < num; id++) {
        cout << aprt[id] << ", ";
    }
    cout << endl;

}

void changeFor(int i, int j, int k)
{
    for (int id = i-1; id< j; id++) {
        res[id] += k;
    }
    printArr(res, 5);
}

void changeSufix(int i, int j, int k)
{
    sufix[i-1] += k;
    if (j < 5) {
        sufix[j] -= k;
    }
    printArr(sufix, 5);
}


int main()
{
    sufix[0] = arr[0];
    for (int i = 1; i <= 5; i++)
    {
        sufix[i] = arr[i] - arr[i-1];
    }
    cout << "sufix: " ;
    printArr(sufix, 5);


    changeFor(1,3, 3);
    changeFor(3,5, 2);
    changeFor(2,4, 5);

    changeSufix(1,3, 3);
    changeSufix(3,5, 2);
    changeSufix(2,4, 5);

    res[0] = sufix[0];
    for(int id = 1; id < 5; id++) {
        res[id] = res[id-1] + sufix[id];
    }




    cout << "result: " << endl;
    for (int it : res) {
        cout << it << ", ";
    }


    return 0;
}