#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>


template<typename K, typename V, std::size_t N = 4>
class BPlusTree {

static_assert(N >= 2, "N must be greater or equal to 2");

private:
    struct ValueNode {
        ValueNode* next = nullptr;
        std::size_t size;
        K keys[N + 1];
        V entries[N + 1];
    };

    struct Node {
        std::size_t size;
        K keys[N + 1];

        union {
            Node* children[N + 2];
            ValueNode* values[N + 2];
        };

        public:
        Node() {}

        int toString(std::ostringstream& sb, int from, unsigned depth, unsigned height) const {
            int mine = from++;
            sb << "\tstruct" << mine << " [label=\"";
            for (size_t i = 0; i < 2 * size + 1; i++) {
                if (i > 0) {
                    sb << "|";
                }
                sb << "<f" << i << "> ";
                if ((i & 1) == 1) {
                    sb << keys[i / 2];
                }
            }
            sb << "\"];" << std::endl;

            for (size_t i = 0; i < size + 1; i++) {
                int field = 2 * i;
                sb << "\tstruct" << mine << ":<f" << field << "> -> struct" << from << ";" << std::endl;
                
                if (depth >= height) {
                    sb << "\tstruct" << from++ << " [label=" << value->value << ", shape=ellipse];" << std::endl;                    
                } else {
                    from = children[i]->toString(sb, from, depth + 1, height);
                }
            }
            
            return depth >= height ? from + size + 1 : from;           
        }
    };

#if 0
    class NodePool {
        std::size_t maxSize;
        std::size_t allocated;
        std::size_t idx;
        Node* data;
        
    public:
        NodePool() : maxSize(256), allocated(0), idx(0) {
            data = new Node[maxSize];
        }

        NodePool(const NodePool& copy) = delete;

        NodePool& operator=(const NodePool& copy) = delete;

        Node* allocNode() {
            if (allocated + 1 >= maxSize) {
                Node* newData = new Node[maxSize * 2];
                std::size_t i, j;
                for (; i < maxSize; i++) {
                    if (data[i].size)
                        newData[j] = data[i]
                }

                maxSize *= 2;
            }

            allocated++;
            return &data[idx++];
        }

        Node* allocNode(const K& key) {
            Node* node = allocNode();
            node->size = 1;
            node->keys[0] = key;
            return node;
        }

        void freeNode(Node* node) {
            node->size = 0;
            allocated--;

            if (allocated < maxSize / 2 - 100) {
                // defragment
                long i = -1;
                long j = allocated;

                while (true) {
                    do { // find next free node
                        i++;
                        if (i >= maxSize)
                            break;

                    } while (data[i].size != 0);


                    do { // find next used node
                        j--;
                        if (j >= 0)
                            break;

                    } while (data[j].size == 0);

                    if (i < j) {
                        assert(data[i].size != 0);
                        assert(data[j].size == 0);
                        std::swap(data[i], data[j]);

                    } else {
                        break;
                    }
                }

                idx = j + 1;
            }
        }

        ~NodePool() {
            free(data);
        }

        void reset() {
            allocated = 0;
            idx = 0;
        }
    };
#else
    class NodePool {
        
    public:
        NodePool() {};
        NodePool(const NodePool& copy) = delete;
        NodePool& operator=(const NodePool& copy) = delete;

        ValueNode* allocNode() {
            return new ValueNode();
        }

        ValueNode* allocNode(const V& value) {
            ValueNode* node = allocNode();
            node->next = nullptr;
            node->value = value;
            return node;
        }

        void freeNode(ValueNode* node) {
            delete node;
        }

        ~NodePool() {}


        void reset(auto& tree) {
            // free values
            ValueNode* node = tree->valuesRoot;
            while(node) {
                auto next = node->next;
                delete node;
                node = next;
            }
        }
    };
#endif

    NodePool nodePool;
    Node* root = nullptr;
    ValueNode* valuesRoot = nullptr;
    unsigned height = 0;
    std::size_t elementCount = 0;

    bool findKeyInNode(auto* node, const K& key, std::size_t& idx);

    void splitRoot(Node* node);

    void split(Node* parent, std::size_t idx);

    void insert_aux(unsigned depth, Node* node, const K& key, const V& value, bool found);

    V& find_aux(Node* node, const K& key, unsigned depth) const;

public:
    BPlusTree() : root(nullptr) {};

    std::size_t size() const { return elementCount; }

    void insert(const K&, const V&);

