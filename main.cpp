#include <algorithm>
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "bplustree.hpp"

template<std::size_t N>
void test_boolean_insertion_deletion(int testRange) {
    BPlusTree<int, bool, N> tree;
    std::unordered_map<int, bool> keys;

    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys[key] = i % 2 == 0;
        tree.insert(key, i % 2 == 0);
        assert(tree.size() == keys.size());
    }

    for (const auto& key : keys) {
        bool expectedValue = key.second;
        assert(tree.at(key.first) == expectedValue);
    }

    for (const auto& key : keys) {
        assert(tree.erase(key.first));
    }

    assert(tree.size() == 0);
    std::cout << "test_boolean_insertion_deletion<" << N << "> passed!" << std::endl;
}

template<std::size_t N>
void test_int_insertion_deletion(int testRange) {
    BPlusTree<int, int, N> tree;
    std::unordered_map<int, int> keys;

    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys[key] = key;
        tree.insert(key, key);
        assert(tree.size() == keys.size());
    }

    for (const auto& key : keys) {
        assert(tree.at(key.first) == key.second);
    }

    for (const auto& key : keys) {
        assert(tree.erase(key.first));
    }

    assert(tree.size() == 0);
    std::cout << "test_int_insertion_deletion<" << N << "> passed!" << std::endl;
}

template<std::size_t N>
void test_string_insertion_deletion(int testRange) {
    BPlusTree<int, std::string, N> tree;
    std::unordered_set<int> keys;

    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys.insert(key);
        tree.insert(key, "Value_" + std::to_string(key));
    }

    assert(tree.size() == keys.size());

    for (const auto& key : keys) {
        std::string expectedValue = "Value_" + std::to_string(key);
        assert(tree.at(key) == expectedValue);
    }

    for (const auto& key : keys) {
        assert(tree.erase(key));
    }

    assert(tree.size() == 0);
    std::cout << "test_string_insertion_deletion<" << N << "> passed!" << std::endl;
}

template<std::size_t N>
void test_iteration(int testRange) {
    BPlusTree<int, int, N> tree;
    std::vector<int> keys;

    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
            keys.push_back(key);
            tree.insert(key, key);
        }
    }

    std::sort(keys.begin(), keys.end());
    auto it = tree.begin();
    for (const auto& key : keys) {
        assert(it != tree.end());
        assert(*it == key);
        ++it;
    }
    assert(it == tree.end());

    auto rit = tree.rbegin();
    for (auto rkey = keys.rbegin(); rkey != keys.rend(); ++rkey) {
        assert(rit != tree.rend());
        assert(*rit == *rkey);
        ++rit;
    }
    assert(rit == tree.rend());
    std::cout << "test_iteration<" << N << "> passed!" << std::endl;
}

struct CopyCounter {
    static size_t copyCount;

    CopyCounter() = default;

    CopyCounter(const CopyCounter&) {
        ++copyCount;
    }

    CopyCounter& operator=(const CopyCounter&) {
        ++copyCount;
        return *this;
    }

    CopyCounter(CopyCounter&&) noexcept {}

    CopyCounter& operator=(CopyCounter&&) noexcept { return *this; }

    static void reset() {
        copyCount = 0;
    }
};

size_t CopyCounter::copyCount = 0;

template<std::size_t N>
void test_copy_counter(int testRange) {
    BPlusTree<int, CopyCounter, N> tree;
    std::unordered_set<int> keys;

    CopyCounter::reset();

    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys.insert(key);
        tree.insert(key, std::move(CopyCounter{}));
    }

    assert(tree.size() == keys.size());

    for (const auto& key : keys) {
        assert(tree.contains(key));
    }

    for (const auto& key : keys) {
        assert(tree.erase(key));
    }

    assert(tree.size() == 0);
    assert(CopyCounter::copyCount == 0 && "CopyCounter was copied during insertion or deletion!");
    
    CopyCounter c;
    tree.insert(111, c);

    assert(tree.size() == 1);
    assert(CopyCounter::copyCount == 1 && "CopyCounter was not copied!");
    std::cout << "test_copy_counter<" << N << "> passed!" << std::endl;
}

template<std::size_t N>
void test_at_and_find(int testRange) {
    BPlusTree<int, int, N> tree;
    std::unordered_map<int, int> keys;

    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys[key] = key;
        tree.insert(key, key);
    }

    for (const auto& key : keys) {
        assert(tree.at(key.first) == key.second);
    }

    const BPlusTree<int, int, N>& constTree = tree;
    for (const auto& key : keys) {
        assert(constTree.at(key.first) == key.second);
    }

    for (const auto& key : keys) {
        auto it = tree.find(key.first);
        assert(it != tree.end());
        assert(*it == key.second);
    }

    for (const auto& key : keys) {
        auto it = constTree.find(key.first);
        assert(it != constTree.end());
        assert(*it == key.second);
    }

    for (const auto& key : keys) {
        assert(tree.contains(key.first));
        assert(constTree.contains(key.first));
    }

    assert(!tree.contains(-1));
    assert(!constTree.contains(-1));

    std::cout << "test_at_and_find<" << N << "> passed!" << std::endl;
}

template<std::size_t N>
void test_const_at_and_find(int testRange) {
    BPlusTree<int, int, N> tree;
    std::unordered_map<int, int> keys;

    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys[key] = key;
        tree.insert(key, key);
    }

    const BPlusTree<int, int, N>& constTree = tree;
    for (const auto& key : keys) {
        assert(constTree.at(key.first) == key.second);
    }

    for (const auto& key : keys) {
        auto it = constTree.find(key.first);
        assert(it != constTree.end());
        assert(*it == key.second);
    }

    std::cout << "test_const_at_and_find<" << N << "> passed!" << std::endl;
}

template<std::size_t N>
void test_constness_of_find(int testRange) {
    BPlusTree<int, int, N> tree;
    std::unordered_map<int, int> keys;

    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys[key] = key;
        tree.insert(key, key);
    }

    const BPlusTree<int, int, N>& constTree = tree;
    for (const auto& key : keys) {
        auto it = constTree.find(key.first);
        assert(it != constTree.end());
        assert(*it == key.second);
    }

    for (const auto& key : keys) {
        auto it = tree.find(key.first);
        assert(it != tree.end());
        assert(*it == key.second);
    }

    std::cout << "test_constness_of_find<" << N << "> passed!" << std::endl;
}

int main() {
    std::srand(420);

    test_boolean_insertion_deletion<3>(100);
    test_string_insertion_deletion<3>(100);
    test_int_insertion_deletion<3>(100);
    test_iteration<3>(100);

    test_boolean_insertion_deletion<4>(100);
    test_string_insertion_deletion<4>(100);
    test_int_insertion_deletion<4>(100);
    test_iteration<4>(100);

    test_boolean_insertion_deletion<5>(1000);
    test_string_insertion_deletion<5>(100);
    test_int_insertion_deletion<5>(1000);
    test_iteration<5>(1000);

    test_int_insertion_deletion<9>(10000);
    test_int_insertion_deletion<22>(10000);
    test_int_insertion_deletion<95>(10000);

    test_copy_counter<4>(100);
    test_copy_counter<5>(100);

    test_at_and_find<4>(100);
    test_const_at_and_find<4>(100);
    test_constness_of_find<4>(100);

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
