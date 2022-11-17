#pragma once

#include <random>

#include "valik/search/query_result.hpp"

// res_t == std::vector<valik::query_result>
template <typename res_t>
auto generate_thread_result(size_t nr_reads,
                                    size_t nr_bins,
                                    size_t max_hits_per_read) -> res_t
{
    std::mt19937 bin_gen{}; // seed generator
    std::uniform_int_distribution<> bin_distr(0, nr_bins - 1); // define the range

    std::mt19937 per_read_gen{}; // seed generator
    std::uniform_int_distribution<> per_read_distr(0, max_hits_per_read);

    res_t valik_thread_result;
    for(size_t read_id=0; read_id<nr_reads; read_id++)
    {
        std::set<size_t> bin_hits;
        for (int i = 0; i < per_read_distr(per_read_gen); i++)
            bin_hits.insert(bin_distr(bin_gen));    // generate random bin hit

        valik::query_result query_res(std::to_string(read_id), bin_hits);
        valik_thread_result.emplace_back(query_res);
    }

    return valik_thread_result;
}

template <typename res_t>
void print_thread_result(res_t valik_thread_result)
{
    std::cout << "\t\tValik thread results\n";
    std::cout << "Read ID\tBin hits\n";
    for (auto & query_res : valik_thread_result)
    {
        std::cout << query_res.get_id() << '\t';
        for (auto & bin : query_res.get_hits())
            std::cout << bin << '\t';
        std::cout << '\n';
    }
}