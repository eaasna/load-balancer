#include "cart_queue.hpp"
#include "debug_helper.hpp"
#include "simulate_thread_result.hpp"

int main(int argc, char ** argv)
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

    // Cart consumption parameters
    size_t nr_carts = 2;

    //INPUT PARAMETERS

    auto valik_thread_result = generate_thread_result<std::vector<valik::query_result>>(nr_reads,
                                                                                        nr_bins,
                                                                                        max_hits_per_read);
    cart_queue queue{cart_capacity};

    for (auto & query_result : valik_thread_result)
    {
        for (auto & bin_id : query_result.get_hits())
        {
            auto insert_future = std::async(std::launch::async, &cart_queue::insert, &queue, bin_id, query_result.get_id());
            insert_future.get();
        }
    }

    //!TODO: need to gather all cart futures
    auto f1 = std::async(std::launch::async, &cart_queue::take_full_cart, &queue);
    cart c1 = f1.get();

}
