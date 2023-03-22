// Đây là một chương trình C++ đơn giản về cây nhị phân:

#include <iostream>
using namespace std;

// Định nghĩa cấu trúc của 1 nút của cây
struct Node
{
    int data;
    Node *left;
    Node *right;
};

// Hàm tạo nút mới trong cây
Node *createNode(int data)
{
    Node *newNode = new Node;
    newNode->data = data;
    cout<< "creat node data: "<< data << endl;
    newNode->left = NULL;
    newNode->right = NULL;
    cout<< "creat node at addr: "<< newNode << endl;
    return newNode;
}

// Hàm thêm một nút mới vào cây
Node *insertNode(Node *root, int data)
{
    if (root == NULL)
    {
        return createNode(data);
    }
    else if (data < root->data)
    {
        root->left = insertNode(root->left, data);
    }
    else if (data > root->data)
    {
        root->right = insertNode(root->right, data);
    }
    return root;
}

// Hàm duyệt cây theo thứ tự trước (preorder)
void preorderTraversal(Node *root)
{
    if (root != NULL)
    {
        cout << root->data << " ";
        cout << "root:" << root << endl;
        cout << "root->left:" << root->left << endl;
        cout << "root->right:" << root->right << endl;
        preorderTraversal(root->left);
        preorderTraversal(root->right);
    }
}

// Hàm duyệt cây theo thứ tự giữa (inorder)
void inorderTraversal(Node *root)
{
    if (root != NULL)
    {
        inorderTraversal(root->left);
        cout << root->data << " ";
        inorderTraversal(root->right);
    }
}

// Hàm duyệt cây theo thứ tự sau (postorder)
void postorderTraversal(Node *root)
{
    if (root != NULL)
    {
        postorderTraversal(root->left);
        postorderTraversal(root->right);
        cout << root->data << " ";
    }
}

// Hàm xóa cây
void deleteTree(Node *root)
{
    if (root != NULL)
    {
        deleteTree(root->left);
        deleteTree(root->right);
        delete root;
    }
}

int main()
{
    Node *root = NULL;

    // Thêm các nút vào cây
    root = insertNode(root, 5);
    // cout<< "root addr: "<< &root << endl;
    root = insertNode(root, 3);
    // cout<< "root addr: "<< &root << endl;
    root = insertNode(root, 7);
    // cout<< "root addr: "<< &root << endl;
    root = insertNode(root, 1);
    // cout<< "root addr: "<< &root << endl;
    root = insertNode(root, 9);

    root = insertNode(root, 4);
    root = insertNode(root, 6);

    // Duyệt cây theo thứ tự trước (preorder)
    cout << "Preorder traversal: ";
    preorderTraversal(root);
    cout << endl;

    // Duyệt cây theo thứ tự giữa (inorder)
    cout << "Inorder traversal: ";
    inorderTraversal(root);
    cout << endl;

    // Duyệt cây theo thứ tự sau (postorder)
    cout << "Postorder traversal: ";
    postorderTraversal(root);
    cout << endl;

    // Xóa cây
    deleteTree(root);

    return 0;
}