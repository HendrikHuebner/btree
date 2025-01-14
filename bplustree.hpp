#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>


template<typename K, typename V, std::size_t N = 4>
class BPlusTree {

static_assert(N > 3, "N must be greater than 3");

private:
    struct Node {
        std::size_t size;
        union {
            Node* children[N + 2];
            V* values[N + 1];
        };

        Node* next = nullptr;
        Node* prev = nullptr;
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
                    sb << "\tstruct" << from << " [label=\"Value: " << *values[i] << "\"];" << std::endl;
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

/*
    class NodePool {
    public:
        struct PoolSegment {
            ValueNode* nodes;
            size_t size;
            PoolSegment* next;

            PoolSegment(size_t segmentSize) : size(segmentSize), next(nullptr) {
                nodes = new ValueNode[segmentSize];
            }

            ~PoolSegment() {
                delete[] nodes;
            }
        };

        size_t capacity;
        size_t allocated;
        PoolSegment* segments;
        ValueNode* freeList;

        explicit NodePool(size_t initialCapacity) {
            capacity = initialCapacity;
            allocated = 0;
            segments = new PoolSegment(initialCapacity);
            reset();
        }

        ~NodePool() {
            while (segments) {
                PoolSegment* nextSegment = segments->next;
                delete segments;
                segments = nextSegment;
            }
        }

        NodePool(const NodePool&) = delete;
        NodePool& operator=(const NodePool&) = delete;

        NodePool(NodePool&& other) noexcept {
            capacity = other.capacity;
            allocated = other.allocated;
            segments = other.segments;
            freeList = other.freeList;
            other.segments = nullptr;
            other.freeList = nullptr;
            other.capacity = 0;
            other.allocated = 0;
        }

        NodePool& operator=(NodePool&& other) noexcept {
            if (this != &other) {
                while (segments) {
                    PoolSegment* nextSegment = segments->next;
                    delete segments;
                    segments = nextSegment;
                }

                capacity = other.capacity;
                allocated = other.allocated;
                segments = other.segments;
                freeList = other.freeList;

                other.segments = nullptr;
                other.freeList = nullptr;
                other.capacity = 0;
                other.allocated = 0;
            }
            return *this;
        }

        ValueNode* allocate() {
            if (!freeList) {
                expand();
            }

            ValueNode* node = freeList;
            freeList = freeList->next;
            allocated++;
            node->prev = nullptr;
            node->next = nullptr;
            return node;
        }

        template <typename Value>
        ValueNode* allocate(Value&& value) {
            ValueNode* node = allocate();
            node->value = std::forward<Value>(value);
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
            allocated = 0;
            freeList = nullptr;

            for (PoolSegment* segment = segments; segment != nullptr; segment = segment->next) {
                for (size_t i = 0; i < segment->size; i++) {
                    segment->nodes[i].next = freeList;
                    freeList = &segment->nodes[i];
                }
            }
        }

    private:
        void expand() {
            size_t newSegmentSize = capacity * 2;
            PoolSegment* newSegment = new PoolSegment(newSegmentSize);
            for (size_t i = 0; i < newSegmentSize; i++) {
                newSegment->nodes[i].next = freeList;
                freeList = &newSegment->nodes[i];
            }

            capacity += newSegmentSize;
            newSegment->next = segments;
            segments = newSegment;
        }
    };
*/

    class NodePool {
        public:

        NodePool(int) {}
        template <typename Value>
        V* allocate(Value&& value) {
            return new Value(std::forward<Value>(value));
        }

        void deallocate(V* node) {
            delete node;
        }

        void reset() {}
    };


    class BPlusTreeIterator {
    private:
        Node* current;
        std::size_t idx;
        bool forward;

        template<typename Iterator>
        friend Iterator& incrementIterator(Iterator& it, bool forward);

    public:
        explicit BPlusTreeIterator(Node* node, std::size_t idx, bool forward) : current(node), idx(idx), forward(forward) {}

        V& operator*() { return *current->values[idx]; }
        V* operator->() { return current->values[idx]; }

        BPlusTreeIterator& operator++() {
            return incrementIterator<BPlusTreeIterator>(*this, forward);
        }

        BPlusTreeIterator operator++(int) {
            BPlusTreeIterator temp = *this;
            ++(*this);
            return temp;
        }

        BPlusTreeIterator& operator--() {
            return incrementIterator<BPlusTreeIterator>(*this, !forward);
        }

