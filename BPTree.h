#ifndef BPTREE_H
#define BPTREE_H

#define LEAF true
#include <iostream>
#include <fstream> 
#include <vector>
#include <queue>
#include <sys/socket.h>

template<typename KeyT, typename ValT>
class Node {
public:
    bool leaf;
    Node* parent;   //for non-root only
    Node* next;     //for leaf only
    std::vector<KeyT> key;
    std::vector<Node*> ptr2node;    //for non-leaf only
    std::vector<ValT*> ptr2val;     //for leaf only
    Node(bool _leaf = false);
};

template<typename KeyT, typename ValT>
class BPTree {
private:
    int order;
    Node<KeyT, ValT>* root;
    inline int keyIndex(Node<KeyT, ValT>* _node, KeyT _key);
    inline std::pair<Node<KeyT, ValT>*, int> keyIndexInLeaf(KeyT _key);
    Node<KeyT, ValT>* splitLeaf(Node<KeyT, ValT>* _leaf);
    void createIndex(Node<KeyT, ValT>* _new_node, KeyT _index);
    std::pair<Node<KeyT, ValT>*, KeyT> splitNode(Node<KeyT, ValT>* _node);
    void handleNodeUnderflow(Node<KeyT, ValT>* node);
    void handleLeafUnderflow(Node<KeyT, ValT>* leaf);

public:
    BPTree(int order);
    void insert(KeyT _key, ValT _val);
    void erase(KeyT _key);
    bool update(KeyT _key, ValT _new_val);
    ValT* find(KeyT _key);
    void deserialize(const std::string& filename);
    void serialize(const std::string& filename);
};

template<typename KeyT, typename ValT>
inline int BPTree<KeyT, ValT>::keyIndex(Node<KeyT, ValT>* _node, KeyT _key) {
    int loc = -1;
    int size = _node->key.size();
    while (loc + 1 < size && _node->key[loc + 1] <= _key) {
        loc++;
    }
    return loc;
}

template<typename KeyT, typename ValT>
inline std::pair<Node<KeyT, ValT>*, int> BPTree<KeyT, ValT>::keyIndexInLeaf(KeyT _key) {
    if (root == nullptr) {
        return std::make_pair(nullptr, -1);
    }
    Node<KeyT, ValT>* node = root;
    while (true) {
        int loc = keyIndex(node, _key);
        if (node->leaf) {
            return std::make_pair(node, loc);
        }
        else {
            node = node->ptr2node[loc + 1];
        }
    }
}

template<typename KeyT, typename ValT>
Node<KeyT, ValT>* BPTree<KeyT, ValT>::splitLeaf(Node<KeyT, ValT>* _leaf) {
    Node<KeyT, ValT>* new_leaf = new Node<KeyT, ValT>(LEAF);
    new_leaf->next = _leaf->next;
    _leaf->next = new_leaf;
    new_leaf->parent = _leaf->parent;
    int mid = _leaf->key.size() / 2;
    new_leaf->key.assign(_leaf->key.begin() + mid, _leaf->key.end());
    new_leaf->ptr2val.assign(_leaf->ptr2val.begin() + mid, _leaf->ptr2val.end());
    _leaf->key.erase(_leaf->key.begin() + mid, _leaf->key.end());
    _leaf->ptr2val.erase(_leaf->ptr2val.begin() + mid, _leaf->ptr2val.end());
    return new_leaf;
}

template<typename KeyT, typename ValT>
std::pair<Node<KeyT, ValT>*, KeyT> BPTree<KeyT, ValT>::splitNode(Node<KeyT, ValT>* _node) {
    Node<KeyT, ValT>* new_node = new Node<KeyT, ValT>();
    new_node->parent = _node->parent;
    int mid = (_node->key.size() + 1) / 2 - 1;
    KeyT push_key = _node->key[mid];
    new_node->key.assign(_node->key.begin() + mid + 1, _node->key.end());
    new_node->ptr2node.assign(_node->ptr2node.begin() + mid + 1, _node->ptr2node.end());
    _node->key.erase(_node->key.begin() + mid, _node->key.end());
    _node->ptr2node.erase(_node->ptr2node.begin() + mid + 1, _node->ptr2node.end());
    for (Node<KeyT, ValT>* each : new_node->ptr2node)
        each->parent = new_node;
    return std::make_pair(new_node, push_key);
}

