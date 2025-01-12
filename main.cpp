#include <iostream>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include "bplustree.hpp"

#define SEED 929

template<std::size_t N>
void test_boolean_insertion_deletion(int testRange) {
    BPlusTree<int, bool, N> tree;

    std::unordered_map<int, bool> keys;

    std::cout << "Testing boolean insertion and deletion with N = " << N << "\n";

    // Insert random boolean values
    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys[key] = i % 2 == 0;
        tree.insert(key, i % 2 == 0); // Alternate true/false
        std::cout << "Inserted key: " << key << " with value: " << (i % 2 == 0) << std::endl;
            assert(tree.size() == keys.size());

    }

    // Verify the size of the tree

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
void test_int_insertion_deletion() {
    BPlusTree<int, int, N> tree;

    const int testRange = 100;
    std::unordered_map<int, int> keys;

    std::cout << "Testing boolean insertion and deletion with N = " << N << "\n";

    // Insert random boolean values
    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (10 * testRange);
        keys[key] = key;
        tree.insert(key, key); // Alternate true/false
        std::cout << "Inserted key: " << key << " with value: " << key << std::endl;
            assert(tree.size() == keys.size());
    
        for (auto it = tree.rbegin(); it != tree.rend(); --it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
    }

    // Verify the size of the tree

    // Validate inserted values
    for (const auto& key : keys) {
        assert(tree.at(key.first) == key.second);
    }
    std::cout << "All inserted boolean values are correct.\n";

    // Delete all keys and ensure the tree is empty
    for (const auto& key : keys) {

        assert(tree.erase(key.first));

        std::cout << "Deleted key: " << key.first << "\n";
        for (auto it = tree.rbegin(); it != tree.rend(); --it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
    }

    // Verify the size of the tree
    assert(tree.size() == 0);
    std::cout << "Tree size after deletion: " << tree.size() << "\n";
}

template<std::size_t N>
void test_string_insertion_deletion(int testRange) {
    BPlusTree<int, std::string, N> tree;

    std::unordered_set<int> keys;

    std::cout << "Testing string insertion and deletion with N = " << N << "\n";

    // Insert random string values
    for (int i = 0; i < testRange; ++i) {
        int key = std::rand() % (testRange * 10);
        keys.insert(key);
        tree.insert(key, "Value_" + std::to_string(key));
        std::cout << "Inserted key: " << key << " with value: " << "Value_" + std::to_string(key) << "\n";
    }

    assert(tree.size() == keys.size());
    std::cout << "Tree size after insertion: " << tree.size() << "\n";

    for (const auto& key : keys) {
        std::string expectedValue = "Value_" + std::to_string(key);
        assert(tree.at(key) == expectedValue);
    }
    std::cout << "All inserted string values are correct.\n";

    // Delete all keys and ensure the tree is empty
    for (const auto& key : keys) {
        assert(tree.erase(key));
        std::cout << "Deleted key: " << key << "\n";
        for (auto it = tree.rbegin(); it != tree.rend(); --it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl; 
    }

    assert(tree.size() == 0);
    std::cout << "Tree size after deletion: " << tree.size() << "\n";
}

int main() {
    std::srand(SEED);

    std::cout << "Running tests for BTree with N = 3" << std::endl;
    test_boolean_insertion_deletion<3>(100);
    test_string_insertion_deletion<3>(100);

    std::cout << "Running tests for BTree with N = 4" << std::endl;
    test_boolean_insertion_deletion<4>(100);
    test_string_insertion_deletion<4>(100);

    std::cout << "Running tests for BTree with N = 5" << std::endl;
    test_boolean_insertion_deletion<5>(10000);
    test_string_insertion_deletion<5>(100);

    test_boolean_insertion_deletion<9>(10000);
    test_boolean_insertion_deletion<22>(10000);
    test_boolean_insertion_deletion<95>(10000);

    std::cout << "All tests passed successfully!" << std::endl;

    return 0;
}
