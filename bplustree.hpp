#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>

template <typename T> class SegmentedFreelistAllocator {
public:
  using value_type = T;

private:
  struct FreeNode {
    FreeNode *next;
  };

  using node_type = union {
    FreeNode node;
    T data;
  };

  struct Segment {
    node_type *data;
    std::size_t size;
    Segment *next;

    Segment(std::size_t segmentSize) : size(segmentSize), next(nullptr) {
      data = static_cast<node_type *>(
          ::operator new(segmentSize * sizeof(node_type)));
    }

    ~Segment() { ::operator delete(data); }
  };

  std::size_t capacity;
  std::size_t allocated;
  Segment *segments;
  FreeNode *freeList;

  void expand() {
    std::size_t newSegmentSize = capacity * 1.5;
    auto *newSegment = new Segment(newSegmentSize);

    newSegment->next = segments;
    segments = newSegment;

    for (std::size_t i = 0; i < newSegmentSize; i++) {
      FreeNode *node = &newSegment->data[i].node;
      node->next = freeList;
      freeList = node;
    }

    capacity = newSegmentSize;
  }

public:
  explicit SegmentedFreelistAllocator(std::size_t initialCapacity = 256)
      : capacity(initialCapacity), allocated(0), segments(nullptr),
        freeList(nullptr) {
    assert(initialCapacity > 0);
    segments = new Segment(initialCapacity);
    reset();
  }

  ~SegmentedFreelistAllocator() {
    while (segments) {
      Segment *next = segments->next;
      delete segments;
      segments = next;
    }
  }

  void reset() {
    freeList = nullptr;
    for (std::size_t i = 0; i < segments->size; i++) {
      FreeNode *node = &segments->data[i].node;
      node->next = freeList;
      freeList = node;
    }
  }

  [[nodiscard]] value_type *allocate(std::size_t n) {
    if (n != 1)
      throw std::bad_alloc();

    if (!freeList) {
      expand();
    }

    FreeNode *node = freeList;
    freeList = freeList->next;
    allocated++;

    return reinterpret_cast<value_type *>(node);
  }

  void deallocate(value_type *ptr) {
    auto *node = reinterpret_cast<FreeNode *>(ptr);
    node->next = freeList;
    freeList = node;

    allocated--;
  }

  // for std::allocator compliance
  void deallocate(value_type *ptr, std::size_t n) {
      if (n != 1)
        return;
      else 
        deallocate(ptr);
  }

  bool operator==(const SegmentedFreelistAllocator &other) const noexcept {
    return this == &other;
  }
};

template <typename Key, typename Value, std::size_t N,
          typename ValueAllocator = SegmentedFreelistAllocator<Value>>
class BPlusTree {

public:
  using key_type = Key;
  using value_type = Value;

  static_assert(N > 3, "N must be greater than 3");

private:
  struct Node {
    std::size_t size = 0;
    union {
      Node *children[N + 2];
      value_type *values[N + 1];
    };

    Node *next = nullptr;
    Node *prev = nullptr;
    key_type keys[N + 1];

  public:
    Node() {}

    Node(Node *child) { children[0] = child; }
  };

public:
  class BPlusTreeIterator {

  private:
    Node *current;
    std::size_t idx;
    bool forward;

    template <typename Iterator>
    friend Iterator &incrementIterator(Iterator &it, bool forward);

  public:
    using value_type = Value;

    BPlusTreeIterator() : current(nullptr), idx(0), forward(false) {}
    explicit BPlusTreeIterator(Node *node, std::size_t idx, bool forward)
        : current(node), idx(idx), forward(forward) {}

    value_type &operator*() const { return *current->values[idx]; }
    value_type *operator->() { return current->values[idx]; }

    BPlusTreeIterator &operator++() {
      return incrementIterator<BPlusTreeIterator>(*this, forward);
    }

    BPlusTreeIterator operator++(int) {
      BPlusTreeIterator temp = *this;
      ++(*this);
      return temp;
    }

