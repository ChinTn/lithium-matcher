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
#include <unordered_map>
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


        // 4. The Lookup Table (The Phone Book)
        // Maps OrderID -> pointer to the resting Order.
        // This lets us find any order in O(1) when someone wants to cancel it.
        std::unordered_map<uint64_t, Order*> orderMap;

        // --- Helper Functions ---
        // These handle the annoying pointer logic of adding/removing people from the line

        void addOrderToLevel(PriceLevel& level, Order* order);
        void removeOrderFromLevel(PriceLevel& level, Order* order);

        //Core Matching Logic
        void matchOrder(Order* incomingOrder);
    
    public:
        //constructor
        OrderBook();

        // Overloaded constructor for fast testing
        OrderBook(size_t poolSize);

        // === Test Inspection Methods ===
        uint32_t getBestBid() const { return currentBestBid; }
        uint32_t getBestAsk() const { return currentBestAsk; }

        const Order* getFirstOrderAtPrice(Side side, uint32_t price) const {
            if(price >= MAX_PRICE) return nullptr;
            if(side == Side::BUY) return bids[price].firstInLine;
            return asks[price].firstInLine;
        }

        uint32_t getTotalQuantityAtPrice(Side side, uint32_t price) const {
            if(price >= MAX_PRICE) return 0;
            const PriceLevel& level = (side == Side::BUY) ? bids[price] : asks[price];
            uint32_t total = 0;
            const Order* current = level.firstInLine;
            while(current != nullptr) {
                total += current->quantity;
                current = current->next;
            }
            return total;
        }

        //The main entry point for the outside world
        // The Mailroom thread calls this function to give us a new order.
        void processOrder(uint64_t orderId, Side side, uint32_t price, uint32_t quantity);

        // Cancel the order
        bool cancelOrder(uint64_t orderId);

        //returns number of the orders in the book
        size_t getRestingOrderCount() const { return orderMap.size(); }

};