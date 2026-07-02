/*
 * Copyright (c) 2026 Chintan. All rights reserved.
 * 
 * This software is the confidential and proprietary information of Chintan.
 * You shall not disclose such Confidential Information and shall use it only in 
 * accordance with the terms of the license agreement you entered into with Chintan.
 * 
 * Unauthorized copying of this file, via any medium, is strictly prohibited.
 */

#pragma once
#include "Types.h"
#include "MemoryPool.h"
#include <array>

// We define a maximum price to size our arrays. 
// 100,000 cents = $1,000.00
constexpr uint32_t MAX_PRICE = 100000;

class OrderBook {
    private:
        // 1. The Storage (The Parking Lot)
        // We instantiate our MemoryPool specifically to hold 'Order' objects.
        // We will initialize it with 1,000,000 slots in the .cpp file.
        MemoryPool<Order> orderPool;

        // 2. The Organizers (The Whiteboards)
        // Arrays representing every possible price point up to $1,000.00.
        std::array<PriceLevel, MAX_PRICE> bids; // Buyers (Highest Price is the Best)
        std::array<PriceLevel, MAX_PRICE> asks; // Sellers (Lowest price is the Best)

        // 3. The Trackers (So we don't have to search the arrays)
        // These hold the exact array index of the current best prices.
        uint32_t currentBestBid;
        uint32_t currentBestAsk;

        // --- Helper Functions ---
        // These handle the annoying pointer logic of adding/removing people from the line

        void addOrderToLevel(PriceLevel& level, Order* order);
        void removeOrderFromLevel(PriceLevel& level, Order* order);

        //Core Matching Logic
        void matchOrder(Order* incomingOrder);
    
    public:
        //constructor
        OrderBook();

        //The main entry point for the outside world
        // The Mailroom thread calls this function to give us a new order.
        void processOrder(uint64_t orderId, Side side, uint32_t price, uint32_t quantity);
};