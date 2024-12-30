#include <algorithm>
#include <iostream>
#include <cassert>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "btree.hpp"

#define SEED 99

template<std::size_t N>
void test_boolean_insertion_deletion() {
    BTree<int, bool, N> tree;

    const int testRange = 100;
    std::unordered_map<int, bool> keys;

    std::cout << "Testing boolean insertion and deletion with N = " << N << "\n";

    // Insert random boolean values
    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys[key] = i % 2 == 0;
        tree.insert(key, i % 2 == 0); // Alternate true/false
        std::cout << "Inserted key: " << key << " with value: " << (i % 2 == 0) << "\n";
        assert(tree.size() == keys.size());
    }

    // Verify the size of the tree
    std::cout  << tree.toString() << "\n";

    // Validate inserted values
    for (const auto& key : keys) {
        bool expectedValue = key.second;
        assert(tree.at(key.first) == expectedValue);
    }
    std::cout << "All inserted boolean values are correct.\n";

    // Delete all keys and ensure the tree is empty
    for (const auto& key : keys) {
        assert(tree.erase(key.first));
        std::cout << "Deleted key: " << key.first << "\n";
    }

    // Verify the size of the tree
    assert(tree.size() == 0);
    std::cout << "Tree size after deletion: " << tree.size() << "\n";
}

template<std::size_t N>
void test_string_insertion_deletion() {
    BTree<int, std::string, N> tree;

    const int testRange = 100;
    std::unordered_set<int> keys;

    std::cout << "Testing string insertion and deletion with N = " << N << "\n";

    // Insert random string values
    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % 1000;
        keys.insert(key);
        tree.insert(key, "Value_" + std::to_string(key));
        std::cout << "Inserted key: " << key << " with value: " << "Value_" + std::to_string(key) << "\n";
    }

    // Verify the size of the tree
    assert(tree.size() == keys.size());
    std::cout << "Tree size after insertion: " << tree.size() << "\n";

    // Validate inserted values
    for (const auto& key : keys) {
        std::string expectedValue = "Value_" + std::to_string(key);
        assert(tree.at(key) == expectedValue);
    }
    std::cout << "All inserted string values are correct.\n";

    // Delete all keys and ensure the tree is empty
    for (const auto& key : keys) {
        assert(tree.erase(key));
        std::cout << "Deleted key: " << key << "\n";
    }

    // Verify the size of the tree
    assert(tree.size() == 0);
    std::cout << "Tree size after deletion: " << tree.size() << "\n";
}

int main() {
    std::srand(SEED);

    std::cout << "Running tests for BTree with N = 3" << std::endl;
    test_boolean_insertion_deletion<3>();

    test_string_insertion_deletion<3>();

    std::cout << "Running tests for BTree with N = 4" << std::endl;
    test_boolean_insertion_deletion<4>();
    test_string_insertion_deletion<4>();

    std::cout << "Running tests for BTree with N = 5" << std::endl;
    test_boolean_insertion_deletion<5>();
    test_string_insertion_deletion<5>();

    std::cout << "All tests passed successfully!" << std::endl;

    return 0;
}
