/**
 * TRIE (Prefix Tree)
 * ===================
 * Efficient string storage and retrieval.
 * O(L) per insert/search where L = string length.
 *
 * Applications:
 *   - Autocomplete / prefix search
 *   - Word dictionary
 *   - XOR maximum (binary trie)
 *   - Suffix array problems
 */

#include <bits/stdc++.h>
using namespace std;

// ─────────────────────────────────────────────
// 1. ARRAY-BASED TRIE (fast, fixed alphabet)
// ─────────────────────────────────────────────
struct TrieNode {
    int children[26];
    int count;      // how many words pass through this node
    bool isEnd;

    TrieNode() : count(0), isEnd(false) {
        fill(children, children + 26, -1);
    }
};

struct Trie {
    vector<TrieNode> nodes;

    Trie() { nodes.emplace_back(); }  // root at index 0

    void insert(const string& s) {
        int cur = 0;
        for (char c : s) {
            int ch = c - 'a';
            if (nodes[cur].children[ch] == -1) {
                nodes[cur].children[ch] = nodes.size();
                nodes.emplace_back();
            }
            cur = nodes[cur].children[ch];
            nodes[cur].count++;
        }
        nodes[cur].isEnd = true;
    }

    bool search(const string& s) {
        int cur = 0;
        for (char c : s) {
            int ch = c - 'a';
            if (nodes[cur].children[ch] == -1) return false;
            cur = nodes[cur].children[ch];
        }
        return nodes[cur].isEnd;
    }

    bool startsWith(const string& prefix) {
        int cur = 0;
        for (char c : prefix) {
            int ch = c - 'a';
            if (nodes[cur].children[ch] == -1) return false;
            cur = nodes[cur].children[ch];
        }
        return true;
    }

    // Count words with given prefix
    int countPrefix(const string& prefix) {
        int cur = 0;
        for (char c : prefix) {
            int ch = c - 'a';
            if (nodes[cur].children[ch] == -1) return 0;
            cur = nodes[cur].children[ch];
        }
        return nodes[cur].count;
    }

    // Delete a word (if exists)
    bool erase(const string& s) {
        if (!search(s)) return false;
        int cur = 0;
        for (char c : s) {
            int ch = c - 'a';
            cur = nodes[cur].children[ch];
            nodes[cur].count--;
        }
        nodes[cur].isEnd = false;
        return true;
    }
};

// ─────────────────────────────────────────────
// 2. POINTER-BASED TRIE (unordered_map for any alphabet)
// ─────────────────────────────────────────────
struct TrieNodeMap {
    unordered_map<char, TrieNodeMap*> children;
    bool isEnd = false;
    int count = 0;
};

struct TrieMap {
    TrieNodeMap* root;
    TrieMap() : root(new TrieNodeMap()) {}

    void insert(const string& s) {
        TrieNodeMap* cur = root;
        for (char c : s) {
            if (!cur->children.count(c))
                cur->children[c] = new TrieNodeMap();
            cur = cur->children[c];
            cur->count++;
        }
        cur->isEnd = true;
    }

    bool search(const string& s) {
        TrieNodeMap* cur = root;
        for (char c : s) {
            if (!cur->children.count(c)) return false;
            cur = cur->children[c];
        }
        return cur->isEnd;
    }
};

// ─────────────────────────────────────────────
// 3. BINARY TRIE – for XOR problems
// ─────────────────────────────────────────────
// Store integers as 30-bit binary strings.
// Find maximum XOR of two numbers.
struct BinaryTrie {
    static const int BITS = 30;
    int ch[2];
    BinaryTrie() { ch[0] = ch[1] = -1; }
};

struct XORTrie {
    vector<BinaryTrie> nodes;

    XORTrie() { nodes.emplace_back(); }

    void insert(int x) {
        int cur = 0;
        for (int i = BinaryTrie::BITS; i >= 0; i--) {
            int bit = (x >> i) & 1;
            if (nodes[cur].ch[bit] == -1) {
                nodes[cur].ch[bit] = nodes.size();
                nodes.emplace_back();
            }
            cur = nodes[cur].ch[bit];
        }
    }

