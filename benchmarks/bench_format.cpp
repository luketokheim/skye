#include <benchmark/benchmark.h>
#include <boost/json/src.hpp>
#include <fmt/core.h>
#include <skye/format.hpp>

#include <chrono>
#include <random>
#include <sstream>

skye::SessionMetrics make_random_metrics()
{
    const static skye::SessionMetrics kMetrics = []() {
        std::mt19937 engine(std::random_device{}());
        std::uniform_int_distribution<int> dist{1, 1 << 16};

        const auto start_time = std::chrono::steady_clock::now();
        const auto end_time =
            start_time + std::chrono::milliseconds{dist(engine)};

        return skye::SessionMetrics{dist(engine), dist(engine), dist(engine),
                                    dist(engine), start_time,   end_time};
    }();

    return kMetrics;
}

void BM_Format_Json_Fmt(benchmark::State& state)
{
    const auto m = make_random_metrics();
    for (auto _ : state) {
        const std::string str = fmt::format("{}", m);

        benchmark::DoNotOptimize(str);
    }
}

BENCHMARK(BM_Format_Json_Fmt);

void BM_Format_Json_Boost(benchmark::State& state)
{
    namespace json = boost::json;

    const auto m = make_random_metrics();
    const json::value v = {
        {"fd", m.fd},
        {"num_request", m.num_request},
        {"bytes_read", m.bytes_read},
        {"bytes_write", m.bytes_write},
        {"duration",
         std::chrono::duration<double>(m.end_time - m.start_time).count()}};

    for (auto _ : state) {
        const std::string str = json::serialize(v);

        benchmark::DoNotOptimize(str);
    }
}

BENCHMARK(BM_Format_Json_Boost);

void BM_Format_Json_Boost_Convert(benchmark::State& state)
{
    namespace json = boost::json;

    const auto m = make_random_metrics();
    for (auto _ : state) {
        const json::value v = {
            {"fd", m.fd},
            {"num_request", m.num_request},
            {"bytes_read", m.bytes_read},
            {"bytes_write", m.bytes_write},
            {"duration",
             std::chrono::duration<double>(m.end_time - m.start_time).count()}};
        const std::string str = json::serialize(v);

        benchmark::DoNotOptimize(str);
    }
}

BENCHMARK(BM_Format_Json_Boost_Convert);

std::ostream& operator<<(std::ostream& out, const skye::SessionMetrics& m)
{
    out << "{\"fd\":" << m.fd << ",\"num_request\":" << m.num_request
        << ",\"bytes_read\":" << m.bytes_read
        << ","
           "\"bytes_write\":"
        << m.bytes_write << ",\"duration\":"
        << std::chrono::duration<double>(m.end_time - m.start_time).count()
        << "}";
    return out;
}

void BM_Format_Json_Iostream(benchmark::State& state)
{
    const auto m = make_random_metrics();
    for (auto _ : state) {
        std::ostringstream out;
        out << m;

        const std::string str = out.str();

        benchmark::DoNotOptimize(str);
    }
}

BENCHMARK(BM_Format_Json_Iostream);
