// Binary tree simple implementation with basic operations and raw pointers.
#include <iostream>

struct Node {
    int data;
    Node* left;
    Node* right;
    Node(int value) : data(value), left(nullptr), right(nullptr) {}
};

class BinarySearchTree {
private:
    Node* root;
public:
    BinarySearchTree() : root(nullptr) {}

    ~BinarySearchTree() {
        clear(root);
        root = nullptr;
    }

    BinarySearchTree(const BinarySearchTree&) = delete;
    BinarySearchTree& operator=(const BinarySearchTree&) = delete;

    BinarySearchTree(BinarySearchTree&& other) noexcept : root(other.root) {
        other.root = nullptr;
    }

    BinarySearchTree& operator=(BinarySearchTree&& other) noexcept {
        if (this != &other) {
            clear(root);
            root = other.root;
            other.root = nullptr;
        }
        return *this;
    }

    bool insert(int value) {
        if (!root) {
            root = new Node(value);
            return true;
        }

        Node* ptr = root;
        while (ptr) {
            if (value < ptr->data) {
                if (!ptr->left) {
                    ptr->left = new Node(value);
                    return true;
                }
                ptr = ptr->left;
            } else if (value > ptr->data) {
                if (!ptr->right) {
                    ptr->right = new Node(value);
                    return true;
                }
                ptr = ptr->right;
            } else {
                // Keep BST keys unique.
                return false;
            }
        }

        return false;
    }

    bool contains(int value) const {
        const Node* ptr = root;
        while (ptr) {
            if (ptr->data == value) {
                return true;
            } else if (value < ptr->data) {
                ptr = ptr->left;
            } else {
                ptr = ptr->right;
            }
        }
        return false;
    }

    int height() const {
        return heightImpl(root);
    }

    void displayTree() const {
        showTree(root);
    }

private:
    static int heightImpl(const Node* node) {
        if (!node) return 0;

        return (std::max(heightImpl(node->left), heightImpl(node->right)) + 1);
    }

    static void showTree(const Node* node) {
        if (!node) return;
        std::cout << node->data << ", \n";
        showTree(node->left);
        showTree(node->right);
    }

    static void clear(Node* node) {
        if (!node) return;
        clear(node->left);
        clear(node->right);
        delete node;
    }
};

int main() {
    BinarySearchTree bst;
    bst.insert(5);
    bst.insert(3);
    bst.insert(7);
    bst.insert(3);
    bst.insert(1);
    bst.insert(9);

    bst.displayTree();

    std::cout << "Contains 3: " << std::boolalpha << bst.contains(3) << std::endl;
    std::cout << "Contains 4: " << std::boolalpha << bst.contains(4) << std::endl;
    std::cout << "Height: " << bst.height() << std::endl;

    return 0;
}