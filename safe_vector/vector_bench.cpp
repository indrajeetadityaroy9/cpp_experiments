#include "vector.hpp"
#include <catch_amalgamated.hpp>
#include <algorithm>
#include <array>
#include <string>
#include <vector>

struct LargeObject {
    std::array<int, 64> data{};
};

// Helper to handle different pop_back signatures
template <typename Vector>
void pop_or_terminate(Vector& vec) {
    using pop_result_t = std::invoke_result_t<decltype(&Vector::pop_back), Vector&>;
    if constexpr (std::is_void_v<pop_result_t>) {
        vec.pop_back();
    } else {
        auto result = vec.pop_back();
        if constexpr (requires { result.has_value(); }) {
            if (!result.has_value()) {
                std::terminate();
            }
        }
    }
}

// =============================================================================
// Integer benchmarks
// =============================================================================
TEST_CASE("Vector<int> push_back", "[benchmark][int]") {
    BENCHMARK("custom::vector push_back 5000 ints") {
        customvector::vector<int> vec;
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(i);
        }
        return vec.size();
    };

    BENCHMARK("std::vector push_back 5000 ints") {
        std::vector<int> vec;
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(i);
        }
        return vec.size();
    };

    BENCHMARK("custom::vector push_back 5000 ints (reserved)") {
        customvector::vector<int> vec;
        vec.reserve(5000);
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(i);
        }
        return vec.size();
    };

    BENCHMARK("std::vector push_back 5000 ints (reserved)") {
        std::vector<int> vec;
        vec.reserve(5000);
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(i);
        }
        return vec.size();
    };
}

TEST_CASE("Vector<int> pop_back", "[benchmark][int]") {
    BENCHMARK("custom::vector pop 5000 ints") {
        customvector::vector<int> vec;
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(i);
        }
        for (int i = 0; i < 5000; ++i) {
            pop_or_terminate(vec);
        }
        return vec.size();
    };

    BENCHMARK("std::vector pop 5000 ints") {
        std::vector<int> vec;
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(i);
        }
        for (int i = 0; i < 5000; ++i) {
            vec.pop_back();
        }
        return vec.size();
    };
}

// =============================================================================
// String benchmarks
// =============================================================================
TEST_CASE("Vector<string> push_back", "[benchmark][string]") {
    BENCHMARK("custom::vector push_back 5000 strings") {
        customvector::vector<std::string> vec;
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(std::to_string(i));
        }
        return vec.size();
    };

    BENCHMARK("std::vector push_back 5000 strings") {
        std::vector<std::string> vec;
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(std::to_string(i));
        }
        return vec.size();
    };

    BENCHMARK("custom::vector push_back 5000 strings (reserved)") {
        customvector::vector<std::string> vec;
        vec.reserve(5000);
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(std::to_string(i));
        }
        return vec.size();
    };

    BENCHMARK("std::vector push_back 5000 strings (reserved)") {
        std::vector<std::string> vec;
        vec.reserve(5000);
        for (int i = 0; i < 5000; ++i) {
            vec.push_back(std::to_string(i));
        }
        return vec.size();
    };
}

// =============================================================================
// Large object benchmarks
// =============================================================================
TEST_CASE("Vector<LargeObject> push_back", "[benchmark][large]") {
    auto make_large = [](int i) {
        LargeObject obj;
        std::fill(obj.data.begin(), obj.data.end(), i);
        return obj;
    };

    BENCHMARK("custom::vector push_back 2000 large objects") {
        customvector::vector<LargeObject> vec;
        for (int i = 0; i < 2000; ++i) {
            vec.push_back(make_large(i));
        }
        return vec.size();
    };

    BENCHMARK("std::vector push_back 2000 large objects") {
        std::vector<LargeObject> vec;
        for (int i = 0; i < 2000; ++i) {
            vec.push_back(make_large(i));
        }
        return vec.size();
    };

    BENCHMARK("custom::vector push_back 2000 large objects (reserved)") {
        customvector::vector<LargeObject> vec;
        vec.reserve(2000);
        for (int i = 0; i < 2000; ++i) {
            vec.push_back(make_large(i));
        }
        return vec.size();
    };

    BENCHMARK("std::vector push_back 2000 large objects (reserved)") {
        std::vector<LargeObject> vec;
        vec.reserve(2000);
        for (int i = 0; i < 2000; ++i) {
            vec.push_back(make_large(i));
        }
        return vec.size();
    };
}

// =============================================================================
// High-volume benchmarks
// =============================================================================
TEST_CASE("Vector high volume operations", "[benchmark][volume]") {
    BENCHMARK("custom::vector 20000 int push_back") {
        customvector::vector<int> vec;
        for (int i = 0; i < 20000; ++i) {
            vec.push_back(i);
        }
        return vec.size();
    };

    BENCHMARK("std::vector 20000 int push_back") {
        std::vector<int> vec;
        for (int i = 0; i < 20000; ++i) {
            vec.push_back(i);
        }
        return vec.size();
    };

    BENCHMARK("custom::vector 20000 int push_back (reserved)") {
        customvector::vector<int> vec;
        vec.reserve(20000);
        for (int i = 0; i < 20000; ++i) {
            vec.push_back(i);
        }
        return vec.size();
    };

    BENCHMARK("std::vector 20000 int push_back (reserved)") {
        std::vector<int> vec;
        vec.reserve(20000);
        for (int i = 0; i < 20000; ++i) {
            vec.push_back(i);
        }
        return vec.size();
    };
}
