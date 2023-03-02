#include <iostream>
#include <Eigen/Dense>

int main()
{
    // Tạo ma trận 3x3 với các giá trị ngẫu nhiên
    Eigen::Matrix3d A = Eigen::Matrix3d::Random();

    // Tạo vector với các giá trị ngẫu nhiên
    Eigen::VectorXd b = Eigen::VectorXd::Random(3);

    // In ra ma trận và vector
    std::cout << "A =\n" << A << std::endl;
    std::cout << "b =\n" << b << std::endl;

    // Nhân ma trận với vector
    Eigen::VectorXd c = A * b;
    std::cout << "c =\n" << c << std::endl;

    // Cộng hai ma trận
    Eigen::Matrix3d B = Eigen::Matrix3d::Random();
    std::cout << "B =\n" << B << std::endl;

    Eigen::Matrix3d D = A + B;
    std::cout << "D =\n" << D << std::endl;

    return 0;
}
