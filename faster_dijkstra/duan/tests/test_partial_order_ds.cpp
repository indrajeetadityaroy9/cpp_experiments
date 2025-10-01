/**
 * Unit tests for PartialOrderDS
 */

#include "../include/partial_order_ds.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>

using namespace duan;

// Test result tracking
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    std::cout << "Running test: " << #name << "... "; \
    try { \
        name(); \
        std::cout << "PASSED\n"; \
        tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << "\n"; \
        tests_failed++; \
    }

// Test 1: Basic initialization and empty check
void test_initialization() {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);
    assert(ds.empty());
}

// Test 2: Single insert and pull
void test_single_insert() {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    ds.Insert(1, 50.0);
    assert(!ds.empty());
    assert(ds.total_elements() == 1);

    auto [keys, sep] = ds.Pull();
    assert(keys.size() == 1);
    assert(keys[0] == 1);
    assert(ds.empty());
}

// Test 3: Multiple inserts with same key (keep minimum)
void test_duplicate_keys() {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    ds.Insert(1, 50.0);
    ds.Insert(1, 30.0);  // Should update to 30.0
    ds.Insert(1, 60.0);  // Should ignore (30.0 is better)

    assert(ds.total_elements() == 1);

    auto [keys, sep] = ds.Pull();
    assert(keys.size() == 1);
    assert(keys[0] == 1);
}

// Test 4: Multiple inserts and pull
void test_multiple_inserts() {
    PartialOrderDS ds;
    ds.Initialize(5, 100.0);

    ds.Insert(1, 10.0);
    ds.Insert(2, 20.0);
    ds.Insert(3, 30.0);

    assert(ds.total_elements() == 3);

    auto [keys, sep] = ds.Pull();
    assert(keys.size() == 3);

    // Keys should be sorted by value
    std::sort(keys.begin(), keys.end());
    assert(keys[0] == 1 && keys[1] == 2 && keys[2] == 3);
}

// Test 5: Pull with M limit
void test_pull_with_limit() {
    PartialOrderDS ds;
    ds.Initialize(3, 100.0);  // M = 3

    for (int i = 0; i < 10; ++i) {
        ds.Insert(i, 10.0 * i);
    }

    auto [keys, sep] = ds.Pull();
    assert(keys.size() == 3);  // Should return at most M elements

    // Should get the 3 smallest
    std::sort(keys.begin(), keys.end());
    assert(keys[0] == 0 && keys[1] == 1 && keys[2] == 2);

    // Separator should be > max pulled value
    assert(sep > 20.0);
}

// Test 6: BatchPrepend small set (L ≤ M)
void test_batch_prepend_small() {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    vector<KeyValuePair> batch = {
        {1, 5.0},
        {2, 10.0},
        {3, 15.0}
    };

    ds.BatchPrepend(batch);
    assert(ds.total_elements() == 3);

    auto [keys, sep] = ds.Pull();
    assert(keys.size() == 3);
}

// Test 7: BatchPrepend large set (L > M)
void test_batch_prepend_large() {
    PartialOrderDS ds;
    ds.Initialize(5, 100.0);  // M = 5

    vector<KeyValuePair> batch;
    for (int i = 0; i < 20; ++i) {
        batch.emplace_back(i, 5.0 * i);
    }

    ds.BatchPrepend(batch);
    assert(ds.total_elements() == 20);

    // Pull should get first M elements
    auto [keys, sep] = ds.Pull();
    assert(keys.size() == 5);
}

// Test 8: BatchPrepend with duplicates
void test_batch_prepend_duplicates() {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    vector<KeyValuePair> batch = {
        {1, 10.0},
        {1, 5.0},   // Smaller value should win
        {1, 15.0}
    };

    ds.BatchPrepend(batch);
    assert(ds.total_elements() == 1);  // Only one key

    auto [keys, sep] = ds.Pull();
    assert(keys.size() == 1);
    assert(keys[0] == 1);
}

// Test 9: Interleaved operations
void test_interleaved_operations() {
    PartialOrderDS ds;
    ds.Initialize(5, 100.0);

    ds.Insert(1, 10.0);
    ds.Insert(2, 20.0);

    vector<KeyValuePair> batch = {{3, 5.0}, {4, 15.0}};
    ds.BatchPrepend(batch);

    ds.Insert(5, 25.0);

    assert(ds.total_elements() == 5);

    auto [keys, sep] = ds.Pull();
    assert(keys.size() == 5);
}

// Test 10: Pull from empty DS
void test_pull_empty() {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    auto [keys, sep] = ds.Pull();
    assert(keys.empty());
    assert(sep == 100.0);  // Should return B
}

// Test 11: Block splitting behavior
void test_block_splitting() {
    PartialOrderDS ds;
    ds.Initialize(4, 100.0);  // Small M to trigger splits

    // Insert more than M elements
    for (int i = 0; i < 10; ++i) {
        ds.Insert(i, 10.0 * i);
    }

    // Should still contain all elements
    assert(ds.total_elements() == 10);

    // Pull all elements in batches
    int total_pulled = 0;
    while (!ds.empty()) {
        auto [keys, sep] = ds.Pull();
        total_pulled += keys.size();
        assert(keys.size() <= 4);  // Should not exceed M
    }

    // Should have pulled all 10 elements
    assert(total_pulled == 10);
}

// Test 12: Ordering preserved
void test_ordering_preserved() {
    PartialOrderDS ds;
    ds.Initialize(3, 100.0);

    // Insert in random order
    ds.Insert(5, 50.0);
    ds.Insert(2, 20.0);
    ds.Insert(8, 80.0);
    ds.Insert(1, 10.0);
    ds.Insert(4, 40.0);

    // Pull M elements - should get 3 smallest
    auto [keys, sep] = ds.Pull();
    assert(keys.size() == 3);

    // Extract values and check ordering
    std::sort(keys.begin(), keys.end());
    assert(keys[0] == 1);  // value 10.0
    assert(keys[1] == 2);  // value 20.0
    assert(keys[2] == 4);  // value 40.0
}

int main() {
    std::cout << "=== PartialOrderDS Unit Tests ===\n\n";

    TEST(test_initialization);
    TEST(test_single_insert);
    TEST(test_duplicate_keys);
    TEST(test_multiple_inserts);
    TEST(test_pull_with_limit);
    TEST(test_batch_prepend_small);
    TEST(test_batch_prepend_large);
    TEST(test_batch_prepend_duplicates);
    TEST(test_interleaved_operations);
    TEST(test_pull_empty);
    TEST(test_ordering_preserved);

    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return (tests_failed == 0) ? 0 : 1;
}
