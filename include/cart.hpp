#pragma once

#include <set>

class cart
{
    private:
        size_t bin_id;
        std::set<std::string> queries;

    public:
        cart() = default;
        cart(cart const &) = default;
        cart & operator=(cart const &) = default;
        cart(cart &&) = default;
        cart & operator=(cart &&) = default;
        ~cart() = default;

        cart(size_t id)
            : bin_id{id}
        {}

        const size_t & get_bin() const
        {
            return bin_id;
        }

        size_t get_occupancy() const
        {
            return queries.size();
        }

        const std::set<std::string> & get_queries() const
        {
            return queries;
        }

        void add_query(std::string query_id)
        {
            queries.insert(query_id);
        }
};
