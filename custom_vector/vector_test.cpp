#define CATCH_CONFIG_MAIN
#include "vendor/catch_amalgamated.hpp"
#include <string>
#include "vector.cpp"
using customvector::vector;

TEST_CASE("push_back grows capacity and stores values", "[vector]") {
    vector<int> values;

    SECTION("initial state") {
        REQUIRE(values.getSize() == 0);
        REQUIRE(values.getCapacity() == 1);
    }

    values.push_back(10);
    values.push_back(20);
    values.push_back(30);
    values.push_back(40);

    REQUIRE(values.getSize() == 4);
    REQUIRE(values.at(0) == 10);
    REQUIRE(values.at(1) == 20);
    REQUIRE(values.at(2) == 30);
    REQUIRE(values.at(3) == 40);
    REQUIRE(values.getCapacity() >= values.getSize());

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
        REQUIRE(copy.getSize() == original.getSize());
        for (std::size_t i = 0; i < copy.getSize(); ++i) {
            REQUIRE(copy.at(i) == static_cast<int>(i));
        }
    }

    SECTION("move constructor") {
        vector<int> temp(original);
        vector<int> moved(std::move(temp));
        REQUIRE(moved.getSize() == 6);
        for (std::size_t i = 0; i < moved.getSize(); ++i) {
            REQUIRE(moved.at(i) == static_cast<int>(i));
        }
        REQUIRE(temp.getSize() == 0);
        REQUIRE(temp.getCapacity() == 0);
    }

    SECTION("copy assignment") {
        vector<int> copyAssign;
        copyAssign = original;
        REQUIRE(copyAssign.getSize() == original.getSize());
        for (std::size_t i = 0; i < copyAssign.getSize(); ++i) {
            REQUIRE(copyAssign.at(i) == static_cast<int>(i));
        }
    }

    SECTION("move assignment") {
        vector<int> moveAssign;
        vector<int> temp(original);
        moveAssign = std::move(temp);
        REQUIRE(moveAssign.getSize() == 6);
        for (std::size_t i = 0; i < moveAssign.getSize(); ++i) {
            REQUIRE(moveAssign.at(i) == static_cast<int>(i));
        }
        REQUIRE(temp.getSize() == 0);
        REQUIRE(temp.getCapacity() == 0);
    }
}

TEST_CASE("shrinkToFit and pop_back adjust size/capacity", "[vector]") {
    vector<std::string> words;
    words.push_back("alpha");
    words.push_back("beta");
    words.push_back("gamma");

    words.pop_back();
    REQUIRE(words.getSize() == 2);
    REQUIRE(words.at(0) == "alpha");
    REQUIRE(words.at(1) == "beta");

    words.shrinkToFit();
    REQUIRE(words.getCapacity() == words.getSize());
}

TEST_CASE("C++23 std::expected get_checked returns value or error", "[vector][cpp23]") {
    vector<int> values;
    values.push_back(10);
    values.push_back(20);
    values.push_back(30);

    SECTION("valid index returns expected value") {
        auto result = values.get_checked(0);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 10);

        auto result2 = values.get_checked(2);
        REQUIRE(result2.has_value());
        REQUIRE(result2.value() == 30);
    }

    SECTION("invalid index returns error") {
        auto result = values.get_checked(5);
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == customvector::VectorError::IndexOutOfBounds);
    }
}

TEST_CASE("empty() method works correctly", "[vector][cpp23]") {
    vector<int> values;

    REQUIRE(values.empty());

    values.push_back(1);
    REQUIRE_FALSE(values.empty());

    values.pop_back();
    REQUIRE(values.empty());
}

TEST_CASE("iterator support enables range-based for loops", "[vector][cpp23]") {
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
