// Example program
#include <iostream>

struct odomData
{
    int x = 3;
    int theta = 2;
};

void GetData(odomData *&msg) // Passing the pointer by reference, so use the &
{
    int speed = msg->x;     // Accessing the data in the struct
    int yaw = (*msg).theta; // Another way of accessing the data that has been passed
    // print
    std::cout << speed << std::endl;
    std::cout << yaw << std::endl;
}

int main(void)
{

    odomData d;
    odomData *ptr = &d; // The pointer (of type odomData*) holds the address of the struct d of type odomData

    GetData(ptr); // Call the function to get the data, passing the pointer by reference

    return 0;
}