#pragma once

#include <algorithm>
#include <random>
#include <type_traits>

namespace test {

template <typename Buffer>
Buffer make_random_string(std::size_t count)
{
    static_assert(std::is_same_v<char, typename Buffer::value_type>);

    std::mt19937 gen{std::random_device{}()};

    std::uniform_int_distribution<int> dis{32, 127};

    Buffer buf(count, 0);
    std::generate(
        buf.begin(), buf.end(), [&]() { return static_cast<char>(dis(gen)); });

    return buf;
}

} // namespace test