template<typename KeyT, typename ValT>
void BPTree<KeyT, ValT>::createIndex(Node<KeyT, ValT>* _new_node, KeyT _index) {
    Node<KeyT, ValT>* node = _new_node->parent;
    int loc = keyIndex(node, _index);
    node->key.insert(node->key.begin() + loc + 1, _index);
    node->ptr2node.insert(node->ptr2node.begin() + loc + 2, _new_node);
    if (node->key.size() > order) {
        std::pair<Node<KeyT, ValT>*, KeyT> pair = splitNode(node);
        Node<KeyT, ValT>* new_node = pair.first;
        KeyT push_key = pair.second;
        if (node == root) {
            Node<KeyT, ValT>* new_root = new Node<KeyT, ValT>();
            new_root->key.push_back(push_key);
            new_root->ptr2node.push_back(node);
            new_root->ptr2node.push_back(new_node);
            root = new_root;
            node->parent = root;
            new_node->parent = root;
        }
        else {
            createIndex(new_node, push_key);
        }
    }
}

template<typename KeyT, typename ValT>
Node<KeyT, ValT>::Node(bool _leaf) : leaf(_leaf), parent(nullptr), next(nullptr) {}

template<typename KeyT, typename ValT>
BPTree<KeyT, ValT>::BPTree(int order) : root(nullptr), order(order) {}


template<typename KeyT, typename ValT>
void BPTree<KeyT, ValT>::insert(KeyT _key, ValT _val) {
    if (root == nullptr) {
        root = new Node<KeyT, ValT>(LEAF);
        root->key.push_back(_key);
        root->ptr2val.emplace_back(new ValT(_val));
        root->ptr2node.push_back(nullptr);
        return;
    }
    std::pair<Node<KeyT, ValT>*, int> pair = keyIndexInLeaf(_key);
    Node<KeyT, ValT>* leaf = pair.first;
    int loc = pair.second;
    if (loc != -1 && leaf->key[loc] == _key) {
        std::cout << "Key " << _key << " with value " << *(leaf->ptr2val[loc]) << " is already in B+ tree, overwrite it with new val " << _val << std::endl;
        *(leaf->ptr2val[loc]) = _val;
        return;
    }
    leaf->key.insert(leaf->key.begin() + loc + 1, _key);
    leaf->ptr2val.insert(leaf->ptr2val.begin() + loc + 1, new ValT(_val));
    if (leaf->key.size() > order) {
        Node<KeyT, ValT>* new_leaf = splitLeaf(leaf);
        if (leaf == root) {
            Node<KeyT, ValT>* new_root = new Node<KeyT, ValT>();
            new_root->key.push_back(new_leaf->key[0]);
            new_root->ptr2node.push_back(leaf);
            new_root->ptr2node.push_back(new_leaf);
            root = new_root;
            leaf->parent = root;
            new_leaf->parent = root;
        }
        else {
            createIndex(new_leaf, new_leaf->key[0]);
        }
    }
}
template<typename KeyT, typename ValT>
void BPTree<KeyT, ValT>::erase(KeyT _key) {
    std::pair<Node<KeyT, ValT>*, int> pair = keyIndexInLeaf(_key);
    Node<KeyT, ValT>* leaf = pair.first;
    int loc = pair.second;

    if (loc == -1 || leaf->key[loc] != _key) {
        std::cout << "Key " << _key << " is not in B+ tree" << std::endl;
        return;
    }

    leaf->key.erase(leaf->key.begin() + loc);
    delete leaf->ptr2val[loc];
    leaf->ptr2val.erase(leaf->ptr2val.begin() + loc);

    // no underflow
    if (leaf->key.size() >= (order + 1) / 2 || leaf == root) {
        return;
    }

    handleLeafUnderflow(leaf);
}

