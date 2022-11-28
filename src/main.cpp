#include "cart_queue.hpp"
#include "simulate_thread_result.hpp"

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

    //INPUT PARAMETERS

    auto valik_thread_result = generate_thread_result<std::vector<valik::query_result>>(nr_reads,
                                                                                        nr_bins,
                                                                                        max_hits_per_read);
    // valik thread results are (read, [bins])
    print_thread_result(valik_thread_result);
    // carts should be (bin, [reads])

    cart_queue queue{nr_bins, cart_capacity};


    for (auto & query_result : valik_thread_result)
    {
        for (auto & bin_id : query_result.get_hits())
        {
            queue.insert(bin_id, query_result.get_id());
        }
    }
    print_queue_carts(queue.carts_being_filled);
    queue.flush();
    print_queue_carts(queue.carts_being_filled);
}
