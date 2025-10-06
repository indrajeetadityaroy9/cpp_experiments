#include "vector.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using clock_type = std::chrono::high_resolution_clock;
using duration_type = std::chrono::nanoseconds;

struct LargeObject {
    std::array<int, 64> data{};
};

template <typename Fn>
duration_type measure(Fn&& fn) {
    const auto start = clock_type::now();
    fn();
    return std::chrono::duration_cast<duration_type>(clock_type::now() - start);
}

constexpr std::size_t process_runs = 3;

struct ScenarioConfig {
    std::size_t iterations;
    std::size_t reserve_hint;
    const char* label;
    std::size_t samples{3};
};

template <typename Vector>
void reserve_if_available(Vector& vec, std::size_t reserve_hint) {
    if (reserve_hint == 0) {
        return;
    }
    if constexpr (requires(Vector& v) { v.reserve(reserve_hint); }) {
        vec.reserve(reserve_hint);
    }
}

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

template <typename Vector, typename Value>
void insert_or_terminate(Vector& vec, std::size_t index, Value&& value) {
    if constexpr (requires(Vector& v, std::size_t idx, Value&& val) {
                      v.insert(idx, std::forward<Value>(val));
                  }) {
        auto result = vec.insert(index, std::forward<Value>(value));
        if constexpr (requires { result.has_value(); }) {
            if (!result.has_value()) {
                std::terminate();
            }
        }
    } else {
        vec.insert(vec.begin() + static_cast<typename Vector::difference_type>(index),
                   std::forward<Value>(value));
    }
}

template <typename Vector, typename ValueGenerator>
duration_type run_push(ValueGenerator make_value, std::size_t iterations, std::size_t reserve_hint) {
    return measure([&] {
        Vector vec;
        reserve_if_available(vec, reserve_hint);
        for (std::size_t i = 0; i < iterations; ++i) {
            auto value = make_value(i);
            vec.push_back(std::move(value));
        }
    });
}

template <typename Vector, typename ValueGenerator>
duration_type run_pop(ValueGenerator make_value, std::size_t iterations, std::size_t reserve_hint) {
    return measure([&] {
        Vector vec;
        reserve_if_available(vec, reserve_hint);
        for (std::size_t i = 0; i < iterations; ++i) {
            auto value = make_value(i);
            vec.push_back(std::move(value));
        }
        for (std::size_t i = 0; i < iterations; ++i) {
            pop_or_terminate(vec);
        }
    });
}

template <typename Vector, typename ValueGenerator>
duration_type run_insert_middle(ValueGenerator make_value, std::size_t iterations, std::size_t reserve_hint) {
    return measure([&] {
        Vector vec;
        const std::size_t preload = reserve_hint > 0 ? reserve_hint : iterations;
        const std::size_t reserve_target = reserve_hint > 0 ? reserve_hint + iterations : 0;

        reserve_if_available(vec, reserve_target);

        for (std::size_t i = 0; i < preload; ++i) {
            auto value = make_value(i);
            vec.push_back(std::move(value));
        }

        for (std::size_t i = 0; i < iterations; ++i) {
            const std::size_t index = vec.size() / 2;
            auto value = make_value(preload + i);
            insert_or_terminate(vec, index, std::move(value));
            pop_or_terminate(vec);
        }
    });
}

double to_microseconds(duration_type d) {
    return std::chrono::duration<double, std::micro>(d).count();
}

double ratio(double custom_value, double std_value) {
    return std_value != 0.0 ? custom_value / std_value : 0.0;
}

double delta(double custom_value, double std_value) {
    return custom_value - std_value;
}

double percent_delta(double custom_value, double std_value) {
    return std_value != 0.0 ? (custom_value - std_value) / std_value * 100.0 : 0.0;
}

template <typename Fn>
duration_type average_sample(std::size_t samples, Fn&& fn) {
    duration_type total{0};
    for (std::size_t i = 0; i < samples; ++i) {
        total += fn();
    }
    return duration_type(total.count() / static_cast<long long>(samples));
}

