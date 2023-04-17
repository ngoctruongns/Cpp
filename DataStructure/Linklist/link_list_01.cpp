#include <iostream>
using namespace std;

// Khai báo cấu trúc Node
struct Node
{
    int data;
    Node *next;
};

// Hàm in các phần tử trong danh sách liên kết
void printList(Node *node)
{
    while (node != NULL)
    {
        cout << node->data << " ";
        node = node->next;
    }
}

// Hàm in sử dụng for
void printList1(Node *node)
{
    for (Node* ptr = node; ptr != nullptr; ptr = ptr->next)
    {
        cout << ptr->data << " : ";
    }
}

// Tìm phần tử giữa chuỗi
int findMidList(Node* head)
{

    Node *slow_ptr = head;
    Node *fast_ptr = head;

    for (int i = 0; fast_ptr != NULL; i++)
    {
        if (i % 2 == 1)
        {
            slow_ptr = slow_ptr->next;
        }
        fast_ptr = fast_ptr->next;
    }
    cout << "The middle element is: " << slow_ptr->data << endl;
    return slow_ptr->data;

}

int main()
{
    // Khởi tạo các node
    Node *head = NULL;
    Node *second = NULL;
    Node *third = NULL;

    // Cấp phát bộ nhớ cho các node
    head = new Node();
    second = new Node();
    third = new Node();

    // Gán giá trị cho từng node
    head->data = 1;
    head->next = second;

    second->data = 2;
    second->next = third;

    third->data = 3;
    third->next = NULL;

    // In ra màn hình danh sách liên kết
    // printList(head);
    printList1(head);
    findMidList(head);

    return 0;
}
