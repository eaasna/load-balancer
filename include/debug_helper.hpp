#pragma once

#include <unordered_map>

#include <seqan3/core/debug_stream.hpp>

#include "cart.hpp"

void print_queue_carts(std::unordered_map<size_t, cart> cart_queue);
