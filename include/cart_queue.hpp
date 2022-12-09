#pragma once

#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

/** A cart queue groups values together and sends them to consumers in batches.
 *
 * The cart queue is organized in so called carts. There is a list of carts that currently are being filled.
 * Each cart that is being filled, collects values for a unique bin. If a cart is full it is being pushed
 * to to full cart queue. The full cart queue then can be processed by consumers.
 *
 * The cart queue is designed to be accessed by multiple consumers and producers concurrently.
 */
struct cart_queue {
    using cart = std::vector<std::string>;

    // same as cart, but with additional mutex
    struct secured_cart {
        std::mutex mutex;
        cart       basket;
    };

     //!< List of carts that are ready to be filled
    std::vector<secured_cart> carts_being_filled;

    //!< Maximum number of elements that should be put into a cart before it is send of to be consumed
    size_t cart_max_capacity;


    std::mutex filled_carts_mutex;
    std::condition_variable filled_carts_process_ready_cv;
    std::condition_variable filled_carts_queue_ready_cv;

    std::vector<std::tuple<size_t, cart>> filled_carts;
    size_t max_queued_carts;
    bool finishing{false};

    cart_queue(size_t number_of_bins, size_t cart_max_capacity, size_t max_queued_carts)
        : carts_being_filled(number_of_bins)
        , cart_max_capacity{cart_max_capacity}
        , max_queued_carts{max_queued_carts}
    {
        for (auto& cart : carts_being_filled) {
            cart.basket.reserve(cart_max_capacity);
        }
    }

    // Insert a query into a bin - thread safe
    void insert(size_t bin_id, std::string query) {
        assert(carts_being_filled.size() < bin_id && "bin_id has to be between 0 and number_of_bins");

        auto& cart = carts_being_filled[bin_id];

        // locking the cart and inserting the query
        auto _ = std::lock_guard{cart.mutex};
        assert(cart.basket.size() < cart_max_capacity && "carts should always have at least one empty value");
        cart.basket.emplace_back(std::move(query));

        // check if cart is full
        if (cart.basket.size() == cart_max_capacity) {
            // lock full cart queue
            auto _ = std::unique_lock{filled_carts_mutex};

            // wait for enough space
            while (filled_carts.size() == max_queued_carts) {
                filled_carts_queue_ready_cv.wait(_);
            }
            filled_carts.emplace_back(bin_id, std::move(cart.basket));
            cart.basket.reserve(cart_max_capacity);
            filled_carts_process_ready_cv.notify_one();
        }
    }

    // Take a cart from the filled_carts list - thread safe
    auto dequeue() -> std::optional<std::tuple<int ,cart>> {
        auto g = std::unique_lock{filled_carts_mutex};

        // if no cart is available, wait
        while (filled_carts.empty() and !finishing) {
            filled_carts_process_ready_cv.wait(g);
        }
        if (finishing and filled_carts.empty()) return std::nullopt;

        // move cart from queue and return it
        auto v = std::move(filled_carts.back());
        filled_carts.pop_back();
        filled_carts_queue_ready_cv.notify_one();
        return {std::move(v)};
    }

    // flushes all partially filled carts to the filled_carts list - thread safe
    void finish() {
        for (size_t bin_id{0}; bin_id < carts_being_filled.size(); ++bin_id) {
            auto& cart = carts_being_filled[bin_id];
            auto g1 = std::unique_lock{cart.mutex};
            auto g2 = std::unique_lock{filled_carts_mutex};
            filled_carts.emplace_back(bin_id, std::move(cart.basket));
            cart.basket.reserve(cart_max_capacity);
            filled_carts_process_ready_cv.notify_one();
        }

        auto g2 = std::unique_lock{filled_carts_mutex};
        finishing = true;
        filled_carts_process_ready_cv.notify_all();
    }
};

void print_queue_carts(std::vector<cart_queue::secured_cart> const& cart_queue)
{
    std::cout << "\t\t\tCARTS\n";
    std::cout << "Bin ID\tQueries\n";
    for (size_t bin_id{0}; bin_id < cart_queue.size(); ++bin_id) {
        auto const& cart = cart_queue[bin_id];
        std::cout << bin_id << '\t';

        for (auto & query : cart.basket)
            std::cout << query << '\t';

        std::cout << '\n';
    }
}
