#pragma once

#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

struct cart_queue {
    // same as cart, but with additional mutex
    struct secured_cart {
        std::mutex mutex;
        std::vector<std::string> queries;
    };

    std::vector<secured_cart> carts_being_filled;
    size_t cart_max_capacity;

    std::mutex filled_carts_mutex;
    std::condition_variable filled_carts_cv;
    std::vector<std::tuple<size_t, std::vector<std::string>>> filled_carts;
    size_t max_queued_carts;

    cart_queue(size_t number_of_bins, size_t cart_max_capacity)
        : carts_being_filled(number_of_bins)
        , cart_max_capacity{cart_max_capacity}
    {
        for (auto& cart : carts_being_filled) {
            cart.queries.reserve(cart_max_capacity);
        }
    }

    // Insert a query into a bin - thread safe
    void insert(size_t bin_id, std::string query) {
        assert(carts_being_filled.size() < bin_id && "bin_id has to be between 0 and number_of_bins");

        auto& cart = carts_being_filled[bin_id];

        // locking the cart and inserting the query
        auto _ = std::lock_guard{cart.mutex};
        assert(cart.queries.size() < cart_max_capacity && "carts should always have at least one empty value");
        cart.queries.emplace_back(std::move(query));

        // check if cart is full
        if (cart.queries.size() == cart_max_capacity) {
            // lock full cart queue
            auto _ = std::unique_lock{filled_carts_mutex};
            filled_carts.emplace_back(bin_id, std::move(cart.queries));
            cart.queries.reserve(cart_max_capacity);
            filled_carts_cv.notify_one();
        }
    }

    // Take a cart from the filled_carts list - thread safe
    auto dequeue() -> std::tuple<int ,std::vector<std::string>> {
        auto g = std::unique_lock{filled_carts_mutex};

        // if no cart is available, wait
        while (filled_carts.empty()) {
            filled_carts_cv.wait(g);
        }

        // move cart from queue and return it
        auto v = std::move(filled_carts.back());
        filled_carts.pop_back();
        return v;
    }

    // flushes all partially filled carts to the filled_carts list - thread safe
    void flush() {
        for (size_t bin_id{0}; bin_id < carts_being_filled.size(); ++bin_id) {
            auto& cart = carts_being_filled[bin_id];
            auto g1 = std::unique_lock{cart.mutex};
            auto g2 = std::unique_lock{filled_carts_mutex};
            filled_carts.emplace_back(bin_id, std::move(cart.queries));
            cart.queries.reserve(cart_max_capacity);
            filled_carts_cv.notify_one();
        }
    }
};

void print_queue_carts(std::vector<cart_queue::secured_cart> const& cart_queue)
{
    std::cout << "\t\t\tCARTS\n";
    std::cout << "Bin ID\tQueries\n";
    for (size_t bin_id{0}; bin_id < cart_queue.size(); ++bin_id) {
        auto const& cart = cart_queue[bin_id];
        std::cout << bin_id << '\t';

        for (auto & query : cart.queries)
            std::cout << query << '\t';

        std::cout << '\n';
    }
}