    BPlusTreeIterator &operator--() {
      return incrementIterator<BPlusTreeIterator>(*this, !forward);
    }

    BPlusTreeIterator operator--(int) {
      BPlusTreeIterator temp = *this;
      --(*this);
      return temp;
    }

    bool operator==(const BPlusTreeIterator &other) const {
      return current == other.current;
    }
    bool operator!=(const BPlusTreeIterator &other) const {
      return current != other.current;
    }
  };

  class ConstBPlusTreeIterator {
  private:
    const Node *current;
    std::size_t idx;
    bool forward;

    template <typename Iterator>
    friend Iterator &incrementIterator(Iterator &it, bool forward);

  public:
    using value_type = Value;

    ConstBPlusTreeIterator() : current(nullptr), idx(0), forward(false) {}
    explicit ConstBPlusTreeIterator(const Node *node, std::size_t idx,
                                    bool forward)
        : current(node), idx(idx), forward(forward) {}

    const value_type &operator*() const { return *current->values[idx]; }
    const value_type *operator->() const { return current->values[idx]; }

    ConstBPlusTreeIterator &operator++() {
      return incrementIterator<BPlusTreeIterator>(*this, forward);
    }

    ConstBPlusTreeIterator operator++(int) {
      ConstBPlusTreeIterator temp = *this;
      ++(*this);
      return temp;
    }

    ConstBPlusTreeIterator &operator--() {
      return incrementIterator<BPlusTreeIterator>(*this, !forward);
    }

    ConstBPlusTreeIterator operator--(int) {
      ConstBPlusTreeIterator temp = *this;
      --(*this);
      return temp;
    }

    bool operator==(const ConstBPlusTreeIterator &other) const {
      return current == other.current;
    }
    bool operator!=(const ConstBPlusTreeIterator &other) const {
      return current != other.current;
    }
  };

  // static_assert(std::bidirectional_iterator<BPlusTreeIterator>);
  // static_assert(std::bidirectional_iterator<ConstBPlusTreeIterator>);

  using iterator = BPlusTreeIterator;
  using const_iterator = ConstBPlusTreeIterator;
  using reverse_iterator =
      BPlusTreeIterator; // std::reverse_iterator<BPlusTreeIterator>;
  using reverse_const_iterator =
      ConstBPlusTreeIterator; // std::reverse_iterator<ConstBPlusTreeIterator>;

private:
  ValueAllocator valueAllocator;
  SegmentedFreelistAllocator<Node> nodeAllocator;
  Node *root = nullptr;
  Node *minNode = nullptr;
  Node *maxNode = nullptr;
  unsigned height = 0;
  std::size_t keyCount = 0;

  bool findKeyInNode(Node *, const key_type &, std::size_t &) const;

  template<typename KeyFwd>
  void insertLeaf(Node *, std::size_t, KeyFwd &&, value_type *);

  void removeKeyFromLeaf(Node *, std::size_t);

  template<typename KeyFwd>
  void insertInner(Node *, std::size_t, KeyFwd &&, Node *);

  void removeInnerKey(Node *, std::size_t);

  void split(Node *, std::size_t, bool);

  template <typename... Args>
  void insert(unsigned, Node *, const key_type &, Args &&...);

  template <typename Iterator>
  Iterator find(Node *, const key_type &, unsigned) const;

  void freeValues(Node *);

public:
  BPlusTree() : nodeAllocator(64), root(nullptr){};

  BPlusTree(BPlusTree &&other)
      : nodeAllocator(std::move(other.nodeAllocator)), root(other.root),
        minNode(other.minNode), maxNode(other.maxNode), height(other.height),
        keyCount(other.keyCount) {

    other.root = nullptr;
    other.minNode = nullptr;
    other.maxNode = nullptr;
    other.keyCount = 0;
    other.height = 0;
  };