        BPlusTreeIterator operator--(int) {
            BPlusTreeIterator temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const BPlusTreeIterator& other) const { return current == other.current; }
        bool operator!=(const BPlusTreeIterator& other) const { return current != other.current; }
    };

    class ConstBPlusTreeIterator {
    private:
        const Node* current;
        std::size_t idx;
        bool forward;

        template<typename Iterator>
        friend Iterator& incrementIterator(Iterator& it, bool forward);

    public:
        explicit ConstBPlusTreeIterator(const Node* node, std::size_t idx, bool forward) : current(node), idx(idx), forward(forward) {}

        const V& operator*() const { return *current->values[idx]; }
        const V* operator->() const { return current->values[idx]; }

        ConstBPlusTreeIterator& operator++() {
            return incrementIterator<BPlusTreeIterator>(*this, forward);
        }

        ConstBPlusTreeIterator operator++(int) {
            ConstBPlusTreeIterator temp = *this;
            ++(*this);
            return temp;
        }

        ConstBPlusTreeIterator& operator--() {
            return incrementIterator<BPlusTreeIterator>(*this, !forward);
        }

        ConstBPlusTreeIterator operator--(int) {
            ConstBPlusTreeIterator temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const ConstBPlusTreeIterator& other) const { return current == other.current; }
        bool operator!=(const ConstBPlusTreeIterator& other) const { return current != other.current; }
    };

    using iterator = BPlusTreeIterator;
    using const_iterator = ConstBPlusTreeIterator;
    using reverse_iterator = std::reverse_iterator<BPlusTreeIterator>;
    using reverse_const_iterator = std::reverse_iterator<ConstBPlusTreeIterator>;
    
    NodePool nodeAllocator;
    Node* root = nullptr;
    Node* valuesBegin = nullptr;
    Node* valuesEnd = nullptr;
    unsigned height = 0;
    std::size_t elementCount = 0;

    /**
     * Searches keys of node. If key is found return index of right child of the key. 
     * Otherwise return index of the right child of the first key greater than the target. 
     */
    bool findKeyInNode(Node* node, const K& key, std::size_t& idx) const;
    
    void insertValue(Node* node, std::size_t idx, auto&& key, V* value);

    void insertNode(Node* node, std::size_t idx, auto&& key, Node* child);

    void removeKeyFromNode(Node* node, std::size_t i);

    void removeKeyFromValueNode(Node* node, std::size_t i);

    void split(Node* parent, std::size_t idx, bool  childIsLeaf);

    void insert(unsigned depth, Node* node, const K& key, auto&& value);

    iterator find(Node* node, const K& key, unsigned depth);

    const_iterator find(Node* node, const K& key, unsigned depth) const;

    void free_nodes(Node* node, unsigned depth);

public:
    BPlusTree() : nodeAllocator(256), root(nullptr) {};

    BPlusTree(BPlusTree&& other) :
        nodeAllocator(std::move(other.nodeAllocator)),
        root(other.root),
        valuesBegin(other.valuesBegin),
        valuesEnd(other.valuesEnd),
        height(other.height),
        elementCount(other.elementCount) {

        other.root = nullptr;
        other.valuesBegin = nullptr;
        other.valuesEnd = nullptr;
        other.elementCount = 0;
        other.height = 0;
    };

    BPlusTree& operator=(BPlusTree&& other) {
        nodeAllocator = std::move(other.nodeAllocator);
        root = other.root;
        valuesBegin = other.valuesBegin;
        valuesEnd = other.valuesEnd;
        height = other.height;
        elementCount = other.elementCount;

        other.root = nullptr;
        other.valuesBegin = nullptr;
        other.valuesEnd = nullptr;
        other.elementCount = 0;
        other.height = 0;
    };

    BPlusTree(const BPlusTree&) = delete;
    BPlusTree& operator=(const BPlusTree&) = delete;

    ~BPlusTree() {
        free_nodes(root, 1);
        root = nullptr;
        valuesBegin = nullptr;
        valuesEnd = nullptr;
    }

    std::size_t size() const { return elementCount; }

    bool empty() const { return elementCount > 0; }

    template<typename Value>
    void insert(const K&, Value&&);

    void insert(const std::pair<K, V>& entry) { insert(entry.first, entry.second); }

    bool erase(const K&);

    bool erase_aux(unsigned depth, Node* node, const K& key);

    void clear();

    V& at(const K& key);

