#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>


template<typename K, typename V, std::size_t N = 4>
class BPlusTree {

static_assert(N > 2, "N must be greater than 2");

private:

    struct ValueNode {
        ValueNode* next = nullptr;
        ValueNode* prev = nullptr;
        V value;
    };

    struct Node {
        std::size_t size;
        union {
            Node* children[N + 2];
            ValueNode* values[N + 1];
        };

        K keys[N + 1];

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

            for (size_t i = 0; i < size; i++) {
                int field = 2 * i;
                
                sb << "\tstruct" << mine << ":<f" << field << "> -> struct" << from << ";" << std::endl;

                if (depth >= height) {
                    sb << "\tstruct" << from << " [label=\"Value: " << values[i]->value << "\"];" << std::endl;
                    from++;

                } else {
                    from = children[i]->toString(sb, from, depth + 1, height);
                }
            }

            if (depth < height) {
                sb << "\tstruct" << mine << ":<f" << 2 * size << "> -> struct" << from << ";" << std::endl;
                from = children[size]->toString(sb, from, depth + 1, height);
            }
            
            return from;           
        }
    };

    class NodePool {
    public:
        size_t capacity;
        size_t allocated;
        ValueNode* pool;
        ValueNode* freeList;

        explicit NodePool(size_t initialCapacity) {
            capacity = initialCapacity;
            pool = nullptr;
            freeList = nullptr;
            allocated = 0;
            reset();
        }

        ~NodePool() {
            delete[] pool;
        }

        NodePool(const NodePool&) = delete;
        NodePool& operator=(const NodePool&) = delete;

        NodePool(NodePool&& other) noexcept {
            capacity = other.capacity;
            pool = other.pool;
            freeList = other.freeList;
            allocated = other.allocated;
        }

        NodePool& operator=(NodePool&& other) noexcept {
            if (this != &other) {
                delete[] pool;
                capacity = other.capacity;
                pool = other.pool;
                freeList = other.freeList;
                allocated = other.allocated;
            }
            return *this;
        }

        ValueNode* allocate() {
            if (!freeList) {
                size_t newCapacity = capacity * 2;
                ValueNode* newPool = new ValueNode[newCapacity];

                for (size_t i = 0; i < newCapacity; i++) {
                    newPool[i].next = freeList;
                    freeList = &newPool[i];
                }

                for (size_t i = 0; i < capacity; i++) {
                    newPool[i] = pool[i];
                }
                delete[] pool;
                pool = newPool;
                capacity = newCapacity;
            }

            ValueNode* node = freeList;
            node->prev = nullptr;
            node->next = nullptr;

            freeList = freeList->next;
            allocated++;
            return node;
        }

        ValueNode* allocate(auto&& value) {
            ValueNode* node = allocate();
            node->value = std::forward<decltype(value)>(value);
            return node;
        }

        void deallocate(ValueNode* node) {
            if (!node) {
                return;
            }
            node->next = freeList;
            freeList = node;
            allocated--;
        }

        void reset() {
            freeList = nullptr;
            for (size_t i = 0; i < capacity; i++) {
                pool[i].next = freeList;
                freeList = &pool[i];
            }
            allocated = 0;
        }
    };

    class Iterator {
    private:
        ValueNode* current;
    public:
        explicit Iterator(ValueNode* node) : current(node) {}

        V& operator*() { return current->value; }
        V* operator->() { return &current->value; }

        Iterator& operator++() {
            if (current) current = current->next;
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            (*this)++;
            return temp;
        }

        Iterator& operator--() {
            if (current) current = current->prev;
            return *this;
        }

        Iterator operator--(int) {
            Iterator temp = *this;
            (*this)--;
            return temp;
        }

        bool operator==(const Iterator& other) const { return current == other.current; }
        bool operator!=(const Iterator& other) const { return current != other.current; }
    };

    NodePool nodePool;
    Node* root = nullptr;
    ValueNode* valuesBegin = nullptr;
    ValueNode* valuesEnd = nullptr;
    unsigned height = 0;
    std::size_t elementCount = 0;

    bool findKeyInNode(Node* node, const K& key, std::size_t& idx) const;
    
    void insertValue(Node* node, std::size_t idx, auto&& key, auto* value, bool reorderLinks = true);

    void insertNode(Node* node, std::size_t idx, auto&& key, auto* child);

    void removeKeyFromNode(Node* node, std::size_t i);

    void removeKeyFromValueNode(Node* node, std::size_t i, bool reorderLinks = true);

    void splitRoot(Node* node, bool isLeaf);

    void split(Node* parent, std::size_t idx, bool  childIsLeaf);

    void insert_aux(unsigned depth, Node* node, const K& key, const V& value);
    
    V& find_aux(Node* node, const K& key, unsigned depth) const;

