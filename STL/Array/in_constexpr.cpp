#include <array>
#include <cstddef>

constexpr int get_second()
{
    constexpr std::array<int, 3> arr = {1, 2, 3};
    // arr[1] bên trong std::array gọi tới __array_traits::_S_ref
    return arr[1]; // <-- truy cập phần tử tại compile-time
}

int main()
{
    constexpr int v = get_second(); // được tính tại compile-time
    static_assert(v == 2, "Compile-time check");
}