    const V& at(const K& key) const;

    iterator find(const K& key);

    const_iterator find(const K& key) const;

    bool contains(const K& key) const noexcept;

    iterator begin() noexcept { return iterator(valuesBegin, 0, true); }
    iterator end() noexcept { return iterator(nullptr, 0, true); }
    const_iterator begin() const noexcept { return cbegin(); }
    const_iterator end() const noexcept { return cend(); }
    const_iterator cbegin() const noexcept { return const_iterator(valuesBegin, 0, true); }
    const_iterator cend() const noexcept { return const_iterator(nullptr, 0, true); }

    reverse_iterator rbegin() noexcept { return std::reverse_iterator<iterator>(begin()); }
    reverse_iterator rend() noexcept { return std::reverse_iterator<iterator>(end()); }
    reverse_const_iterator rbegin() const noexcept { return std::reverse_iterator<const_iterator>(cbegin()); }
    reverse_const_iterator rend() const noexcept { return std::reverse_iterator<const_iterator>(cend()); }
    reverse_const_iterator crbegin() const noexcept { return std::reverse_iterator<const_iterator>(cbegin()); }
    reverse_const_iterator crend() const noexcept { return std::reverse_iterator<const_iterator>(cend()); }

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

template<typename Iterator>
Iterator& incrementIterator(Iterator& it, bool forward) {
    if (!it.current) return it;

    if (forward) {
        if (++it.idx >= it.current->size) {
            it.current = it.current->next;
            it.idx = 0;
        }
    } else {
        if (it.idx == 0) {
            it.current = it.current->prev;
            it.idx = it.current ? it.current->size - 1 : 0;
        } else {
            it.idx--;
        }
    }

    return it;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::insertNode(Node* node, std::size_t i, auto&& key, Node* child) {    
    for (std::size_t j = node->size; j > i; j--) {
        node->keys[j] = std::move(node->keys[j - 1]);
        node->children[j + 1] = node->children[j];
    }

    node->keys[i] = std::forward<decltype(key)>(key);
    node->children[i + 1] = child;
    node->size++;
}

template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>::insertValue(Node* node, std::size_t i, auto&& key, V* value) {
    assert(node->size > 0 && "Can't insert into empty node");

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
void BPlusTree<K, V, N>::removeKeyFromValueNode(Node* node, std::size_t i) {
    assert(node->size <= N);

    V* value = node->values[i];

    /* remove from linked list
    if (reorderLinks) {
        if (value->next) {
            if (value->prev) {
                value->prev->next = value->next;
                value->next->prev = value->prev;

            } else {
                assert(value == valuesBegin);

                valuesBegin = value->next;
                valuesBegin->prev = nullptr;
            }
        } else {
            assert(value == valuesEnd);
            
            if (value->prev) {
                valuesEnd = value->prev;
                value->prev->next = nullptr;

            } else {
                valuesBegin = nullptr;
                valuesEnd = nullptr;
            }
        } 
    
        nodeAllocator.deallocate(value);
    } */
 
    // shuffle nodes
    for (std::size_t j = i; j < node->size - 1; j++) {
        node->keys[j] = std::move(node->keys[j + 1]);
        node->values[j] = node->values[j + 1];
    }

    node->size--;
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

        right->prev = left;
        right->next = left->next;
        left->next = right;
        
        if (right->next) {
            right->next->prev = right;
        } else {
            assert(left == valuesEnd);
            valuesEnd = right;
        }
        
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
template<typename Value>
void BPlusTree<K, V, N>::insert(const K& key, Value&& value) {
    if(!root) {
        assert(!valuesBegin);
        root = new Node();
        root->size = 1;
        root->keys[0] = key;
        root->values[0] = new V(std::forward<Value>(value));

        valuesBegin = root;
        valuesEnd = valuesBegin;

        elementCount = 1;
        height = 1;

    } else {
        elementCount++;
        insert(1, root, key, std::forward<Value>(value));
        
        if (root->size > N) {
            Node* newRoot = new Node();
            newRoot->size = 0;
            newRoot->children[0] = root;

            split(newRoot, 0, height <= 1);
            root = newRoot;
            height++;
        }
    }
}


template<typename K, typename V, std::size_t N>
bool BPlusTree<K, V, N>::findKeyInNode(Node* node, const K& key, std::size_t& idx) const {

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
                idx = pivot + 1;
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
void BPlusTree<K, V, N>::insert(unsigned depth, Node* node, const K& key, auto&& value) {
    bool isLeaf = depth >= height;

    std::size_t idx;
    bool foundInCurrentNode = findKeyInNode(node, key, idx);


    if(isLeaf) { // if not leaf

        if (foundInCurrentNode) {
            // replace
            *node->values[idx - 1] = std::move(value);
            elementCount--;

        } else {
            insertValue(node, idx, key, nodeAllocator.allocate(std::move(value)));  
        }

    } else {
        insert(depth + 1, node->children[idx], key, value);
    
        if(node->children[idx]->size > N) {
            split(node, idx, depth + 1 >= height);
        }
    }

    assert(node->size <= N + 1);
}

template<typename K, typename V, std::size_t N>
const V& BPlusTree<K, V, N>::at(const K& key) const {
    const_iterator it = find(key);

    if (it == cend())
        throw std::out_of_range("key not found");

    return *it;
}

template<typename K, typename V, std::size_t N>
V& BPlusTree<K, V, N>::at(const K& key) {
    iterator it = find(key);

    if (it == end())
        throw std::out_of_range("key not found");

    return *it;
}

template<typename K, typename V, std::size_t N>
bool BPlusTree<K, V, N>::contains(const K& key) const noexcept {
    if (!root)
        return false;
    
    return find(root, key, 1) != cend();
}

template<typename K, typename V, std::size_t N>
BPlusTree<K, V, N>::iterator BPlusTree<K, V, N>::find(const K& key) {
    if (!root)
        return end();

    return find(root, key, 1);
}

template<typename K, typename V, std::size_t N>
BPlusTree<K, V, N>::const_iterator BPlusTree<K, V, N>::find(const K& key) const {
    if (!root)
        return cend();

    return find(root, key, 1);
}

template<typename K, typename V, std::size_t N>
BPlusTree<K, V, N>::iterator BPlusTree<K, V, N>::find(Node* node, const K& key, unsigned depth) {
    bool isLeaf = depth >= height;

    std::size_t idx;
    bool found = findKeyInNode(node, key, idx);

    if (isLeaf) {
        if (found) {
            return iterator(node, idx - 1, true);
        } else {
            return end();
        }

    } else {
        return find(node->children[idx], key, depth + 1);
    }
}


template<typename K, typename V, std::size_t N>
BPlusTree<K, V, N>::const_iterator BPlusTree<K, V, N>::find(Node* node, const K& key, unsigned depth) const {
    bool isLeaf = depth >= height;

    std::size_t idx;
    bool found = findKeyInNode(node, key, idx);

    if (isLeaf) {
        if (found) {
            return const_iterator(node, idx - 1, true);
        } else {
            return cend();
        }

    } else {
        return find(node->children[idx], key, depth + 1);
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

        V* valueToErase = nextLargest->values[0];
        const K& nextSmallestKey = nextSmallest->keys[nextSmallest->size - 1];

        // duplicate key of nextSmallest as inner node
        nextLargest->keys[0] = nextSmallestKey;
        node->keys[idx] = nextSmallestKey;

        // swap value nodes
        V* nextSmallestValue = nextSmallest->values[nextSmallest->size - 1];
        nextSmallest->values[nextSmallest->size - 1] = valueToErase;
        nextLargest->values[0] = nextSmallestValue;

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
            insertValue(child, 0, node->keys[idx - 1], leftSibling->values[leftSibling->size - 1]);

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
            removeKeyFromValueNode(rightSibling, 0);
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
            leftSibling->next = child->next;

            if (child->next) {
                child->next->prev = leftSibling;
            } else {
                assert(child == valuesEnd);
                valuesEnd = leftSibling;
            }

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
            child->next = rightSibling->next;

            if (rightSibling->next) {
                rightSibling->next->prev = child;
            } else {
                assert(rightSibling == valuesEnd);
                valuesEnd = child;
            }

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
void BPlusTree<K, V, N>::free_nodes(Node* node, unsigned depth) {
    if (depth < height) {
        for (std::size_t i = 0; i <= node->size; i++) {
            Node* child = node->children[i];
            free_nodes(child, depth + 1);
        }
    }

    delete node;
}


template<typename K, typename V, std::size_t N>
void BPlusTree<K, V, N>:: clear() {
    free_nodes(root, 1);
    nodeAllocator.reset();
    elementCount = 0;
    height = 0;
    root = nullptr;
}