template<typename KeyT, typename ValT>
void BPTree<KeyT, ValT>::handleLeafUnderflow(Node<KeyT, ValT>* leaf) {
    Node<KeyT, ValT>* parent = leaf->parent;
    int idx = -1;

    // index in parent node
    for (size_t i = 0; i < parent->ptr2node.size(); ++i) {
        if (parent->ptr2node[i] == leaf) {
            idx = i;
            break;
        }
    }

    // borrow from left sib
    if (idx > 0) {
        Node<KeyT, ValT>* left_sibling = parent->ptr2node[idx - 1];
        if (left_sibling->key.size() > (order + 1) / 2) {
            // borrow
            leaf->key.insert(leaf->key.begin(), left_sibling->key.back());
            leaf->ptr2val.insert(leaf->ptr2val.begin(), left_sibling->ptr2val.back());
            left_sibling->key.pop_back();
            left_sibling->ptr2val.pop_back();

            // update parent index
            parent->key[idx - 1] = leaf->key[0];
            return;
        }
    }

    // borrow from right sib
    if (idx < parent->ptr2node.size() - 1) {
        Node<KeyT, ValT>* right_sibling = parent->ptr2node[idx + 1];
        if (right_sibling->key.size() > (order + 1) / 2) {
            // borrow
            leaf->key.push_back(right_sibling->key.front());
            leaf->ptr2val.push_back(right_sibling->ptr2val.front());
            right_sibling->key.erase(right_sibling->key.begin());
            right_sibling->ptr2val.erase(right_sibling->ptr2val.begin());

            // update parent index
            parent->key[idx] = right_sibling->key[0];
            return;
        }
    }

    // can't borrow, merge
    if (idx > 0) {
        // left
        Node<KeyT, ValT>* left_sibling = parent->ptr2node[idx - 1];
        left_sibling->key.insert(left_sibling->key.end(), leaf->key.begin(), leaf->key.end());
        left_sibling->ptr2val.insert(left_sibling->ptr2val.end(), leaf->ptr2val.begin(), leaf->ptr2val.end());
        left_sibling->next = leaf->next;

        parent->key.erase(parent->key.begin() + idx - 1);
        parent->ptr2node.erase(parent->ptr2node.begin() + idx);

        delete leaf;

        if (parent->key.size() < (order + 1) / 2 && parent != root) {
            handleNodeUnderflow(parent);
        }
    }
    else {
        // right
        Node<KeyT, ValT>* right_sibling = parent->ptr2node[idx + 1];
        leaf->key.insert(leaf->key.end(), right_sibling->key.begin(), right_sibling->key.end());
        leaf->ptr2val.insert(leaf->ptr2val.end(), right_sibling->ptr2val.begin(), right_sibling->ptr2val.end());
        leaf->next = right_sibling->next;

        parent->key.erase(parent->key.begin() + idx);
        parent->ptr2node.erase(parent->ptr2node.begin() + idx + 1);

        delete right_sibling;

        if (parent->key.size() < (order + 1) / 2 && parent != root) {
            handleNodeUnderflow(parent);
        }
    }

    // parent is empty
    if (root->key.empty() && !root->ptr2node.empty()) {
        Node<KeyT, ValT>* new_root = root->ptr2node[0];
        delete root;
        root = new_root;
        root->parent = nullptr;
    }
}

template<typename KeyT, typename ValT>
void BPTree<KeyT, ValT>::handleNodeUnderflow(Node<KeyT, ValT>* node) {
    Node<KeyT, ValT>* parent = node->parent;
    int idx = -1;

    for (size_t i = 0; i < parent->ptr2node.size(); ++i) {
        if (parent->ptr2node[i] == node) {
            idx = i;
            break;
        }
    }

    // borrow from left
    if (idx > 0) {
        Node<KeyT, ValT>* left_sibling = parent->ptr2node[idx - 1];
        if (left_sibling->key.size() > (order + 1) / 2) {
            // borrow
            node->key.insert(node->key.begin(), parent->key[idx - 1]);
            node->ptr2node.insert(node->ptr2node.begin(), left_sibling->ptr2node.back());
            left_sibling->ptr2node.back()->parent = node;

            // update parent index
            parent->key[idx - 1] = left_sibling->key.back();
            left_sibling->key.pop_back();
            left_sibling->ptr2node.pop_back();
            return;
        }
    }

    // borrow from right
    if (idx < parent->ptr2node.size() - 1) {
        Node<KeyT, ValT>* right_sibling = parent->ptr2node[idx + 1];
        if (right_sibling->key.size() > (order + 1) / 2) {
            // borrow
            node->key.push_back(parent->key[idx]);
            node->ptr2node.push_back(right_sibling->ptr2node.front());
            right_sibling->ptr2node.front()->parent = node;

            // update parent index
            parent->key[idx] = right_sibling->key.front();
            right_sibling->key.erase(right_sibling->key.begin());
            right_sibling->ptr2node.erase(right_sibling->ptr2node.begin());
            return;
        }
    }

    // can't borrow, merge
    if (idx > 0) {
        // left
        Node<KeyT, ValT>* left_sibling = parent->ptr2node[idx - 1];
        left_sibling->key.push_back(parent->key[idx - 1]);
        left_sibling->key.insert(left_sibling->key.end(), node->key.begin(), node->key.end());
        left_sibling->ptr2node.insert(left_sibling->ptr2node.end(), node->ptr2node.begin(), node->ptr2node.end());

        for (auto& child : node->ptr2node) {
            child->parent = left_sibling;
        }

        parent->key.erase(parent->key.begin() + idx - 1);
        parent->ptr2node.erase(parent->ptr2node.begin() + idx);

        delete node;

        if (parent->key.size() < (order + 1) / 2 && parent != root) {
            handleNodeUnderflow(parent);
        }
    }
    else {
        // right
        Node<KeyT, ValT>* right_sibling = parent->ptr2node[idx + 1];
        node->key.push_back(parent->key[idx]);
        node->key.insert(node->key.end(), right_sibling->key.begin(), right_sibling->key.end());
        node->ptr2node.insert(node->ptr2node.end(), right_sibling->ptr2node.begin(), right_sibling->ptr2node.end());

        for (auto& child : right_sibling->ptr2node) {
            child->parent = node;
        }

        parent->key.erase(parent->key.begin() + idx);
        parent->ptr2node.erase(parent->ptr2node.begin() + idx + 1);

        delete right_sibling;

        if (parent->key.size() < (order + 1) / 2 && parent != root) {
            handleNodeUnderflow(parent);
        }
    }

    // parent is empty
    if (root->key.empty() && !root->ptr2node.empty()) {
        Node<KeyT, ValT>* new_root = root->ptr2node[0];
        delete root;
        root = new_root;
        root->parent = nullptr;
    }
}

