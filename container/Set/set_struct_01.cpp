#include <iostream>
#include <set>

struct data_t {
    int entry;
    int duration;

    // Toán tử so sánh để sắp xếp các phần tử
    bool operator<(const data_t& other) const {
        if (duration != other.duration) {
            return duration < other.duration;
        }
        return entry < other.entry;
    }
};

int main() {
    std::set<data_t> dataset;
    data_t new_data;

    while (true) {
        std::cout << "Input element ";
        std::cin >> new_data.entry;
        if (new_data.entry == -1) break;
        std::cin >> new_data.duration;

        dataset.insert(new_data);

        // Phần tử nhỏ nhất luôn ở đầu tiên trong set
        const data_t& min_element = *dataset.begin();

        std::cout << "Duration MIN element: "
                  << "Entry: " << min_element.entry
                  << ", Duration: " << min_element.duration << std::endl;
    }

    return 0;
}
