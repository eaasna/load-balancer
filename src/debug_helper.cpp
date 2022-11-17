#include "debug_helper.hpp"

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