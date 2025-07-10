#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <iostream>

typedef boost::geometry::model::d2::point_xy<double> point_type;
typedef boost::geometry::model::polygon<point_type> polygon_type;

int main()
{
    // Khởi tạo điểm
    point_type point_check, point[4];
    // point_type point(2.5, 2.5); // ví dụ điểm có tọa độ (2.5, 2.5)
    point_check.x(2.5);
    point_check.y(6.0);
    std::cout << "Point: " << boost::geometry::wkt(point_check)<< std::endl;

    point[0].x(0.0);
    point[0].y(0.0);
    std::cout << "Point1: " << boost::geometry::wkt(point[0])<< std::endl;
    point[1].x(5.0);
    point[1].y(0.0);
    std::cout << "Point2: " << boost::geometry::wkt(point[1])<< std::endl;
    point[2].x(5.0);
    point[2].y(5.0);
    std::cout << "Point3: " << boost::geometry::wkt(point[2])<< std::endl;
    point[3].x(0.0);
    point[3].y(5.0);
    std::cout << "Point4: " << boost::geometry::wkt(point[3])<< std::endl;


    // Khởi tạo đa giác
    polygon_type polygon;
    // boost::geometry::read_wkt("POLYGON((0 0,5 0,5 5,0 5))", polygon); // ví dụ đa giác hình vuông kích thước 5x5
    for (const point_type& point_t : point)  // không cho thay đổi giá trị và tạo tham chiếu để tiết kiệm bộ nhớ
    {
        boost::geometry::append(polygon.outer(),point_t);

    }
    std::cout << "polygon init:" << boost::geometry::wkt(polygon) << std::endl;
    // Sắp xếp lại thứ tự các đỉnh cho polygon
    boost::geometry::correct(polygon);

    // In lại thứ tự đa giác
    std::cout << "polygon correct:" << boost::geometry::wkt(polygon) << std::endl;

    // Kiểm tra xem điểm có nằm trong đa giác hay không
    if (boost::geometry::within(point_check, polygon)) {
        std::cout << "Điểm nằm trong đa giác\n";
    } else {
        std::cout << "Điểm không nằm trong đa giác\n";
    }

    return 0;
}
