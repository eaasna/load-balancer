#pragma once

#include <unordered_map>

#include <seqan3/core/debug_stream.hpp>

#include "cart.hpp"

struct cart_queue {
    size_t cart_max_capacity;
    std::unordered_map<size_t, cart> carts_being_filled;

    void insert(size_t bin_id, std::string const& query)
    {
        // Only adds a cart if it doesn't exists yet
        auto [iter, succ] = carts_being_filled.try_emplace(bin_id, bin_id);
        auto& c = iter->second;
        if (c.get_occupancy() == cart_max_capacity) return; //!TODO this if shouldn't be here
        c.add_query(query);
        if (c.get_occupancy() == cart_max_capacity) {
            //!TODO do something more useful
        }
    }
};

void print_queue_carts(std::unordered_map<size_t, cart> cart_queue)
{
    seqan3::debug_stream << "\t\t\tCARTS\n";
    seqan3::debug_stream << "Bin ID\tQueries\n";
    for (auto & [bin_id, cart] : cart_queue)
    {
        seqan3::debug_stream << cart.get_bin() << '\t';
        for (auto & query : cart.get_queries())
            seqan3::debug_stream << query << '\t';

        seqan3::debug_stream << '\n';
    }
}
