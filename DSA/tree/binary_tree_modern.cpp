// Modern C++ example: Binary Search Tree (BST)
// Features: insert, search, erase, traversals, height, min/max, and validation.

#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <vector>

class BinarySearchTree {
public:
    bool insert(int value) {
        return insertImpl(root_, value);
    }

    bool contains(int value) const {
        return containsImpl(root_.get(), value);
    }

    bool erase(int value) {
        return eraseImpl(root_, value);
    }

    std::vector<int> inorder() const {
        std::vector<int> result;
        inorderImpl(root_.get(), result);
        return result;
    }

    std::vector<int> preorder() const {
        std::vector<int> result;
        preorderImpl(root_.get(), result);
        return result;
    }

    std::vector<int> postorder() const {
        std::vector<int> result;
        postorderImpl(root_.get(), result);
        return result;
    }

    std::vector<int> levelOrder() const {
        std::vector<int> result;
        if (!root_) {
            return result;
        }

        std::queue<const Node*> q;
        q.push(root_.get());
        while (!q.empty()) {
            const Node* current = q.front();
            q.pop();
            result.push_back(current->data);

            if (current->left) {
                q.push(current->left.get());
            }
            if (current->right) {
                q.push(current->right.get());
            }
        }
        return result;
    }

    int height() const {
        return heightImpl(root_.get());
    }

    bool empty() const {
        return root_ == nullptr;
    }

    int minValue() const {
        if (!root_) {
            throw std::runtime_error("Tree is empty.");
        }

        const Node* current = root_.get();
        while (current->left) {
            current = current->left.get();
        }
        return current->data;
    }

    int maxValue() const {
        if (!root_) {
            throw std::runtime_error("Tree is empty.");
        }

        const Node* current = root_.get();
        while (current->right) {
            current = current->right.get();
        }
        return current->data;
    }

    bool isValidBst() const {
        return isValidBstImpl(
            root_.get(),
            std::numeric_limits<long long>::min(),
            std::numeric_limits<long long>::max()
        );
    }

private:
    struct Node {
        explicit Node(int value) : data(value) {}
        ~Node() {
            std::cout << "distroy: " << data << std::endl;
        }

        int data{};
        std::unique_ptr<Node> left{};
        std::unique_ptr<Node> right{};
    };

    std::unique_ptr<Node> root_{};

    static bool insertImpl(std::unique_ptr<Node>& node, int value) {
        if (!node) {
            node = std::make_unique<Node>(value);
            return true;
        }

        if (value < node->data) {
            return insertImpl(node->left, value);
        }
        if (value > node->data) {
            return insertImpl(node->right, value);
        }
        return false;
    }

    static bool containsImpl(const Node* node, int value) {
        if (!node) {
            return false;
        }
        if (value < node->data) {
            return containsImpl(node->left.get(), value);
        }
        if (value > node->data) {
            return containsImpl(node->right.get(), value);
        }
        return true;
    }

    static int heightImpl(const Node* node) {
        if (!node) {
            return -1;
        }
        return 1 + std::max(heightImpl(node->left.get()), heightImpl(node->right.get()));
    }

    static void inorderImpl(const Node* node, std::vector<int>& out) {
        if (!node) {
            return;
        }
        inorderImpl(node->left.get(), out);
        out.push_back(node->data);
        inorderImpl(node->right.get(), out);
    }

    static void preorderImpl(const Node* node, std::vector<int>& out) {
        if (!node) {
            return;
        }
        out.push_back(node->data);
        preorderImpl(node->left.get(), out);
        preorderImpl(node->right.get(), out);
    }

    static void postorderImpl(const Node* node, std::vector<int>& out) {
        if (!node) {
            return;
        }
        postorderImpl(node->left.get(), out);
        postorderImpl(node->right.get(), out);
        out.push_back(node->data);
    }

    static bool isValidBstImpl(const Node* node, long long minAllowed, long long maxAllowed) {
        if (!node) {
            return true;
        }

        if (node->data <= minAllowed || node->data >= maxAllowed) {
            return false;
        }

        return isValidBstImpl(node->left.get(), minAllowed, node->data) &&
               isValidBstImpl(node->right.get(), node->data, maxAllowed);
    }

    static int removeMin(std::unique_ptr<Node>& node) {
        if (!node->left) {
            int minValue = node->data;
            node = std::move(node->right);
            return minValue;
        }
        return removeMin(node->left);
    }

    static bool eraseImpl(std::unique_ptr<Node>& node, int value) {
        if (!node) {
            return false;
        }

        if (value < node->data) {
            return eraseImpl(node->left, value);
        }
        if (value > node->data) {
            return eraseImpl(node->right, value);
        }

        if (!node->left && !node->right) {
            node.reset();
            return true;
        }
        if (!node->left) {
            node = std::move(node->right);
            return true;
        }
        if (!node->right) {
            node = std::move(node->left);
            return true;
        }

        node->data = removeMin(node->right);
        return true;
    }
};

static void printValues(const char* label, const std::vector<int>& values) {
    std::cout << label;
    for (int value : values) {
        std::cout << value << ' ';
    }
    std::cout << '\n';
}

int main() {
    BinarySearchTree bst;

    const std::vector<int> valuesToInsert{5, 3, 7, 1, 9, 4, 6, 8, 2};
    for (int value : valuesToInsert) {
        const bool inserted = bst.insert(value);
        std::cout << "Insert " << value << ": " << (inserted ? "ok" : "duplicate") << '\n';
    }

    std::cout << '\n';
    printValues("Inorder (sorted):   ", bst.inorder());
    printValues("Preorder:           ", bst.preorder());
    printValues("Postorder:          ", bst.postorder());
    printValues("Level-order (BFS):  ", bst.levelOrder());

    std::cout << '\n';
    std::cout << "Contains 6: " << std::boolalpha << bst.contains(6) << '\n';
    std::cout << "Contains 42: " << std::boolalpha << bst.contains(42) << '\n';
    std::cout << "Min value: " << bst.minValue() << '\n';
    std::cout << "Max value: " << bst.maxValue() << '\n';
    std::cout << "Height (edge-based): " << bst.height() << '\n';
    std::cout << "Valid BST: " << std::boolalpha << bst.isValidBst() << '\n';

    std::cout << '\n';
    std::cout << "Erase 7 (node with two children): " << std::boolalpha << bst.erase(7) << '\n';
    std::cout << "Erase 1 (leaf): " << std::boolalpha << bst.erase(1) << '\n';
    std::cout << "Erase 100 (missing): " << std::boolalpha << bst.erase(100) << '\n';

    printValues("Inorder after erase:", bst.inorder());
    std::cout << "Height after erase: " << bst.height() << '\n';

    return 0;
}