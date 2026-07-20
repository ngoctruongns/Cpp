/*

Hex values: 00 00 3D 3D 00 05 02 07 40 69 60
Parse data từ chuỗi hex values trên theo struct blackbox_setting_t dưới đây.
Trong đó hex values là lấy từ byte index 32, ta cần parese từ byte index 37, 38, 39, 40 để điền vào struct
blackbox_setting_t. Sau đó in ra các giá trị đã parse được.
*/

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

typedef struct {
    uint8_t course;  // 37

    /* Byte 38:
    bit 4~7 : User-defined water temperature
    bit 0~3 : User-defined dehydration rpm
    */
    uint8_t dehy_rpm   : 4;    // bit 0 ~ 3
    uint8_t water_temp : 4;    // bit 4 ~ 7

    /* byte 39:
    bit 4~7 : User-defined drying options
    bit 1~3 : User-defined number of rinses
    bit 0 : Steam Option information
    */
    uint8_t steam_opt : 1;    // bit 0
    uint8_t rinse_cnt : 3;    // bit 1 ~ 3
    uint8_t dry_opt   : 4;    // bit 4 ~ 7

    /* byte 40:
    bit 5~ 7 : WashOption
    bit 4 : Dry Steam
    bit 3 : Turboshot
    bit 2 : Dry Filter Clogging
    bit 1 : Dry Iron
    bit 0 : Rinse Hold
    */
    uint8_t rinse_hold         : 1;    // bit 0
    uint8_t dry_iron           : 1;    // bit 1
    uint8_t dry_filter_clogged : 1;    // bit 2
    uint8_t turboshot          : 1;    // bit 3
    uint8_t dry_steam          : 1;    // bit 4
    uint8_t wash_opt           : 3;    // bit 5 ~ 7
} blackbox_setting_t;

// Parse blackbox_setting_t from raw bytes.
// Data starts at byte index 32, so:
//   byte 37 -> data[5], byte 38 -> data[6], byte 39 -> data[7], byte 40 -> data[8]
blackbox_setting_t parseBlackbox(const std::vector<uint8_t>& data)
{
    blackbox_setting_t s{};

    s.course = data[5];                        // byte 37

    uint8_t b38 = data[6];                     // byte 38
    s.dehy_rpm   = (b38 >> 0) & 0x0F;         // bits 0~3
    s.water_temp = (b38 >> 4) & 0x0F;         // bits 4~7

    uint8_t b39 = data[7];                     // byte 39
    s.steam_opt = (b39 >> 0) & 0x01;          // bit 0
    s.rinse_cnt = (b39 >> 1) & 0x07;          // bits 1~3
    s.dry_opt   = (b39 >> 4) & 0x0F;          // bits 4~7

    uint8_t b40 = data[8];                     // byte 40
    s.rinse_hold         = (b40 >> 0) & 0x01; // bit 0
    s.dry_iron           = (b40 >> 1) & 0x01; // bit 1
    s.dry_filter_clogged = (b40 >> 2) & 0x01; // bit 2
    s.turboshot          = (b40 >> 3) & 0x01; // bit 3
    s.dry_steam          = (b40 >> 4) & 0x01; // bit 4
    s.wash_opt           = (b40 >> 5) & 0x07; // bits 5~7

    return s;
}

void printBlackbox(const blackbox_setting_t& s)
{
    std::cout << "=== blackbox_setting_t ===" << '\n';
    std::cout << "  course             : " << static_cast<int>(s.course)             << '\n';
    std::cout << "  dehy_rpm           : " << static_cast<int>(s.dehy_rpm)           << '\n';
    std::cout << "  water_temp         : " << static_cast<int>(s.water_temp)         << '\n';
    std::cout << "  steam_opt          : " << static_cast<int>(s.steam_opt)          << '\n';
    std::cout << "  rinse_cnt          : " << static_cast<int>(s.rinse_cnt)          << '\n';
    std::cout << "  dry_opt            : " << static_cast<int>(s.dry_opt)            << '\n';
    std::cout << "  rinse_hold         : " << static_cast<int>(s.rinse_hold)         << '\n';
    std::cout << "  dry_iron           : " << static_cast<int>(s.dry_iron)           << '\n';
    std::cout << "  dry_filter_clogged : " << static_cast<int>(s.dry_filter_clogged) << '\n';
    std::cout << "  turboshot          : " << static_cast<int>(s.turboshot)          << '\n';
    std::cout << "  dry_steam          : " << static_cast<int>(s.dry_steam)          << '\n';
    std::cout << "  wash_opt           : " << static_cast<int>(s.wash_opt)           << '\n';
}

int main()
{
    std::cout << "Nhap chuoi hex (bat dau tu byte index 32), vi du:\n"
              << "  00 00 3D 3D 00 05 02 07 40 69 60\n"
              << "Nhap 'q' de thoat.\n";

    std::string line;
    while (true) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "q" || line == "Q") break;
        if (line.empty()) continue;

        // Parse space-separated hex tokens into bytes
        std::vector<uint8_t> data;
        std::istringstream iss(line);
        std::string token;
        bool parseError = false;
        while (iss >> token) {
            try {
                data.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
            } catch (...) {
                std::cerr << "Loi: token khong hop le: '" << token << "'\n";
                parseError = true;
                break;
            }
        }
        if (parseError) continue;

        // Need at least 9 bytes so that data[8] (byte index 40) is accessible
        if (data.size() < 9) {
            std::cerr << "Loi: can it nhat 9 bytes (byte index 32..40). Nhan duoc: "
                      << data.size() << " byte(s).\n";
            continue;
        }

        printBlackbox(parseBlackbox(data));
    }

    return 0;
}