#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
using namespace std;
std::vector<std::string> findRelativeRanks(std::vector<int>& score) {
    int n = score.size();
    std::vector<int> indices(n);

    for (int i = 0; i < n; ++i) {
        indices[i] = i;
    }

    std::sort(indices.begin(), indices.end(), [&score](int a, int b) {
        return score[a] > score[b];
    });

    std::vector<std::string> result(n);

    for (int i = 0; i < n; ++i) {
        if (i == 0) {
            result[indices[i]] = "Gold Medal";
        } else if (i == 1) {
            result[indices[i]] = "Silver Medal";
        } else if (i == 2) {
            result[indices[i]] = "Bronze Medal";
        } else {
            result[indices[i]] = std::to_string(i + 1);
        }
    }

    return result;
}

vector<string> findRelativeRanks1(vector<int>& score) {
    std::vector<std::pair<int, int>> scores;
    for (int i = 0; i < score.size(); i++) {
        scores.push_back({score[i], i});
    }
    std::sort(scores.begin(), scores.end(), std::greater<std::pair<int, int>>());
    std::vector<std::string> result(score.size());
    for (int i = 0; i < scores.size(); i++) {
        if (i == 0) result[scores[i].second] = "Gold Medal";
        else if (i == 1) result[scores[i].second] = "Silver Medal";
        else if (i == 2) result[scores[i].second] = "Bronze Medal";
        else result[scores[i].second] = std::to_string(i + 1);
    }
    return result;
    }

int main() {
    // Ví dụ điểm số
    std::vector<int> scores;
    int N = 1000000;

    for (int i= 0; i< N; i++ ) {
        scores.push_back(N -i);
    }

    // Lấy thời gian bắt đầu
    auto start = std::chrono::high_resolution_clock::now();

    // Gọi hàm cần đo thời gian
    std::vector<std::string> result = findRelativeRanks(scores);
    // std::vector<std::string> result = findRelativeRanks1(scores);

    // Lấy thời gian kết thúc
    auto end = std::chrono::high_resolution_clock::now();

    // Tính toán thời gian thực hiện
    std::chrono::duration<double> duration = end - start;

    // In thời gian thực hiện
    std::cout << "Time taken by function: " << duration.count() << " seconds" << std::endl;

    // // In kết quả để kiểm tra
    // for (const auto& rank : result) {
    //     std::cout << rank << " ";
    // }
    // std::cout << std::endl;

    return 0;
}
