#pragma once

#include <unordered_map>
#include <condition_variable>
#include <mutex>
#include <future>

#include <seqan3/core/debug_stream.hpp>

#include "cart.hpp"

struct cart_queue {
    size_t cart_max_capacity;
    //!TODO: these should be separated
    std::unordered_map<size_t, cart> carts_being_filled;
    std::unordered_map<size_t, cart> full_carts;
    std::condition_variable cv;
    std::mutex fill_mutex;

    void insert(size_t bin_id, std::string const& query)
    {
        std::lock_guard<std::mutex> lk(fill_mutex);
        // Only adds a cart if it doesn't exist yet
        auto [iter, succ] = carts_being_filled.try_emplace(bin_id, bin_id); // calls cart(bin_id) constructor

        //!TODO: thread should lock c to insert into it
        auto& c = iter->second;
        c.add_query(query);
        if (c.get_occupancy() == cart_max_capacity)
        {
            full_carts.emplace(std::make_pair(full_carts.size(), c));
            carts_being_filled.erase(bin_id);
        }
    }

    bool full_carts_in_queue()
    {
        return (full_carts.size() > 0);
    }

    cart take_full_cart()
    {
        //!TODO: what if there aren't any more full carts
        // flush half-filled carts
        std::lock_guard<std::mutex> lk(fill_mutex);
        auto [bin_id, full_cart] = *(full_carts).begin();
        full_carts.erase(bin_id);
        return full_cart;
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