public:
    BPlusTree() :  nodePool(256), root(nullptr) {};

    std::size_t size() const { return elementCount; }

    void insert(const K&, const V&);

    //void insert (K&&, V&&);

    bool erase(const K&);

    bool erase_aux(unsigned depth, Node* node, const K& key);

    void erase_all();

    V& at(const K& key);

    const V& at(const K& key) const;

    Iterator begin() { return Iterator(valuesBegin); }

    Iterator end() { return Iterator(nullptr); }

    Iterator rbegin() { return Iterator(valuesEnd); }

    Iterator rend() { return Iterator(nullptr); }

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

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::insertNode(Node* node, std::size_t i, auto&& key, auto* child) {    
    for (std::size_t j = node->size; j > i; j--) {
        node->keys[j] = std::move(node->keys[j - 1]);
        node->children[j + 1] = node->children[j];
    }

    node->keys[i] = std::forward<decltype(key)>(key);
    node->children[i + 1] = child;
    node->size++;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::insertValue(Node* node, std::size_t i, auto&& key, auto* value, bool reorderLinks) {

    assert(node->size > 0 && "Can't insert into empty node");

    // insert into linked list
    if (reorderLinks) {
        if (i >= node->size) {
            value->next = node->values[i - 1]->next;
            value->prev = node->values[i - 1];
            node->values[i - 1]->next = value;

            if (value->next) {
                value->next->prev = value;
            
            } else {
                valuesEnd = value;
            }
        } else {

            value->next = node->values[i];
            value->prev = node->values[i]->prev;
            node->values[i]->prev = value;
        
            if (value->prev) {
                value->prev->next = value;
            } else {
                valuesBegin = value;
            }
        }
    }

    // shuffle node pointers  
    for (std::size_t j = node->size; j > i; j--) {
        node->keys[j] = std::move(node->keys[j - 1]);
        node->values[j] = node->values[j - 1];
    }

    node->keys[i] = std::forward<decltype(key)>(key);
    node->values[i] = value;
    node->size++;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::removeKeyFromNode(Node* node, std::size_t i) {
    assert(node->size <= N);
    
    for (std::size_t j = i; j < node->size - 1; j++) {
        node->keys[j] = std::move(node->keys[j + 1]);
        node->children[j] = node->children[j + 1];
    }

    node->children[node->size - 1] = node->children[node->size];
    node->size--;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::removeKeyFromValueNode(Node* node, std::size_t i, bool reorderLinks) {
    assert(node->size <= N);

    ValueNode* value = node->values[i];

    // remove from linked list
    if (reorderLinks) {

        if (value->next) {
            if (value->prev) {
                value->prev->next = value->next;
                value->next->prev = value->prev;

            } else {
                //assert(value == valuesBegin);

                valuesBegin = value->next;
                valuesBegin->prev = nullptr;
            }
        } else {
            //assert(value == valuesEnd);
            
            if (value->prev) {
                valuesEnd = value->prev;
                value->prev->next = nullptr;

            } else {
                valuesBegin = nullptr;
                valuesEnd = nullptr;
            }
        }
    
        nodePool.deallocate(std::move(value));
    }
 
    // shuffle nodes
    for (std::size_t j = i; j < node->size - 1; j++) {
        node->keys[j] = std::move(node->keys[j + 1]);
        node->values[j] = node->values[j + 1];
    }

    node->size--;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::splitRoot(Node* node, bool isLeaf) {
    Node* left = new Node();
    Node* right = new Node();

    if (isLeaf) {
        // duplicate key at splitIndex
        constexpr std::size_t splitIndex = (N + 1) / 2;

        right->size = splitIndex;
        left->size = splitIndex;

        for(std::size_t k = 0; k < splitIndex; k++) {
            left->keys[k] = std::move(node->keys[k]);
            left->values[k] = std::move(node->values[k]);
            right->keys[k] = std::move(node->keys[k + splitIndex]);
            right->values[k] = std::move(node->values[k + splitIndex]);
        }

        if constexpr (N % 2 == 0) {
            right->keys[splitIndex] = std::move(node->keys[N]);
            right->values[splitIndex] = std::move(node->values[N]);
            right->size = splitIndex + 1;
        }

        node->keys[0] = node->keys[splitIndex];

    } else {
        constexpr std::size_t splitIndex = N / 2;

        right->size = splitIndex;
        left->size = splitIndex;

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
        }

        node->keys[0] = std::move(node->keys[splitIndex]);
    }
    
    node->children[0] = left;
    node->children[1] = right;
    node->size = 1;
    height++;
}


template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::split(Node* parent, std::size_t idx, bool childIsLeaf) {
    assert(parent);

    Node* left = parent->children[idx];
    Node* right = new Node();
    
    if (childIsLeaf) {
        constexpr std::size_t splitIndex = (N + 1) / 2;

        for(std::size_t k = 0; k < splitIndex; k++) {
            right->keys[k] = std::move(left->keys[k + splitIndex]);
            right->values[k] = left->values[k + splitIndex];
        }
        
        if constexpr (N % 2 == 0) {
            right->keys[splitIndex] = std::move(left->keys[N]);
            right->values[splitIndex] = left->values[N];
            right->size = splitIndex + 1;
        } else {
            right->size = splitIndex;
        }

        insertNode(parent, idx, std::move(left->keys[splitIndex]), right);
        left->size = splitIndex;

    } else {
        // child is inner node
        constexpr std::size_t splitIndex = N / 2;

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
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::insert(const K& key, const V& value) {
    if(!root) {
        assert(!valuesBegin);
        root = new Node();
        root->size = 1;
        root->keys[0] = key;

        valuesBegin = nodePool.allocate(value);
        valuesEnd = valuesBegin;
        root->values[0] = valuesBegin;

        elementCount = 1;
        height = 1;

    } else {
        elementCount++;
        insert_aux(1, root, key, value);
        
        if (root->size > N) {
            splitRoot(root, height <= 1);
        }
    }
}

/*
 * Searches keys of node. If key is found return index of right child of the key. 
 * Otherwise return index of the right child of the first key greater than the target. 
 */
template<typename K, typename V, std::size_t N>
bool BPlusTree<K, V, N>::findKeyInNode(Node* node, const K& key, std::size_t& idx) const {
    for(idx = node->size; idx > 0; idx--) {
        if(key == node->keys[idx - 1]) {
            return true;
        }

        if(key > node->keys[idx - 1]) {
            break;
        }
    }

    return false;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::insert_aux(unsigned depth, Node* node, const K& key, const V& value) {
    bool isLeaf = depth >= height;

    std::size_t idx;
    bool foundInCurrentNode = findKeyInNode(node, key, idx);


    if(isLeaf) { // if not leaf

        if (foundInCurrentNode) {
            // replace
            node->values[idx - 1]->value = std::move(value);
            elementCount--;

        } else {
            insertValue(node, idx, std::move(key), nodePool.allocate(std::move(value)));  
        }

    } else {
        insert_aux(depth + 1, node->children[idx], key, value);
    
        if(node->children[idx]->size > N) {
            split(node, idx, depth + 1 >= height);
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
        throw std::out_of_range("key not found");

    return find_aux(root, key, 1);
}

template<typename K, typename V, std::size_t N>
V& BPlusTree<K, V, N>::find_aux(Node* node, const K& key, unsigned depth) const {
    bool isLeaf = depth >= height;

    std::size_t idx;
    bool found = findKeyInNode(node, key, idx);

    if (isLeaf) {
        if (found) {
            return node->values[idx - 1]->value;

        } else {
            throw std::out_of_range("key not found");
        }

    } else {
        return find_aux(node->children[idx], key, depth + 1);
    }
}

template<typename K, typename V, std::size_t N>
bool BPlusTree<K, V, N>::erase(const K& key) {
    if (!root)
        return false;
    
    if (height == 1 && root->size == 1 && root->keys[0] == key) {
        assert(valuesBegin->next == nullptr);
        removeKeyFromValueNode(root, 0);

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
bool BPlusTree<K, V, N>::erase_aux(unsigned depth, Node* node, const K& key) {

    // find key in current node
    std::size_t idx;
    bool isLeaf = depth >= height;
    bool childIsLeaf = depth + 1 >= height;
    bool found = findKeyInNode(node, key, idx);

    if (key == 96) {
        printf("asfhsrghrsghj");
    }
    
    if (isLeaf) {
        if (found) {
            // remove from leaf
            removeKeyFromValueNode(node, idx - 1);
            return true;

        } else {
            // nothing to do
            return false;
    }
        }

    bool retval;
    Node* child;

    if (found) {
        // find next smallest and largest
        // swap smaller with current key and replace larger
        idx--;
        child = node->children[idx];

        unsigned currentDepth = depth + 1;
        Node* nextSmallest = child;
        Node* nextLargest = node->children[idx + 1];

        while (currentDepth < height) {
            nextSmallest = nextSmallest->children[nextSmallest->size];
            nextLargest = nextLargest->children[0];
            currentDepth++;
        }

        assert(nextLargest->keys[0] == key && "Inner node must have duplicate key as direct successor");

        if constexpr (N == 3) {
            // rare special case
            if (childIsLeaf && nextSmallest->size == 1 && idx > 0) {
                removeKeyFromValueNode(nextLargest, 0);
                
                for (std::size_t i = idx; i < node->size - 1; i++) {
                    node->keys[i] = node->keys[i + 1];
                    node->children[i + 1] = node->children[i + 2];
                }

                node->size--;
                for (std::size_t i = 0; i < nextLargest->size; i++) {
                    node->children[idx]->keys[1 + i] = nextLargest->keys[i];
                    node->children[idx]->values[1 + i] = nextLargest->values[i];
                    node->children[idx]->size++;
                }

                return true;
            }
        }

        ValueNode* valueToErase = nextLargest->values[0];
        const K& nextSmallestKey = nextSmallest->keys[nextSmallest->size - 1];

        // duplicate key of nextSmallest as inner node
        nextLargest->keys[0] = nextSmallestKey;
        node->keys[idx] = nextSmallestKey;

        // swap value nodes
        ValueNode* nextSmallestValue = nextSmallest->values[nextSmallest->size - 1];
        nextSmallest->values[nextSmallest->size - 1] = valueToErase;
        nextLargest->values[0] = nextSmallestValue;

        // swap links
        nextSmallestValue->next = valueToErase->next;
        valueToErase->next = nextSmallestValue;
        valueToErase->prev = nextSmallestValue->prev;
        nextSmallestValue->prev = valueToErase;

        if (nextSmallestValue->next) {
            nextSmallestValue->next->prev = nextSmallestValue;
        }

        if (nextSmallestValue == valuesBegin) {
            valuesBegin = valueToErase;
        }
        
        if (valueToErase == valuesEnd) {
            valuesEnd = nextSmallestValue;
        }

        if (valueToErase->prev) {
            valueToErase->prev->next = valueToErase;
        }

        erase_aux(depth + 1, child, nextSmallestKey);
        retval = true;

    } else {
        child = node->children[idx];
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

        if (childIsLeaf) {
            // rotate keys
            node->keys[idx - 1] = leftSibling->keys[leftSibling->size - 1];
            insertValue(child, 0, node->keys[idx - 1], leftSibling->values[leftSibling->size - 1], false);

        } else {
            // rotate keys
            insertNode(child, 0, std::move(node->keys[idx - 1]), child->children[0]);
            child->children[0] = leftSibling->children[leftSibling->size];
            node->keys[idx - 1] = std::move(leftSibling->keys[leftSibling->size - 1]);
        }

        leftSibling->size--;
        return retval;
    }

    // try steal child from right sibling
    Node* rightSibling = idx < node->size ? node->children[idx + 1] : nullptr;
    if (rightSibling && rightSibling->size > MIN_KEYS) {

        if (childIsLeaf) {
            // rotate keys
            assert(rightSibling->keys[0] == node->keys[idx]);

            child->keys[MIN_KEYS - 1] = rightSibling->keys[0];
            child->values[MIN_KEYS - 1] = rightSibling->values[0];
            child->size++;
            removeKeyFromValueNode(rightSibling, 0, false);
            node->keys[idx] = rightSibling->keys[0];

        } else {
            // rotate keys
            insertNode(child, MIN_KEYS - 1, 
                std::move(node->keys[idx]), rightSibling->children[0]);
                
            node->keys[idx] = rightSibling->keys[0];
            removeKeyFromNode(rightSibling, 0);
        }

        return retval;
    }

    // merge with left
    if (leftSibling) {
        assert(leftSibling->size == MIN_KEYS);

        if (childIsLeaf) {
            for (std::size_t i = 0; i < MIN_KEYS - 1; i++) {
                leftSibling->keys[MIN_KEYS + i] = std::move(child->keys[i]);
                leftSibling->values[MIN_KEYS + i] = child->values[i];
            }

            leftSibling->size = 2 * MIN_KEYS - 1;

        } else {
            leftSibling->keys[MIN_KEYS] = std::move(node->keys[idx - 1]);
            leftSibling->children[MIN_KEYS + 1] = child->children[0];

            for (std::size_t i = 0; i < MIN_KEYS - 1; i++) {
                leftSibling->keys[MIN_KEYS + i + 1] = std::move(child->keys[i]);
                leftSibling->children[MIN_KEYS + i + 2] = child->children[i + 1];
            }

            leftSibling->size = 2 * MIN_KEYS;
        }

        assert(!isLeaf);
        delete child;

        removeKeyFromNode(node, idx - 1); // remove child
        node->children[idx - 1] = leftSibling;

    } else {
        // merge right
        assert(rightSibling);
        assert(rightSibling->size == MIN_KEYS);

        if (childIsLeaf) {
            for (std::size_t i = 0; i < MIN_KEYS; i++) {
                child->keys[MIN_KEYS + i - 1] = std::move(rightSibling->keys[i]);
                child->values[MIN_KEYS + i - 1] = rightSibling->values[i];
            }

            child->size = 2 * MIN_KEYS - 1;
            
        } else {
            child->keys[MIN_KEYS - 1] = std::move(node->keys[idx]);
            child->children[MIN_KEYS] = rightSibling->children[0];

            for (std::size_t i = 0; i < MIN_KEYS; i++) {
                child->keys[MIN_KEYS + i] = std::move(rightSibling->keys[i]);
                child->children[MIN_KEYS + i + 1] = rightSibling->children[i + 1];
            }
            
            child->size = 2 * MIN_KEYS;
        }

        assert(!isLeaf);
        delete rightSibling;

        // remove right sibling by shifting nodes
        removeKeyFromNode(node, idx);
        node->children[0] = child;

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