  BPlusTree &operator=(BPlusTree &&other) {
    nodeAllocator = std::move(other.nodeAllocator);
    root = other.root;
    minNode = other.minNode;
    maxNode = other.maxNode;
    height = other.height;
    keyCount = other.keyCount;

    other.root = nullptr;
    other.minNode = nullptr;
    other.maxNode = nullptr;
    other.keyCount = 0;
    other.height = 0;
  };

  BPlusTree(const BPlusTree &) = delete;
  BPlusTree &operator=(const BPlusTree &) = delete;

  ~BPlusTree() {
    freeValues(minNode);
    root = nullptr;
    minNode = nullptr;
    maxNode = nullptr;
  }

  std::size_t size() const { return keyCount; }

  bool empty() const { return keyCount > 0; }

  template <typename... Args> 
  void emplace(const key_type &, Args &&...);

  template <typename ValueFwd> 
  void insert(const key_type &, ValueFwd &&);

  void insert(const std::pair<key_type, value_type> &entry) {
    insert(entry.first, entry.second);
  }

  bool erase(const key_type &);

  bool erase(unsigned, Node *, const key_type &);

  void clear();

  value_type &at(const key_type &);

  const value_type &at(const key_type &) const;

  iterator find(const key_type &);

  const_iterator find(const key_type &) const;

  bool contains(const key_type &) const noexcept;

  // =======  Iterators =======

  iterator begin() noexcept { return iterator(minNode, 0, true); }

  iterator end() noexcept { return iterator(nullptr, 0, true); }

  const_iterator begin() const noexcept { return cbegin(); }

  const_iterator end() const noexcept { return cend(); }

  const_iterator cbegin() const noexcept {
    return const_iterator(minNode, 0, true);
  }

  const_iterator cend() const noexcept {
    return const_iterator(nullptr, 0, true);
  }

  reverse_iterator rbegin() noexcept {
    return iterator(maxNode, maxNode->size - 1, false);
  }

  reverse_iterator rend() noexcept { 
    return iterator(nullptr, 0, false); 
  }
  
  reverse_const_iterator rbegin() const noexcept { return crbegin(); }

  reverse_const_iterator rend() const noexcept { return crend(); }

  reverse_const_iterator crbegin() const noexcept {
    return const_iterator(maxNode, maxNode->size - 1, false);
  }

  reverse_const_iterator crend() const noexcept {
    return const_iterator(nullptr, 0, false);
  }
};

// ======= IMPLEMENTATION =======

