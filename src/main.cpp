#include "cart_queue.hpp"

#include <random>
#include <thread>
#include <iostream>

int main(int /*argc*/, char const* const* /*argv*/)
{
    ///////////////////////////////////////
    // INPUT PARAMETERS
    ///////////////////////////////////////

    // Query result simulation parameters
    size_t nr_reads{100};
    size_t nr_bins{16};
    size_t max_hits_per_read{4};

    // Cart production parameters
    size_t cart_capacity = 4;
    size_t max_carts_queued = 4;

    // Threads
    size_t producerThreadsCt = 4;
    size_t consumerThreadsCt = 4;

    // Some statistics for debugging
    std::atomic_int countProduced{};
    std::atomic_int countConsumed{};

    auto queue = cart_queue<std::string>{nr_bins, cart_capacity, max_carts_queued};

    auto producers = std::vector<std::jthread>{};
    // spawns a few worker threads that produce data
    for (size_t i{0}; i < producerThreadsCt; ++i) {
        producers.emplace_back([&]() {
            //!TODO actually running different valik searches here
            std::mt19937 gen{}; // seed generator
            std::uniform_int_distribution<> bin_distr(0, nr_bins - 1); // define the range
            std::uniform_int_distribution<> per_read_distr(0, max_hits_per_read);

            for(size_t read_id=0; read_id<nr_reads; read_id++)
            {
                auto read_str = std::to_string(read_id);
                for (int i = 0; i < per_read_distr(gen); i++)
                {
                    auto bin_id = bin_distr(gen);
                    // This would probably be called in the local_prefilter callback
                    queue.insert(bin_id, read_str);
                    ++countProduced;
                }
            }
        });
    }

    auto consumers = std::vector<std::jthread>{};
    // spawns a few worker threads that consume the data
    for (size_t i{0}; i < consumerThreadsCt; ++i) {
        consumers.emplace_back([&]() {
            auto next = queue.dequeue();
            while (next) {
                // !TODO what should be done with the full cart?
                // write *next to file
                // call system("stellar.....");
                auto const& [bin_id, values] = *next;
                for ([[maybe_unused]] auto const& v : values) {
                    ++countConsumed;
                }

                // dequeue next cart
                next = queue.dequeue();
            }
        });
    }
    producers.clear(); // Wait until all producers have run
    queue.finish(); // Flush carts that are not empty yet
    consumers.clear(); // join all threads/waiting till they finish
    std::cout << "produced: " << countProduced << "\n";
    std::cout << "consumed: " << countConsumed << "\n";
}