    // Find max XOR of x with any number in trie
    int maxXOR(int x) {
        int cur = 0, result = 0;
        for (int i = BinaryTrie::BITS; i >= 0; i--) {
            int bit = (x >> i) & 1;
            int want = 1 - bit;  // prefer opposite bit for max XOR
            if (nodes[cur].ch[want] != -1) {
                result |= (1 << i);
                cur = nodes[cur].ch[want];
            } else if (nodes[cur].ch[bit] != -1) {
                cur = nodes[cur].ch[bit];
            } else break;
        }
        return result;
    }
};

// ─────────────────────────────────────────────
// 4. WORD SEARCH II – find all words in grid  O(m*n*L*26)
// ─────────────────────────────────────────────
// Classic problem: given grid and word list, find all words in grid.
// Build trie from words, then DFS on grid.
vector<string> findWords(vector<vector<char>>& board, vector<string>& words) {
    Trie trie;
    for (auto& w : words) trie.insert(w);

    int rows = board.size(), cols = board[0].size();
    vector<string> result;
    int dx[] = {0,0,1,-1}, dy[] = {1,-1,0,0};

    function<void(int,int,int,string&)> dfs = [&](int r, int c, int node, string& path) {
        if (trie.nodes[node].isEnd) {
            result.push_back(path);
            trie.nodes[node].isEnd = false;  // avoid duplicates
        }
        if (r < 0 || r >= rows || c < 0 || c >= cols || board[r][c] == '#') return;
        char orig = board[r][c];
        int ch = orig - 'a';
        int next = trie.nodes[node].children[ch];
        if (next == -1) return;
        board[r][c] = '#';  // mark visited
        path += orig;
        for (int d = 0; d < 4; d++) dfs(r+dx[d], c+dy[d], next, path);
        path.pop_back();
        board[r][c] = orig;
    };

    string path;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            dfs(r, c, 0, path);
    return result;
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Longest word in dictionary: find longest word that can be built
 *     one char at a time, each prefix also in dictionary.
 *     (insert all, then for each word check all prefixes)
 *
 * [2] Replace words: given list of roots and sentence, replace each word
 *     with its shortest root. (Build trie from roots, scan sentence)
 *
 * [3] Palindrome pairs: given words list, find all (i,j) pairs
 *     where words[i]+words[j] is a palindrome.
 *
 * [4] Maximum XOR of two numbers in array.
 *     Insert all nums in XOR trie, for each num find max XOR.
 *
 * [5] Maximum XOR subarray:
 *     prefix XOR + XOR trie – classic technique.
 */

// Exercise [4] – Maximum XOR
int maximumXOR(vector<int>& nums) {
    XORTrie trie;
    for (int x : nums) trie.insert(x);
    int result = 0;
    for (int x : nums) result = max(result, trie.maxXOR(x));
    return result;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Basic Trie
    Trie t;
    t.insert("apple");
    t.insert("app");
    t.insert("apricot");
    t.insert("banana");

    cout << "search 'apple': " << t.search("apple") << "\n";     // 1
    cout << "search 'app': " << t.search("app") << "\n";         // 1
    cout << "search 'ap': " << t.search("ap") << "\n";           // 0
    cout << "startsWith 'ap': " << t.startsWith("ap") << "\n";   // 1
    cout << "startsWith 'bana': " << t.startsWith("bana") << "\n";  // 1
    cout << "countPrefix 'app': " << t.countPrefix("app") << "\n";  // 2 (apple, app)

    t.erase("app");
    cout << "after erase 'app', search: " << t.search("app") << "\n";  // 0
    cout << "apple still exists: " << t.search("apple") << "\n";       // 1

    // XOR Trie
    vector<int> nums = {3, 10, 5, 25, 2, 8};
    cout << "Max XOR: " << maximumXOR(nums) << "\n";  // 28 (5 XOR 25)

    // TrieMap with special chars
    TrieMap tm;
    tm.insert("hello-world");
    tm.insert("hello-cpp");
    cout << "search 'hello-world': " << tm.search("hello-world") << "\n";  // 1
    cout << "search 'hello': " << tm.search("hello") << "\n";              // 0

    return 0;
}