template <typename Iterator>
Iterator &incrementIterator(Iterator &it, bool forward) {
  if (!it.current)
    return it;

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

template <typename K, typename V, std::size_t N, typename Alloc>
template<typename KeyFwd>
void BPlusTree<K, V, N, Alloc>::insertInner(Node *node, std::size_t i,
                                            KeyFwd &&key, Node *child) {
  for (std::size_t j = node->size; j > i; j--) {
    node->keys[j] = std::move(node->keys[j - 1]);
    node->children[j + 1] = node->children[j];
  }

  node->keys[i] = std::forward<KeyFwd>(key);
  node->children[i + 1] = child;
  node->size++;
}

template <typename K, typename V, std::size_t N, typename Alloc>
template<typename KeyFwd>
void BPlusTree<K, V, N, Alloc>::insertLeaf(Node *node, std::size_t i,
                                           KeyFwd &&key, V *value) {
  for (std::size_t j = node->size; j > i; j--) {
    node->keys[j] = std::move(node->keys[j - 1]);
    node->values[j] = node->values[j - 1];
  }

  node->keys[i] = std::forward<KeyFwd>(key);
  node->values[i] = value;
  node->size++;
}

template <typename K, typename V, std::size_t N, typename Alloc>
void BPlusTree<K, V, N, Alloc>::removeInnerKey(Node *node, std::size_t i) {
  for (std::size_t j = i; j < node->size - 1; j++) {
    node->keys[j] = std::move(node->keys[j + 1]);
    node->children[j] = node->children[j + 1];
  }

  node->children[node->size - 1] = node->children[node->size];
  node->size--;
}

template <typename K, typename V, std::size_t N, typename Alloc>
void BPlusTree<K, V, N, Alloc>::removeKeyFromLeaf(Node *node, std::size_t i) {
  for (std::size_t j = i; j < node->size - 1; j++) {
    node->keys[j] = std::move(node->keys[j + 1]);
    node->values[j] = node->values[j + 1];
  }

  node->size--;
}

template <typename K, typename V, std::size_t N, typename Alloc>
void BPlusTree<K, V, N, Alloc>::split(Node *parent, std::size_t idx,
                                      bool childIsLeaf) {
  Node *left = parent->children[idx];
  Node *right = nodeAllocator.allocate(1);
  std::construct_at(right);

  if (childIsLeaf) {
    constexpr std::size_t splitIndex = (N + 1) / 2;
    insertInner(parent, idx, left->keys[splitIndex], right);

    for (std::size_t k = 0; k < splitIndex; k++) {
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

    left->size = splitIndex;

    right->prev = left;
    right->next = left->next;
    left->next = right;

    if (right->next) {
      right->next->prev = right;
    } else {
      assert(left == maxNode);
      maxNode = right;
    }

  } else {
    // child is inner node
    constexpr std::size_t splitIndex = N / 2;
    insertInner(parent, idx, left->keys[splitIndex], right);

    for (std::size_t k = 0; k < splitIndex; k++) {
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

    left->size = splitIndex;
  }
}

template <typename K, typename V, std::size_t N, typename Alloc>
template <typename... Args>
void BPlusTree<K, V, N, Alloc>::emplace(const K &key, Args &&...args) {
  if (!root) {
    assert(!minNode);

    V *value = valueAllocator.allocate(1);
    std::construct_at(value, std::forward<Args>(args)...);

    root = nodeAllocator.allocate(1);
    std::construct_at(root);

    root->size = 1;
    root->keys[0] = key;
    root->values[0] = value;
    minNode = maxNode = root;

    keyCount = 1;
    height = 1;

  } else {

    insert<Args...>(1, root, key, std::forward<Args>(args)...);
    keyCount++;

    if (root->size > N) {
      Node *newRoot = nodeAllocator.allocate(1);
      std::construct_at(newRoot, root);

      split(newRoot, 0, height <= 1);
      root = newRoot;
      height++;
    }
  }
}

template <typename K, typename V, std::size_t N, typename Alloc>
template <typename ValueFwd>
void BPlusTree<K, V, N, Alloc>::insert(const K &key, ValueFwd &&value) {
  emplace(key, std::forward<ValueFwd>(value));
}

/**
 * Searches keys of node. If key is found return index of right child of the
 * key. Otherwise return index of the right child of the first key greater than
 * the target.
 */
template <typename K, typename V, std::size_t N, typename Alloc>
bool BPlusTree<K, V, N, Alloc>::findKeyInNode(Node *node, const K &key,
                                              std::size_t &idx) const {
  assert(node->size <= N);

  if constexpr (N < 200) {
    // linear search
    for (idx = node->size; idx > 0; idx--) {
      if (key == node->keys[idx - 1])
        return true;

      if (key > node->keys[idx - 1])
        break;
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

template <typename K, typename V, std::size_t N, typename Alloc>
template <typename... Args>
void BPlusTree<K, V, N, Alloc>::insert(unsigned depth, Node *node, const K &key,
                                       Args &&...args) {
  bool isLeaf = depth >= height;

  std::size_t idx;
  bool foundInCurrentNode = findKeyInNode(node, key, idx);

  if (isLeaf) { // if not leaf
    if (foundInCurrentNode) {
      // replace
      *node->values[idx - 1] = V(std::forward<Args>(args)...);
      keyCount--;

    } else {
      V *value = valueAllocator.allocate(1);
      std::construct_at(value, std::forward<Args>(args)...);
      insertLeaf(node, idx, key, value);
    }

  } else {
    insert<Args...>(depth + 1, node->children[idx], key,
                    std::forward<Args>(args)...);

    if (node->children[idx]->size > N) {
      split(node, idx, depth + 1 >= height);
    }
  }

  assert(node->size <= N + 1);
}

template <typename K, typename V, std::size_t N, typename Alloc>
const V &BPlusTree<K, V, N, Alloc>::at(const K &key) const {
  const_iterator it = find(key);

  if (it == cend())
    throw std::out_of_range("key not found");

  return *it;
}

template <typename K, typename V, std::size_t N, typename Alloc>
V &BPlusTree<K, V, N, Alloc>::at(const K &key) {
  iterator it = find(key);

  if (it == end())
    throw std::out_of_range("key not found");

  return *it;
}

template <typename K, typename V, std::size_t N, typename Alloc>
bool BPlusTree<K, V, N, Alloc>::contains(const K &key) const noexcept {
  return find<const_iterator>(root, key, 1) != cend();
}

template <typename K, typename V, std::size_t N, typename Alloc>
BPlusTree<K, V, N, Alloc>::iterator
BPlusTree<K, V, N, Alloc>::find(const K &key) {
  return find<iterator>(root, key, 1);
}

template <typename K, typename V, std::size_t N, typename Alloc>
BPlusTree<K, V, N, Alloc>::const_iterator
BPlusTree<K, V, N, Alloc>::find(const K &key) const {
  return find<const_iterator>(root, key, 1);
}

template <typename K, typename V, std::size_t N, typename Alloc>
template <typename Iterator>
Iterator BPlusTree<K, V, N, Alloc>::find(Node *node, const K &key,
                                         unsigned depth) const {
  if (!root)
    return Iterator();

  bool isLeaf = depth >= height;

  std::size_t idx;
  bool found = findKeyInNode(node, key, idx);

  if (isLeaf) {
    if (found) {
      return Iterator(node, idx - 1, true);
    } else {
      return Iterator();
    }

  } else {
    return find<Iterator>(node->children[idx], key, depth + 1);
  }
}

template <typename K, typename V, std::size_t N, typename Alloc>
bool BPlusTree<K, V, N, Alloc>::erase(const K &key) {
  if (!root)
    return false;

  if (keyCount == 1 && root->keys[0] == key) {
    assert(minNode == root);
    assert(minNode->next == nullptr);
    clear();
    return true;
  }

  bool retval = erase(1, root, key);

  // check if height needs to shrink
  if (root->size == 0) {
    Node *newRoot = root->children[0];
    std::destroy_at(root);
    nodeAllocator.deallocate(root, 1);
    root = newRoot;
    height--;
  }

  if (retval) {
    keyCount--;
  }

  return retval;
}

template <typename K, typename V, std::size_t N, typename Alloc>
bool BPlusTree<K, V, N, Alloc>::erase(unsigned depth, Node *node,
                                      const K &key) {

  // find key in current node
  std::size_t idx;
  bool isLeaf = depth >= height;
  bool childIsLeaf = depth + 1 >= height;
  bool found = findKeyInNode(node, key, idx);

  if (isLeaf) {
    if (found) {
      // remove from leaf
      std::destroy_at(node->values[idx - 1]);
      valueAllocator.deallocate(node->values[idx - 1], 1);
      removeKeyFromLeaf(node, idx - 1);
      return true;

    } else {
      // nothing to do
      return false;
    }
  }

  bool retval;
  Node *child;

  if (found) {
    // find next smallest and largest
    // swap smaller with current key and replace larger
    idx--;
    child = node->children[idx];

    unsigned currentDepth = depth + 1;
    Node *nextSmallest = child;
    Node *nextLargest = node->children[idx + 1];

    while (currentDepth < height) {
      nextSmallest = nextSmallest->children[nextSmallest->size];
      nextLargest = nextLargest->children[0];
      currentDepth++;
    }

    assert(nextLargest->keys[0] == key &&
           "Inner node must have duplicate key as direct successor");

    V *valueToErase = nextLargest->values[0];
    const K &nextSmallestKey = nextSmallest->keys[nextSmallest->size - 1];

    // duplicate key of nextSmallest as inner node
    nextLargest->keys[0] = nextSmallestKey;
    node->keys[idx] = nextSmallestKey;

    // swap value nodes
    V *nextSmallestValue = nextSmallest->values[nextSmallest->size - 1];
    nextSmallest->values[nextSmallest->size - 1] = valueToErase;
    nextLargest->values[0] = nextSmallestValue;

    erase(depth + 1, child, nextSmallestKey);
    retval = true;

  } else {
    child = node->children[idx];
    retval = erase(depth + 1, child, key);
  }

  // rebalance tree
  constexpr std::size_t MIN_KEYS = N / 2;

  if (child->size >= MIN_KEYS) {
    return retval;
  }

  assert(child->size == MIN_KEYS - 1);

  // try steal child from left sibling
  Node *leftSibling = idx > 0 ? node->children[idx - 1] : nullptr;
  if (leftSibling && leftSibling->size > MIN_KEYS) {

    if (childIsLeaf) {
      // rotate keys
      node->keys[idx - 1] = leftSibling->keys[leftSibling->size - 1];
      insertLeaf(child, 0, node->keys[idx - 1],
                 leftSibling->values[leftSibling->size - 1]);

    } else {
      // rotate keys
      insertInner(child, 0, std::move(node->keys[idx - 1]), child->children[0]);
      child->children[0] = leftSibling->children[leftSibling->size];
      node->keys[idx - 1] = std::move(leftSibling->keys[leftSibling->size - 1]);
    }

    leftSibling->size--;
    return retval;
  }

  // try steal child from right sibling
  Node *rightSibling = idx < node->size ? node->children[idx + 1] : nullptr;
  if (rightSibling && rightSibling->size > MIN_KEYS) {

    if (childIsLeaf) {
      // rotate keys
      assert(rightSibling->keys[0] == node->keys[idx]);

      child->keys[MIN_KEYS - 1] = rightSibling->keys[0];
      child->values[MIN_KEYS - 1] = rightSibling->values[0];
      child->size++;
      removeKeyFromLeaf(rightSibling, 0);
      node->keys[idx] = rightSibling->keys[0];

    } else {
      // rotate keys
      insertInner(child, MIN_KEYS - 1, std::move(node->keys[idx]),
                  rightSibling->children[0]);

      node->keys[idx] = rightSibling->keys[0];
      removeInnerKey(rightSibling, 0);
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
        assert(child == maxNode);
        maxNode = leftSibling;
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
    std::destroy_at(child);
    nodeAllocator.deallocate(child, 1);

    removeInnerKey(node, idx - 1); // remove child
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
        assert(rightSibling == maxNode);
        maxNode = child;
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

    std::destroy_at(rightSibling);
    nodeAllocator.deallocate(rightSibling, 1);

    // remove right sibling by shifting nodes
    removeInnerKey(node, idx);
    node->children[0] = child;
  }

  return retval;
}

template <typename K, typename V, std::size_t N, typename Alloc>
void BPlusTree<K, V, N, Alloc>::freeValues(Node *node) {
  if (!node)
    return;

  Node *next = node->next;

  for (std::size_t i = 0; i < node->size; i++) {
    std::destroy_at(node->values[i]);
    valueAllocator.deallocate(node->values[i], 1);
  }

  freeValues(next);
}

template <typename K, typename V, std::size_t N, typename Alloc>
void BPlusTree<K, V, N, Alloc>::clear() {
  freeValues(minNode);
  nodeAllocator.reset();
  keyCount = 0;
  height = 0;
  root = nullptr;
  minNode = nullptr;
  maxNode = nullptr;
}
