#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>


template<typename K, typename V, std::size_t N = 4>
class BTree {

static_assert(N >= 2, "N must be greater or equal to 2");

private:
    struct Node {
        std::size_t size;
        std::pair<K, V> entries[N + 1];
        Node* children[N + 2];

        public:
        Node() {}

        // print DOT representation
        int toString(std::ostringstream& sb, int from, unsigned depth, unsigned height) const {
            int mine = from++;
            sb << "\tstruct" << mine << " [label=\"";
            for (size_t i = 0; i < 2 * size + 1; i++) {
                if (i > 0) {
                    sb << "|";
                }
                sb << "<f" << i << "> ";
                if ((i & 1) == 1) {
                    sb << entries[i / 2].first;
                }
            }
            sb << "\"];" << std::endl;

            if (depth < height) {
                for (size_t i = 0; i < size + 1; i++) {
                    int field = 2 * i;
                    sb << "\tstruct" << mine << ":<f" << field << "> -> struct" << from << ";" << std::endl;
                    from = children[i]->toString(sb, from, depth + 1, height);
                }
            }

            return from;            
        }
    };

    Node* root = nullptr;
    unsigned height = 0;
    std::size_t elementCount = 0;

    bool findKeyInNode(Node* node, const K& key, std::size_t& idx);

    void splitRoot(Node* node);

    void split(Node* parent, std::size_t idx, Node* node);

    void insert_aux(unsigned depth, Node* node, auto&& key, auto&& value);

    V& find_aux(Node* node, const K& key, unsigned depth) const;

    void free_all(Node* node, unsigned depth);

public:
    BTree() : root(nullptr) {};

    ~BTree() {
        free_all(root, 1);
    };

    BTree(const BTree &other) = delete;

    BTree &operator=(const BTree &other) = delete;

    BTree(const BTree &&other) {
        root = other.root;
        elementCount = other.elementCount;
        height = other.height;
    };

    BTree &operator=(const BTree &&other)  {
        root = other.root;
        elementCount = other.elementCount;
        height = other.height;
    };

    std::size_t size() const { return elementCount; }

    void insert(const K&, const V&);

    void insert(K&&, V&&);

    bool erase(const K&);

    bool erase_aux(unsigned depth, Node* node, const K& key);

    void erase_all();

    V& at(const K& key);

    const V& at(const K& key) const;

    std::string toString() const {
        std::ostringstream sb;
        sb << "digraph {" << std::endl;
        sb << "\tnode [shape=record];" << std::endl;
        if (root != nullptr) {
            root->toString(sb, 0, 1, height);
        }
        sb << "}";
        return sb.str();
    }
};

/* =========== IMPLEMENTATION =========== */

template<typename K, typename V, std::size_t N>
static inline void insertNode(auto* node, std::size_t i, auto&& key, auto&& value, auto* child) {
    assert(node->size <= N);
    
    for (std::size_t j = node->size; j > i; j--) {
        node->entries[j] = std::move(node->entries[j - 1]);
        node->children[j + 1] = node->children[j];
    }

    node->entries[i].first = key;
    node->entries[i].second = value;
    node->children[i + 1] = child;
    node->size++;
}

template<typename K, typename V, std::size_t N>
static inline void removeKeyFromNode(auto* node, std::size_t i) {
    assert(node->size <= N);
    
    for (std::size_t j = i; j < node->size - 1; j++) {
        node->entries[j] = std::move(node->entries[j + 1]);
        node->children[j] = node->children[j + 1];
    }

    node->children[node->size - 1] = node->children[node->size];
    node->size--;
}

template<typename K, typename V, std::size_t N>
bool BTree<K, V, N>::findKeyInNode(Node* node, const K& key, std::size_t& idx) {

    if constexpr (N < 10) {
        // linear search
        for(idx = node->size; idx > 0; idx--) {
            if(key == node->keys[idx - 1]) {
                return true;
            }

            if(key > node->keys[idx - 1]) {
                break;
            }
        }

    } else {
        // binary search
        std::size_t start = 0;
        std::size_t end = node->size;

        while (start < end) {
            std::size_t pivot = start + (end - start) / 2;

            if (node->keys[pivot] == key) {
                idx = pivot;
                return true;
            }

            else if (node->keys[pivot] < key) {
                start = pivot + 1;
            } else {
                end = pivot;
            }
        }

        idx = start;
    }

    return false;

    
}

