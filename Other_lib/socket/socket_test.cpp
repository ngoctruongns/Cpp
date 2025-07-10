#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main(int argc, char const *argv[]) {
    // Tạo socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        cout << "Không thể tạo socket" << endl;
        return 1;
    }

    // Kết nối tới server
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8070);
    server_address.sin_addr.s_addr = inet_addr("192.168.0.188"); // Địa chỉ IP của ESP32

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        cout << "Không thể kết nối tới server" << endl;
        return 1;
    }

    cout << "Đã kết nối tới server" << endl;

    // Nhận dữ liệu mảng từ server
    uint16_t data[64];
    memset(data, 0, sizeof(data));
    const char* hello = "Hello from client#";
    char recv_buffer[10];


    for (int i = 0; i < 100; i++)
    {
        int total_bytes = 0;
        send(client_socket, hello, strlen(hello), 0);
        printf("Hello message sent\n");
        // Xóa bộ đệm của hàm recv() trước khi nsaswwhận dữ liệu mới
        int bytes_received = 0;
        do {
            bytes_received = recv(client_socket, recv_buffer, 1, MSG_DONTWAIT);
            if (bytes_received <= 0) {
                continue;
            }
            cout << "Read "<< bytes_received << " bytes, value is "<< int(recv_buffer[0]) << endl;
        } while (recv_buffer[0] != '@');


        // Nhận dữ liệu mới
        while (total_bytes < sizeof(data) ) {

            int bytes_to_receive = sizeof(data) - total_bytes;
            int bytes_received = recv(client_socket, (uint16_t*)data + total_bytes, bytes_to_receive, 0);
            if (bytes_received <= 0) {
                cout << "Lỗi khi nhận dữ liệu" << endl;
                return 1;
            }

            total_bytes += bytes_received;
            cout << "Read "<< bytes_received << " bytes from fd, total lenght is "<< total_bytes << endl;
        }

        cout << "Đã nhận dữ liệu mảng từ server" << endl;

        // Hiển thị dữ liệu mảng
        for (int i = 0; i < 64; i++) {
            cout << data[i] << " ";
        }
        cout << endl;

    }


    close(client_socket); // Đóng kết nối

    return 0;
}
