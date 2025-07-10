#include <stdio.h>
#include <stdbool.h>
#define N  10
int arr[N] = {10, 9, 7, 6, 8, 1, 3, 2, 4, 5};

void printArr()
{
    printf("ARR = ");
    for (int i = 0; i < N; i++)
    {
        printf("%d,", arr[i]);
    }
    printf("\n");
}

int main()
{
    printf("Hello World \n");
    printArr();
    for (int j=1; j < N -1 ; j++)
    {
        bool pass = true;
        for (int i = 0; i<(N - j); i++)
        {
           
                
            if ( arr[i] > arr[i+1])
            {
                int temp = arr[i];
                arr[i]   = arr[i+1];
                arr[i+1] = temp;
                printArr();
                pass = false;
            }
        }
        if (pass) break;
        printf("-------------------------------- \n");
    }
    printf("Sort complete");
    

    return 0;
}