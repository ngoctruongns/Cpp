#include <iostream>
#include <cmath>
#include "gnuplot-iostream.h"
int main()
{
    // Khởi tạo đối tượng kết nối Gnuplot
    Gnuplot gp;

    // Tạo dữ liệu và vẽ đồ thị
    std::vector<double> x(100), y(100);
    for (int i = 0; i < 100; i++)
    {
        x[i] = i / 10.0;
        y[i] = sin(x[i]);
    }
    gp << "plot '-' with lines\n";
    gp.send1d(boost::make_tuple(x, y));

    return 0;
}