template <typename T, typename ValueGenerator>
void profile_type(const char* type_label,
                  ValueGenerator make_value,
                  const ScenarioConfig* scenarios,
                  std::size_t scenario_count) {
    using CustomVec = customvector::vector<T>;
    using StdVec = std::vector<T>;

    std::cout << "\n=== Element type: " << type_label << " ===\n";

    for (std::size_t i = 0; i < scenario_count; ++i) {
        const auto& scenario = scenarios[i];
        std::cout << "\nScenario: " << scenario.label
                  << " (iterations=" << scenario.iterations
                  << ", reserve=" << scenario.reserve_hint
                  << ", samples=" << scenario.samples
                  << ", runs=" << process_runs << ")\n";

        double custom_push_acc = 0.0;
        double std_push_acc = 0.0;
        double custom_pop_acc = 0.0;
        double std_pop_acc = 0.0;
        double custom_insert_acc = 0.0;
        double std_insert_acc = 0.0;

        for (std::size_t run = 0; run < process_runs; ++run) {
            const auto custom_push = average_sample(
                scenario.samples,
                [&] { return run_push<CustomVec>(make_value, scenario.iterations, scenario.reserve_hint); });
            const auto std_push = average_sample(
                scenario.samples,
                [&] { return run_push<StdVec>(make_value, scenario.iterations, scenario.reserve_hint); });
            const auto custom_pop = average_sample(
                scenario.samples,
                [&] { return run_pop<CustomVec>(make_value, scenario.iterations, scenario.reserve_hint); });
            const auto std_pop = average_sample(
                scenario.samples,
                [&] { return run_pop<StdVec>(make_value, scenario.iterations, scenario.reserve_hint); });
            const auto custom_insert = average_sample(
                scenario.samples,
                [&] { return run_insert_middle<CustomVec>(make_value, scenario.iterations, scenario.reserve_hint); });
            const auto std_insert = average_sample(
                scenario.samples,
                [&] { return run_insert_middle<StdVec>(make_value, scenario.iterations, scenario.reserve_hint); });

            custom_push_acc += to_microseconds(custom_push);
            std_push_acc += to_microseconds(std_push);
            custom_pop_acc += to_microseconds(custom_pop);
            std_pop_acc += to_microseconds(std_pop);
            custom_insert_acc += to_microseconds(custom_insert);
            std_insert_acc += to_microseconds(std_insert);
        }

        const double custom_push_us = custom_push_acc / static_cast<double>(process_runs);
        const double std_push_us = std_push_acc / static_cast<double>(process_runs);
        const double custom_pop_us = custom_pop_acc / static_cast<double>(process_runs);
        const double std_pop_us = std_pop_acc / static_cast<double>(process_runs);
        const double custom_insert_us = custom_insert_acc / static_cast<double>(process_runs);
        const double std_insert_us = std_insert_acc / static_cast<double>(process_runs);

        std::cout << "Push back (custom): " << custom_push_us << " µs\n";
        std::cout << "Push back (std):    " << std_push_us << " µs\n";
        std::cout << "delta push (custom - std): " << delta(custom_push_us, std_push_us)
                  << "µs (" << percent_delta(custom_push_us, std_push_us) << " %)" << '\n';
        std::cout << "Ratio (custom/std):  " << ratio(custom_push_us, std_push_us) << 'x' << '\n';

        std::cout << "Pop back (custom):  " << custom_pop_us << " µs\n";
        std::cout << "Pop back (std):     " << std_pop_us << " µs\n";
        std::cout << "delta pop (custom - std): " << delta(custom_pop_us, std_pop_us)
                  << "µs (" << percent_delta(custom_pop_us, std_pop_us) << " %)" << '\n';
        std::cout << "Ratio (custom/std):  " << ratio(custom_pop_us, std_pop_us) << 'x' << '\n';

        std::cout << "Insert mid (custom): " << custom_insert_us << " µs\n";
        std::cout << "Insert mid (std):    " << std_insert_us << " µs\n";
        std::cout << "delta insert (custom - std): " << delta(custom_insert_us, std_insert_us)
                  << "µs (" << percent_delta(custom_insert_us, std_insert_us) << " %)" << '\n';
        std::cout << "Ratio (custom/std):  " << ratio(custom_insert_us, std_insert_us) << 'x' << '\n';
    }
}

int main() {
    constexpr ScenarioConfig scenarios[] = {
        {5'000, 0, "5k no reserve", 3},
        {5'000, 5'000, "5k reserve", 3},
        {20'000, 0, "20k no reserve", 2},
        {20'000, 20'000, "20k reserve", 2},
    };

    const auto scenario_count = sizeof(scenarios) / sizeof(scenarios[0]);

    profile_type<int>(
        "int",
        [](std::size_t i) { return static_cast<int>(i); },
        scenarios,
        scenario_count);

    profile_type<std::string>(
        "std::string",
        [](std::size_t i) { return std::to_string(i); },
        scenarios,
        scenario_count);

    profile_type<LargeObject>(
        "LargeObject (64 ints)",
        [](std::size_t i) {
            LargeObject obj;
            std::fill(obj.data.begin(), obj.data.end(), static_cast<int>(i));
            return obj;
        },
        scenarios,
        scenario_count);

    return 0;
}
