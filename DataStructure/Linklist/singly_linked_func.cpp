#include <iostream>
#include <vector>
using namespace std;

// Khai báo cấu trúc Node
struct Node
{
    int val;
    Node *next;
    Node() : val(0), next(nullptr) {};
    Node(int x) : val(x), next(nullptr) {};
    Node(int x, Node * ptr) : val(x), next(ptr) {};
};

// Hàm in các phần tử trong danh sách liên kết
void printList(Node *node)
{
    while (node != nullptr)
    {
        cout << node->val << "->";
        node = node->next;
    }
    cout << "NULL" << endl;

}

// Hàm in sử dụng for
void printList1(Node *node)
{
    for (Node* ptr = node; ptr != nullptr; ptr = ptr->next)
    {
        cout << ptr->val << "->";
    }
    cout << "NULL" << endl;
}

// Creat new Node
Node * creatNode(int val)
{
    Node *tmp = new Node(val);
    return tmp;
}

// Add node at head
void addHead(Node *&head, int data)
{
    Node * newNode = creatNode(data);
    newNode->next = head;
    head = newNode;
}

// Add node at tail
void addTail(Node *head, int data)
{
    Node * newNode = creatNode(data);
    // Go to end element of list
    while (head->next != nullptr) {
        head = head->next;
    }
    head->next = newNode;
}

// Arrange to list
vector<int> res;
void arrangeList(Node *ls1, Node *ls2)
{
    while (ls1 != nullptr || ls2 != nullptr)
    {
        if (ls2 == nullptr || ls1->val > ls2->val ) {
            res.push_back(ls1->val);
            ls1 = ls1->next;
        } else  if (ls1 == nullptr || ls1->val <= ls2->val ) {
            res.push_back(ls2->val);
            ls2 = ls2->next;
        }
    }
    // print result
    for (int it:res) {
        cout << it << ", ";
    }

}

int main()
{
    // Khởi tạo các node
    Node *lnode1 = nullptr;
    Node *lnode2 = nullptr;
    // list node 1
    addHead(lnode1, 1);
    addHead(lnode1, 5);
    addHead(lnode1, 7);
    addHead(lnode1, 15);
    // list node 2
    addHead(lnode2, 4);
    addHead(lnode2, 8);
    addHead(lnode2, 12);
    addTail(lnode2, 2);
    // addTail(lnode2, 0);


    // In ra màn hình danh sách liên kết
    printList(lnode1);
    printList(lnode2);
    // printList1(head);

    // Arrange 2 list
    arrangeList(lnode1, lnode2);

    return 0;
}