template<typename K, typename V, std::size_t N>
void BTree<K, V, N>::splitRoot(Node* node) {
    constexpr std::size_t splitIndex = N / 2;

    Node* left = new Node();
    Node* right = new Node();

    for(std::size_t k = 0; k < splitIndex; k++) {
        left->entries[k] = std::move(node->entries[k]);
        left->children[k] = node->children[k];

        right->entries[k] = std::move(node->entries[k + splitIndex + 1]);
        right->children[k] = node->children[k + splitIndex + 1];
    }
    
    left->children[splitIndex] = node->children[splitIndex];
    right->children[splitIndex] = node->children[2 * splitIndex + 1];

    if constexpr (N % 2 != 0) {
        right->entries[splitIndex] = std::move(node->entries[N]);
        right->children[splitIndex + 1] = node->children[N + 1];

        right->size = splitIndex + 1;
        left->size = splitIndex;

    } else {
        right->size = splitIndex;
        left->size = splitIndex;
    }
    
    node->entries[0] = std::move(node->entries[splitIndex]);

    node->children[0] = left;
    node->children[1] = right;
    node->size = 1;
    height++;
}


template<typename K, typename V, std::size_t N>
void BTree<K, V, N>::split(Node* parent, std::size_t idx, Node* node) {
    constexpr std::size_t splitIndex = N / 2;
    
    assert(parent);

    Node* right = new Node();
    
    for(std::size_t k = 0; k < splitIndex; k++) {
        right->entries[k] = std::move(node->entries[k + splitIndex + 1]);
        right->children[k] = node->children[k + splitIndex + 1];
    }
    
    right->children[splitIndex] = node->children[2 * splitIndex + 1];

    if constexpr (N % 2 != 0) {
        right->entries[splitIndex] = std::move(node->entries[N]);
        right->children[splitIndex + 1] = node->children[N + 1];

        right->size = splitIndex + 1;
    } else {
        right->size = splitIndex;
    }
    
    insertNode<K, V, N>(parent, idx,  std::move(node->entries[splitIndex].first), std::move(node->entries[splitIndex].second), right);
    parent->children[idx]->size = splitIndex;
}

template<typename K, typename V, std::size_t N>
void BTree<K, V, N>::insert(const K& key, const V& value) {
    if(!root) {
        root = new Node();
        root->entries[0] = {key, value};
        root->size = 1;
        elementCount = 1;
        height = 1;

    } else {
        elementCount++;
        insert_aux(1, root, key, value);
        
        if (root->size > N) {
            splitRoot(root);
        }
    }
}

template<typename K, typename V, std::size_t N>
void BTree<K, V, N>::insert(K&& key, V&& value) {
    if(!root) {
        root = new Node();
        root->entries[0].first = std::move(key);
        root->entries[0].second = std::move(value);
        root->size = 1;
        elementCount = 1;
        height = 1;

    } else {
        elementCount++;
        insert_aux(1, root, std::move(key), std::move(value));
        
        if (root->size > N) {
            splitRoot(root);
        }
    }
}


template<typename K, typename V, std::size_t N>
void BTree<K, V, N>::insert_aux(unsigned depth, Node* node, auto&& key, auto&& value) {

    std::size_t idx;
    if (findKeyInNode(node, key, idx)) {
        // key already exists -> update value but keep size the same
        node->entries[idx].second = std::move(value);
        elementCount--;
        return;
    }

    Node* c = node->children[idx];

    if(depth < height) { // if not leaf
        insert_aux(depth + 1, c, key, value);
        if(c->size > N) {
            split(node, idx, c);
        }
    } else {
        assert(node->size - 1 <= N);
        insertNode<K, V, N>(node, idx, std::forward<decltype(key)>(key),
                            std::forward<decltype(value)>(value),
                            static_cast<Node*>(nullptr));
    }

    assert(node->size <= N + 1);
}


template<typename K, typename V, std::size_t N>
const V& BTree<K, V, N>::at(const K& key) const {
    if (!root)
        return false;

    return find_aux(root, key, 1);
}

template<typename K, typename V, std::size_t N>
V& BTree<K, V, N>::at(const K& key) {
    if (!root)
        throw std::out_of_range("key");

    return find_aux(root, key, 1);
}

template<typename K, typename V, std::size_t N>
V& BTree<K, V, N>::find_aux(Node* node, const K& key, unsigned depth) const {
    bool isInnerNode = depth < height;

    for(int i = node->size - 1; i >= 0; i--) {
        if(key == node->entries[i].first) 
            return node->entries[i].second;

        if(key > node->entries[i].first) {
            if (isInnerNode) {
                return find_aux(node->children[i + 1], key, depth + 1);
            } else {
                throw std::out_of_range("key");
            }
        }
    }

    if (isInnerNode) {
        return find_aux(node->children[0], key, depth + 1);
    } else {
        throw std::out_of_range("key");
    }
}

