/**
 * Unit tests for PartialOrderDS
 */

#include "../include/algorithms/partial_order_ds.hpp"
#include <catch_amalgamated.hpp>
#include <algorithm>

using namespace duan;

TEST_CASE("PartialOrderDS initialization", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);
    REQUIRE(ds.empty());
}

TEST_CASE("PartialOrderDS single insert and pull", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    ds.Insert(1, 50.0);
    REQUIRE_FALSE(ds.empty());
    REQUIRE(ds.total_elements() == 1);

    auto [keys, sep] = ds.Pull();
    REQUIRE(keys.size() == 1);
    REQUIRE(keys[0] == 1);
    REQUIRE(ds.empty());
}

TEST_CASE("PartialOrderDS duplicate keys keep minimum", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    ds.Insert(1, 50.0);
    ds.Insert(1, 30.0);  // Should update to 30.0
    ds.Insert(1, 60.0);  // Should ignore (30.0 is better)

    REQUIRE(ds.total_elements() == 1);

    auto [keys, sep] = ds.Pull();
    REQUIRE(keys.size() == 1);
    REQUIRE(keys[0] == 1);
}

TEST_CASE("PartialOrderDS multiple inserts and pull", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(5, 100.0);

    ds.Insert(1, 10.0);
    ds.Insert(2, 20.0);
    ds.Insert(3, 30.0);

    REQUIRE(ds.total_elements() == 3);

    auto [keys, sep] = ds.Pull();
    REQUIRE(keys.size() == 3);

    // Keys should be sorted by value
    std::sort(keys.begin(), keys.end());
    REQUIRE(keys[0] == 1);
    REQUIRE(keys[1] == 2);
    REQUIRE(keys[2] == 3);
}

TEST_CASE("PartialOrderDS pull with M limit", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(3, 100.0);  // M = 3

    for (int i = 0; i < 10; ++i) {
        ds.Insert(i, 10.0 * i);
    }

    auto [keys, sep] = ds.Pull();
    REQUIRE(keys.size() == 3);  // Should return at most M elements

    // Should get the 3 smallest
    std::sort(keys.begin(), keys.end());
    REQUIRE(keys[0] == 0);
    REQUIRE(keys[1] == 1);
    REQUIRE(keys[2] == 2);

    // Separator should be > max pulled value
    REQUIRE(sep > 20.0);
}

TEST_CASE("PartialOrderDS BatchPrepend small set", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    vector<KeyValuePair> batch = {
        {1, 5.0},
        {2, 10.0},
        {3, 15.0}
    };

    ds.BatchPrepend(batch);
    REQUIRE(ds.total_elements() == 3);

    auto [keys, sep] = ds.Pull();
    REQUIRE(keys.size() == 3);
}

TEST_CASE("PartialOrderDS BatchPrepend large set", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(5, 100.0);  // M = 5

    vector<KeyValuePair> batch;
    for (int i = 0; i < 20; ++i) {
        batch.emplace_back(i, 5.0 * i);
    }

    ds.BatchPrepend(batch);
    REQUIRE(ds.total_elements() == 20);

    // Pull should get first M elements
    auto [keys, sep] = ds.Pull();
    REQUIRE(keys.size() == 5);
}

TEST_CASE("PartialOrderDS BatchPrepend with duplicates", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    vector<KeyValuePair> batch = {
        {1, 10.0},
        {1, 5.0},   // Smaller value should win
        {1, 15.0}
    };

    ds.BatchPrepend(batch);
    REQUIRE(ds.total_elements() == 1);  // Only one key

    auto [keys, sep] = ds.Pull();
    REQUIRE(keys.size() == 1);
    REQUIRE(keys[0] == 1);
}

TEST_CASE("PartialOrderDS interleaved operations", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(5, 100.0);

    ds.Insert(1, 10.0);
    ds.Insert(2, 20.0);

    vector<KeyValuePair> batch = {{3, 5.0}, {4, 15.0}};
    ds.BatchPrepend(batch);

    ds.Insert(5, 25.0);

    REQUIRE(ds.total_elements() == 5);

    auto [keys, sep] = ds.Pull();
    REQUIRE(keys.size() == 5);
}

TEST_CASE("PartialOrderDS pull from empty", "[partial_order_ds]") {
    PartialOrderDS ds;
    ds.Initialize(10, 100.0);

    auto [keys, sep] = ds.Pull();
    REQUIRE(keys.empty());
    REQUIRE(sep == 100.0);  // Should return B
}

TEST_CASE("PartialOrderDS ordering preserved", "[partial_order_ds]") {
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
    REQUIRE(keys.size() == 3);

    // Extract values and check ordering
    std::sort(keys.begin(), keys.end());
    REQUIRE(keys[0] == 1);  // value 10.0
    REQUIRE(keys[1] == 2);  // value 20.0
    REQUIRE(keys[2] == 4);  // value 40.0
}