template<typename KeyT, typename ValT>
ValT* BPTree<KeyT, ValT>::find(KeyT _key) {
    std::pair<Node<KeyT, ValT>*, int> pair = keyIndexInLeaf(_key);
    Node<KeyT, ValT>* leaf = pair.first;
    int loc = pair.second;
    if (loc == -1 || leaf->key[loc] != _key) {
        // std::cout << "Key " << _key << " is not in B+ tree" << std::endl;
        return nullptr;
    }
    else {
        return leaf->ptr2val[loc];
    }
}


template<typename KeyT, typename ValT>
bool BPTree<KeyT, ValT>::update(KeyT _key, ValT _new_val) {
    std::pair<Node<KeyT, ValT>*, int> pair = keyIndexInLeaf(_key);
    Node<KeyT, ValT>* leaf = pair.first;
    int loc = pair.second;
    if (loc == -1 || leaf->key[loc] != _key) {
        std::cout << "Key " << _key << " is not in B+ tree" << std::endl;
        return false;
    }
    else {
        *(leaf->ptr2val[loc]) = _new_val;
        return true;
    }
}

template<typename KeyT, typename ValT>
void BPTree<KeyT, ValT>::serialize(const std::string& filename) {
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        std::cerr << "Error opening file for writing!" << std::endl;
        return;
    }

    std::queue<Node<KeyT, ValT>*> q;
    if (root) q.push(root);

    while (!q.empty()) {
        Node<KeyT, ValT>* node = q.front();
        q.pop();

        // Write node type (leaf or not)
        outfile.write(reinterpret_cast<char*>(&node->leaf), sizeof(node->leaf));

        // Write keys
        size_t key_count = node->key.size();
        outfile.write(reinterpret_cast<char*>(&key_count), sizeof(key_count));
        for (const auto& key : node->key) {
            outfile.write(reinterpret_cast<const char*>(&key), sizeof(key));
        }

        if (node->leaf) {
            // Write values
            for (const auto& val_ptr : node->ptr2val) {
                outfile.write(reinterpret_cast<const char*>(val_ptr), sizeof(ValT));
            }
        }
        else {
            // Write child pointers (push children to queue)
            for (const auto& child : node->ptr2node) {
                q.push(child);
            }
        }
    }

    outfile.close();
}
template<typename KeyT, typename ValT>
void BPTree<KeyT, ValT>::deserialize(const std::string& filename) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile) {
        std::cerr << "Error opening file for reading!" << std::endl;
        return;
    }

    std::queue<Node<KeyT, ValT>*> parent_queue; // Queue to track parent nodes
    Node<KeyT, ValT>* current_node = nullptr;
    root = nullptr;

    while (infile) {
        bool is_leaf;
        infile.read(reinterpret_cast<char*>(&is_leaf), sizeof(is_leaf));

        if (infile.eof()) break;

        current_node = new Node<KeyT, ValT>(is_leaf);

        // Read keys
        size_t key_count;
        infile.read(reinterpret_cast<char*>(&key_count), sizeof(key_count));
        current_node->key.resize(key_count);
        for (size_t i = 0; i < key_count; ++i) {
            infile.read(reinterpret_cast<char*>(&current_node->key[i]), sizeof(KeyT));
        }

        if (is_leaf) {
            // Read values
            current_node->ptr2val.resize(key_count);
            for (size_t i = 0; i < key_count; ++i) {
                current_node->ptr2val[i] = new ValT();
                infile.read(reinterpret_cast<char*>(current_node->ptr2val[i]), sizeof(ValT));
            }
        }
        else {
            // Reserve space for child pointers
            current_node->ptr2node.resize(key_count + 1, nullptr);
        }

        // Link to parent
        if (root == nullptr) {
            root = current_node;
        }
        else {
            Node<KeyT, ValT>* parent = parent_queue.front();
            parent_queue.pop();

            // Find the correct position to insert the child
            for (size_t i = 0; i < parent->ptr2node.size(); ++i) {
                if (parent->ptr2node[i] == nullptr) {
                    parent->ptr2node[i] = current_node;
                    current_node->parent = parent;
                    break;
                }
            }
        }

        // If not leaf, push to parent queue for children
        if (!is_leaf) {
            for (size_t i = 0; i < key_count + 1; ++i) {
                parent_queue.push(current_node);
            }
        }
    }

    infile.close();
}

#endif // BPTREE_H