template<typename K, typename V, std::size_t N>
bool BTree<K, V, N>::erase_aux(unsigned depth, Node* node, const K& key) {

    // find key in current node
    std::size_t idx;
    bool isLeaf = depth >= height;
    bool found = findKeyInNode(node, key, idx);

    if (isLeaf) {
        if (found) {
            // remove from leaf
            removeKeyFromNode<K, V, N>(node, idx);
            return true;

        } else {
            // nothing to do
            return false;
        }

    }

    bool retval;
    Node* child = node->children[idx];

    if (found) {
        // find next smallest and swap with current Node
        unsigned currentDepth = depth + 1;
        Node* nextSmallest = child;
        while (currentDepth < height) {
            nextSmallest = nextSmallest->children[nextSmallest->size];
            currentDepth++;
        }
    
        const K& nextSmallestKey = nextSmallest->entries[nextSmallest->size - 1].first;
        node->entries[idx] = std::move(nextSmallest->entries[nextSmallest->size - 1]);
        retval = erase_aux(depth + 1, child, std::move(nextSmallestKey));

    } else {
        retval = erase_aux(depth + 1, child, key);
    }
    
    // rebalance tree
    constexpr std::size_t MIN_KEYS = N / 2;

    if (child->size >= MIN_KEYS) {
        return retval;
    }

    assert(child->size == MIN_KEYS - 1);

    // try steal child from left sibling
    Node* leftSibling = idx > 0 ? node->children[idx - 1] : nullptr;
    if (leftSibling && leftSibling->size > MIN_KEYS) {

        // rotate keys
        insertNode<K, V, N>(child, 0, std::move(node->entries[idx - 1].first), std::move(node->entries[idx - 1].second), child->children[0]);
        child->children[0] = leftSibling->children[leftSibling->size];
        node->entries[idx - 1] = std::move(leftSibling->entries[leftSibling->size - 1]);

        leftSibling->size--;
        return retval;
    }

    // try steal child from right sibling
    Node* rightSibling = idx < node->size ? node->children[idx + 1] : nullptr;
    if (rightSibling && rightSibling->size > MIN_KEYS) {

        // rotate keys
        insertNode<K, V, N>(child, MIN_KEYS - 1, 
            std::move(node->entries[idx].first),  std::move(node->entries[idx].second), rightSibling->children[0]);
            
        node->entries[idx].first = std::move(rightSibling->entries[0].first);
        node->entries[idx] = std::move(rightSibling->entries[0]);

        removeKeyFromNode<K, V, N>(rightSibling, 0);
        return retval;
    }

    // merge with left
    if (leftSibling) {
        assert(leftSibling->size == MIN_KEYS);

        leftSibling->entries[MIN_KEYS] = std::move(node->entries[idx - 1]);
        leftSibling->children[MIN_KEYS + 1] = child->children[0];

        for (std::size_t i = 0; i < MIN_KEYS - 1; i++) {
            leftSibling->entries[MIN_KEYS + i + 1] = std::move(child->entries[i]);
            leftSibling->children[MIN_KEYS + i + 2] = child->children[i + 1];
        }

        assert(!isLeaf);
        delete child;

        removeKeyFromNode<K, V, N>(node, idx - 1); // remove child

        leftSibling->size = 2 * MIN_KEYS;
        node->children[idx - 1] = leftSibling;

    } else {
        assert(rightSibling);
        assert(rightSibling->size == MIN_KEYS);

        child->entries[MIN_KEYS - 1] = std::move(node->entries[idx]);
        child->children[MIN_KEYS] = rightSibling->children[0];

        for (std::size_t i = 0; i < MIN_KEYS; i++) {
            child->entries[MIN_KEYS + i] = std::move(rightSibling->entries[i]);
            child->children[MIN_KEYS + i + 1] = rightSibling->children[i + 1];
        }

        assert(!isLeaf);
        delete rightSibling;

        // remove right sibling by shifting nodes
        removeKeyFromNode<K, V, N>(node, idx);
        node->children[0] = child;
        child->size = 2 * MIN_KEYS;
    }

    return retval;
}

template<typename K, typename V, std::size_t N>
bool BTree<K, V, N>::erase(const K& key) {
    if (!root)
        return false;
    
    if (height == 1 && root->size == 1 && root->entries[0].first == key) {
        height = 0;
        elementCount = 0;
        delete root;
        root = nullptr;
        return true;
    }

    elementCount--;
    bool retval = erase_aux(1, root, key);

    // check if height needs to shrink
    if (root->size == 0) {
        Node* newRoot = root->children[0];
        delete root;
        root = newRoot;
        height--;
    }

    return retval;
}

template<typename K, typename V, std::size_t N>
void BTree<K, V, N>::free_all(Node* node, unsigned depth) {
    if (depth < height) {
        for (std::size_t i = 0; i <= node->size; i++) {
            Node* child = node->children[i];
            freeNodes(child, depth + 1);
        }
    }

    delete node;
}

template<typename K, typename V, std::size_t N>
void BTree<K, V, N>:: erase_all() {
    free_all(root, 1);
    elementCount = 0;
    height = 0;
    root = nullptr;
}
