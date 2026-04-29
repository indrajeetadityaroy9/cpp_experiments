#include <catch_amalgamated.hpp>
#include <string>
#include "vector.hpp"
using customvector::vector;

TEST_CASE("push_back grows capacity and stores values", "[vector]") {
    vector<int> values;

    SECTION("initial state") {
        REQUIRE(values.size() == 0);
        REQUIRE(values.capacity() == vector<int>::kInitialCapacity);
    }

    values.push_back(10);
    values.push_back(20);
    values.push_back(30);
    values.push_back(40);

    REQUIRE(values.size() == 4);
    REQUIRE(values.at(0) == 10);
    REQUIRE(values.at(1) == 20);
    REQUIRE(values.at(2) == 30);
    REQUIRE(values.at(3) == 40);
    REQUIRE(values.capacity() >= values.size());

    SECTION("out of range access throws") {
        REQUIRE_THROWS_AS(values.at(4), std::out_of_range);
    }
}

TEST_CASE("copy and move operations preserve contents", "[vector]") {
    vector<int> original;
    for (int i = 0; i < 6; ++i) {
        original.push_back(i);
    }

    SECTION("copy constructor") {
        vector<int> copy(original);
        REQUIRE(copy.size() == original.size());
        for (std::size_t i = 0; i < copy.size(); ++i) {
            REQUIRE(copy.at(i) == static_cast<int>(i));
        }
    }

    SECTION("move constructor") {
        vector<int> temp(original);
        vector<int> moved(std::move(temp));
        REQUIRE(moved.size() == 6);
        for (std::size_t i = 0; i < moved.size(); ++i) {
            REQUIRE(moved.at(i) == static_cast<int>(i));
        }
        REQUIRE(temp.size() == 0);
        REQUIRE(temp.capacity() == 0);
    }

    SECTION("copy assignment") {
        vector<int> copyAssign;
        copyAssign = original;
        REQUIRE(copyAssign.size() == original.size());
        for (std::size_t i = 0; i < copyAssign.size(); ++i) {
            REQUIRE(copyAssign.at(i) == static_cast<int>(i));
        }
    }

    SECTION("move assignment") {
        vector<int> moveAssign;
        vector<int> temp(original);
        moveAssign = std::move(temp);
        REQUIRE(moveAssign.size() == 6);
        for (std::size_t i = 0; i < moveAssign.size(); ++i) {
            REQUIRE(moveAssign.at(i) == static_cast<int>(i));
        }
        REQUIRE(temp.size() == 0);
        REQUIRE(temp.capacity() == 0);
    }
}

TEST_CASE("shrinkToFit and pop_back adjust size/capacity", "[vector]") {
    vector<std::string> words;
    words.push_back("alpha");
    words.push_back("beta");
    words.push_back("gamma");

    words.pop_back();
    REQUIRE(words.size() == 2);
    REQUIRE(words.at(0) == "alpha");
    REQUIRE(words.at(1) == "beta");

    words.shrinkToFit();
    REQUIRE(words.capacity() == words.size());
}

TEST_CASE("empty() method works correctly", "[vector]") {
    vector<int> values;

    REQUIRE(values.empty());

    values.push_back(1);
    REQUIRE_FALSE(values.empty());

    values.pop_back();
    REQUIRE(values.empty());
}

TEST_CASE("iterator support enables range-based for loops", "[vector]") {
    vector<int> values;
    values.push_back(10);
    values.push_back(20);
    values.push_back(30);

    SECTION("range-based for loop") {
        int sum = 0;
        for (const auto& val : values) {
            sum += val;
        }
        REQUIRE(sum == 60);
    }

    SECTION("begin and end iterators") {
        REQUIRE(*values.begin() == 10);
        REQUIRE(*(values.end() - 1) == 30);
    }

    SECTION("const iterators") {
        const vector<int>& const_values = values;
        int count = 0;
        for (auto it = const_values.cbegin(); it != const_values.cend(); ++it) {
            ++count;
        }
        REQUIRE(count == 3);
    }
}

TEST_CASE("reserve preallocates storage and preserves elements", "[vector]") {
    vector<int> values;
    values.reserve(10);

    const auto initialCapacity = values.capacity();
    REQUIRE(initialCapacity >= 10);

    int lvalue = 42;
    values.push_back(lvalue);
    values.push_back(100);

    REQUIRE(values.capacity() == initialCapacity);
    REQUIRE(values.size() == 2);
    REQUIRE(values.at(0) == 42);
    REQUIRE(values.at(1) == 100);
}

TEST_CASE("pop_back throws when vector is empty", "[vector]") {
    vector<int> values;

    REQUIRE_THROWS_AS(values.pop_back(), std::out_of_range);
}

TEST_CASE("insert adds elements at specified positions", "[vector]") {
    vector<int> values;
    values.push_back(1);
    values.push_back(3);

    values.insert(1, 2);
    REQUIRE(values.size() == 3);
    REQUIRE(values.at(0) == 1);
    REQUIRE(values.at(1) == 2);
    REQUIRE(values.at(2) == 3);

    values.insert(0, 0);
    REQUIRE(values.size() == 4);
    REQUIRE(values.at(0) == 0);
    REQUIRE(values.at(1) == 1);

    values.insert(values.size(), 4);
    REQUIRE(values.size() == 5);
    REQUIRE(values.at(4) == 4);

    REQUIRE_THROWS_AS(values.insert(values.size() + 1, 99), std::out_of_range);
}

namespace {
    struct MoveOnly {
        explicit MoveOnly(int v) : value(v) {}
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
        int value;
    };
}

TEST_CASE("emplace constructs elements in place", "[vector][emplace]") {
    vector<MoveOnly> values;

    values.emplace_back(42);
    REQUIRE(values.size() == 1);
    REQUIRE(values.at(0).value == 42);

    values.emplace(0, 7);
    REQUIRE(values.size() == 2);
    REQUIRE(values.at(0).value == 7);
    REQUIRE(values.at(1).value == 42);
}