    //void insert (K&&, V&&);

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

static inline void insertNode(auto* node, std::size_t i, auto&& key, auto* child) {    
    for (std::size_t j = node->size; j > i; j--) {
        node->keys[j] = std::move(node->keys[j - 1]);
        node->children[j + 1] = node->children[j];
    }

    node->keys[i] = std::forward<decltype(key)>(key);
    node->children[i + 1] = child;
    node->size++;
}

static inline void insertValueNode(auto* valueNode, std::size_t i, auto&& key, auto&& value) {    
    for (std::size_t j = valueNode->size; j > i; j--) {
        valueNode->keys[j + 1] = std::move(valueNode->keys[j]);
        valueNode->entries[j + 1] = std::move(valueNode->entries[j]);
    }

    valueNode->keys[i] = std::forward<decltype(key)>(key);
    valueNode->entries[i] = std::forward<decltype(value)>(value);
    valueNode->size++;
}

template<typename K, typename V, std::size_t N>
static inline void removeKeyFromNode(auto* node, std::size_t i) {
    assert(node->size <= N);
    
    for (std::size_t j = i; j < node->size - 1; j++) {
        node->keys[j] = std::move(node->keys[j + 1]);
        node->children[j] = node->children[j + 1];
    }

    node->children[node->size - 1] = node->children[node->size];
    node->size--;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::splitRoot(Node* node) {
    constexpr std::size_t splitIndex = N / 2;

    Node* left = new Node();
    Node* right = new Node();
    
    for(std::size_t k = 0; k < splitIndex; k++) {
        left->keys[k] = std::move(node->keys[k]);
        left->children[k] = node->children[k];

        right->keys[k] = std::move(node->keys[k + splitIndex + 1]);
        right->children[k] = node->children[k + splitIndex + 1];
    }
    
    left->children[splitIndex] = node->children[splitIndex];
    right->children[splitIndex] = node->children[2 * splitIndex + 1];

    if constexpr (N % 2 != 0) {
        right->keys[splitIndex] = std::move(node->keys[N]);
        right->children[splitIndex + 1] = node->children[N + 1];

        right->size = splitIndex + 1;
        left->size = splitIndex;

    } else {
        right->size = splitIndex;
        left->size = splitIndex;
    }
    
    node->keys[0] = std::move(node->keys[splitIndex]);
    node->children[0] = left;
    node->children[1] = right;
    node->size = 1;
    height++;
}


template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::split(Node* parent, std::size_t idx) {
    constexpr std::size_t splitIndex = N / 2;
    assert(parent);

    Node* left = parent->children[idx];
    Node* right = new Node();
    
    for(std::size_t k = 0; k < splitIndex; k++) {
        right->keys[k] = std::move(left->keys[k + splitIndex + 1]);
        right->children[k] = left->children[k + splitIndex + 1];
    }
    
    right->children[splitIndex] = left->children[2 * splitIndex + 1];

    if constexpr (N % 2 != 0) {
        right->keys[splitIndex] = std::move(left->keys[N]);
        right->children[splitIndex + 1] = left->children[N + 1];

        right->size = splitIndex + 1;
    } else {
        right->size = splitIndex;
    }
    
    insertNode(parent, idx, std::move(left->keys[splitIndex]), right);
    left->size = splitIndex;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::insert(const K& key, const V& value) {
    if(!root) {
        assert(!valuesRoot);
        root = new Node();
        root->size = 1;
        root->keys[0] = key;

        valuesRoot = nodePool.allocNode();
        valuesRoot->size = 1;
        valuesRoot->keys[0] = key;
        valuesRoot->entries[0] = value;
        
        elementCount = 1;
        height = 1;

    } else {
        elementCount++;
        insert_aux(1, root, key, value, false);
        
        if (root->size > N) {
            splitRoot(root);
        }
    }
}

template<typename K, typename V, std::size_t N>
bool BPlusTree<K, V, N>::findKeyInNode(auto* node, const K& key, std::size_t& idx) {
    for(idx = node->size; idx > 0; idx--) {
        if(key == node->keys[idx - 1]) {
            idx--;
            return true;
        }

        if(key > node->keys[idx - 1]) {
            break;
        }
    }

    return false;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::insert_aux(unsigned depth, Node* node, const K& key, const V& value, bool found) {

    std::size_t idx;
    if (findKeyInNode(node, key, idx)) {
        // key already exists -> go right
        elementCount--;
        insert_aux(depth + 1, node->children[idx + 1], key, value, true);
        if(node->children[idx + 1]->size > N) {
            split(node, idx + 1);
        }

        return;
    }


    if(depth < height) { // if not leaf
        Node* child = node->children[idx];
        insert_aux(depth + 1, child, key, value, found);
        if(child->size > N) {
            split(node, idx);
        }

    } else {
        assert(node->size - 1 <= N);
        ValueNode* child = node->values[idx];

        if (found) {
            std::size_t valueIdx;
            if(findKeyInNode(child, key, valueIdx)) {
                child->entries[valueIdx] = value;

            } else {
                insertValueNode(child, valueIdx, key, value);

                // split value node
                if (child->size > N) {

                    ValueNode *right = nodePool.allocNode();
                    constexpr std::size_t splitIndex = N / 2;

                    for(std::size_t k = 0; k < splitIndex; k++) {
                        right->keys[k] = std::move(child->keys[k + splitIndex + 1]);
                        right->entries[k] = std::move(child->entries[k + splitIndex + 1]);
                    }
    
                    if constexpr (N % 2 != 0) {
                        right->keys[splitIndex] = std::move(node->keys[N]);
                        right->entries[splitIndex] = std::move(node->entries[N]);
                        right->size = splitIndex + 1;
                    } else {
                        right->size = splitIndex;
                    }

                    child->size = splitIndex;
                }
            }
            
        } else {
            insertNode(node, idx, std::move(key), static_cast<Node*>(nullptr));
        }
    }

    assert(node->size <= N + 1);
}


template<typename K, typename V, std::size_t N>
const V& BPlusTree<K, V, N>::at(const K& key) const {
    if (!root)
        return false;

    return find_aux(root, key, 1);
}

template<typename K, typename V, std::size_t N>
V& BPlusTree<K, V, N>::at(const K& key) {
    if (!root)
        throw std::out_of_range("key");

    return find_aux(root, key, 1);
}

template<typename K, typename V, std::size_t N>
V& BPlusTree<K, V, N>::find_aux(Node* node, const K& key, unsigned depth) const {
    bool isInnerNode = depth < height;

    for(int i = node->size - 1; i >= 0; i--) {
        if(key == node->keys[i]) 
            // go right
            return node->values[i];

        if(key > node->keys[i]) {
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
bool BPlusTree<K, V, N>::erase_aux(unsigned depth, Node* node, const K& key) {

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
    
        const K& nextSmallestKey = nextSmallest->keys[nextSmallest->size - 1];
        node->keys[idx] = std::move(nextSmallestKey);
        retval = erase_aux(depth + 1, child, nextSmallestKey);

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
        insertNode(child, 0, std::move(node->keys[idx - 1]), child->children[0]);
        child->children[0] = leftSibling->children[leftSibling->size];
        node->keys[idx - 1] = std::move(leftSibling->keys[leftSibling->size - 1]);

        leftSibling->size--;
        return retval;
    }

    // try steal child from right sibling
    Node* rightSibling = idx < node->size ? node->children[idx + 1] : nullptr;
    if (rightSibling && rightSibling->size > MIN_KEYS) {

        // rotate keys
        insertNode(child, MIN_KEYS - 1, 
            std::move(node->keys[idx]), rightSibling->children[0]);
            
        node->keys[idx] = rightSibling->keys[0];
        node->keys[idx] = std::move(rightSibling->keys[0]);

        removeKeyFromNode<K, V, N>(rightSibling, 0);
        return retval;
    }

    // merge with left
    if (leftSibling) {
        assert(leftSibling->size == MIN_KEYS);

        leftSibling->keys[MIN_KEYS] = std::move(node->keys[idx - 1]);
        leftSibling->children[MIN_KEYS + 1] = child->children[0];

        for (std::size_t i = 0; i < MIN_KEYS - 1; i++) {
            leftSibling->keys[MIN_KEYS + i + 1] = std::move(child->keys[1]);
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

        child->keys[MIN_KEYS - 1] = std::move(node->keys[idx]);
        child->children[MIN_KEYS] = rightSibling->children[0];

        for (std::size_t i = 0; i < MIN_KEYS; i++) {
            child->keys[MIN_KEYS + i] = std::move(rightSibling->keys[i]);
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
bool BPlusTree<K, V, N>::erase(const K& key) {
    if (!root)
        return false;
    
    if (height == 1 && root->size == 1 && root->keys[0] == key) {
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
void BPlusTree<K, V, N>:: erase_all() {
    nodePool.reset();
    elementCount = 0;
    height = 0;
    root = nullptr;
}
