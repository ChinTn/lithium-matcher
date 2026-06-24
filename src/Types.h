#pragma once
// When you build a C++ project, files often reference each other. This line tells the compiler: "If you have already read this file, do not read it again." It prevents annoying "Duplicate Definition" errors.
#include <cstdint>

//Used to differ between Buyers(Bids) and Sellers (Asks)
enum class Side {
    BUY,
    SELL
};

// The core data structure representing a single trade request.
// Designed to be as small as possible to fit neatly in CPU cache.
struct Order {
    uint64_t orderId;
    uint32_t price; // Stored in cents (e.g. 15000 = $150.00) to avoid float rounding errors
    uint32_t quantity;
    Side side;

    // Intrusive Linked List pointers! 
    // This allows us to link Orders together in a line without allocating separate Node objects.
    Order* next = nullptr;
    Order* prev = nullptr;
};   

// Represents a single price point on our whiteboard (e.g. all orders at exactly $150.00).
// This acts as the "signpost" in our pre-allocated array.
struct PriceLevel {
    Order* firstInLine = nullptr; // Head of the queue
    Order* lastInLine = nullptr;  // Tail of the queue
};