#include <algorithm>
#include <random>

namespace test {

template <typename Buffer>
Buffer make_random_string(std::size_t n)
{
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<int> dis(32, 127);

    Buffer buf(n, 0);
    std::generate(std::begin(buf), std::end(buf), [&]() { return dis(gen); });

    return buf;
}

} // namespace test