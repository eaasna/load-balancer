#include "cart_queue.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>

/** !\brief This class calls an external program and hands over the data as the first parameter
 *
 */
class ExternalConsumer {
    std::filesystem::path  tmpPath;
    std::string            executableName;
    inline static std::atomic_int consumerIdCt{};


public:
    ExternalConsumer()
        : tmpPath{std::filesystem::temp_directory_path() / "loadbalancer"}
        , executableName{[]() -> std::string {
            if (auto ptr = std::getenv("LOADBALANCER_CONSUMER")) {
                return ptr;
            } else {
                return "cat"; //!TODO, maybe something smarter?
            }
        }()}
    {
        //!TODO This could require some more error handling
        std::filesystem::create_directories(tmpPath);
    }
    ExternalConsumer(ExternalConsumer const&) = delete;
    ExternalConsumer(ExternalConsumer&&) = delete;

    ~ExternalConsumer() {
        std::filesystem::remove_all(tmpPath);
    }


    auto operator=(ExternalConsumer const&) -> ExternalConsumer& = delete;
    auto operator=(ExternalConsumer&&) -> ExternalConsumer& = delete;

    void processBasket(size_t bin_id, std::vector<std::tuple<std::string, std::string>> const& basket) {
        static thread_local int consumerId = ++consumerIdCt;

        auto pathName = tmpPath / ("file_" + std::to_string(consumerId) + ".fasta");

        // Write to fasta file
        auto ofs = std::ofstream{pathName};
        for (auto const& [name, seq] : basket) {
            ofs << '>' <<  name << '-' << bin_id << '\n';
            ofs << seq << '\n';
        }
        ofs.close();

        // Call consumer program
        auto call = executableName + " " + pathName.string();
        auto ret = std::system(call.c_str());
        //!TODO missing interpretation of the return value
        (void)ret;
    }
};

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

    auto queue = cart_queue<std::tuple<std::string, std::string>>{nr_bins, cart_capacity, max_carts_queued};

    auto producers = std::vector<std::jthread>{};
    // spawns a few worker threads that produce data
    for (size_t i{0}; i < producerThreadsCt; ++i) {
        producers.emplace_back([&]()
        {
            //!TODO actually running different valik searches here
            std::mt19937 gen{}; // seed generator
            std::uniform_int_distribution<> bin_distr(0, nr_bins - 1); // define the range
            std::uniform_int_distribution<> per_read_distr(0, max_hits_per_read);
            std::uniform_int_distribution<> dna4_distr(0, 4); // random values between 0-3 for A, C, G, T

            for(size_t read_id=0; read_id<nr_reads; read_id++)
            {
                auto read_id_as_str = std::to_string(read_id);

                // Generate some random read of length.....50 !TODO
                auto read_str = std::string(50, 'N'); // create string of length 50 filled with Ns
                for (auto& c : read_str)
                {
                    switch(dna4_distr(gen))
                    {
                        case 0: c = 'A'; break;
                        case 1: c = 'C'; break;
                        case 2: c = 'G'; break;
                        case 3: c = 'T'; break;
                        default: assert(false && "This should never happen");
                    }
                }
                for (int i = 0; i < per_read_distr(gen); i++)
                {
                    auto bin_id = bin_distr(gen);
                    // This would probably be called in the local_prefilter callback
                    queue.insert(bin_id, {read_id_as_str, read_str});
                    ++countProduced;
                }
            }
        });
    }

    auto externalConsumer = ExternalConsumer{};
    auto consumers = std::vector<std::jthread>{};
    // spawns a few worker threads that consume the data
    for (size_t i{0}; i < consumerThreadsCt; ++i) {
        consumers.emplace_back([&]() {
            auto next = queue.dequeue();
            while (next) {
                auto const& [bin_id, values] = *next;
                externalConsumer.processBasket(bin_id, values);
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
