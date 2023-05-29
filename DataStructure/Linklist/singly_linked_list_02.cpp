#include <stdio.h>
#include <stdlib.h>

// Định nghĩa cấu trúc cho một nút trong danh sách liên kết đơn
typedef struct Node
{
    int data;
    struct Node *next;
} Node;

// Hàm để tạo một nút mới với giá trị được cung cấp
Node *createNode(int value)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = value;
    newNode->next = NULL;
    return newNode;
}

// Hàm để in ra các phần tử trong danh sách liên kết đơn
void printLinkedList(Node *head)
{
    Node *current = head;
    while (current != NULL)
    {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

int main()
{
    // Khởi tạo các nút trong danh sách liên kết đơn
    Node *head = createNode(1);
    Node *second = createNode(2);
    Node *third = createNode(3);

    // Thiết lập liên kết giữa các nút
    head->next = second;
    second->next = third;

    // In ra danh sách liên kết đơn
    printLinkedList(head);

    return 0;
}
