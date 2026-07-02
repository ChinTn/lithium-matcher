/*
 * Copyright (c) 2026 Chintan. All rights reserved.
 * 
 * This software is the confidential and proprietary information of Chintan.
 * You shall not disclose such Confidential Information and shall use it only in 
 * accordance with the terms of the license agreement you entered into with Chintan.
 * 
 * Unauthorized copying of this file, via any medium, is strictly prohibited.
 */

#include "OrderBook.h"
#include <algorithm>
#include <iostream>

// 1. Constructor: Setup the pool and trackers
// We initialise the memory pool with 5,000,000 slots right off the bat.
OrderBook::OrderBook() : orderPool(5000000) {
    // we start the best bid at 0 (lowest possible)
    // we start ask at MAX_PRICE (highest possible)

    currentBestBid = 0;
    currentBestAsk = MAX_PRICE;
    //putting them on their worst possible state
}

// 2. Entry Point
void OrderBook::processOrder(uint64_t orderId, Side side, uint32_t price, uint32_t quantity) {
    
    //Safety check
    //Dont allow prices higher than our array size
    if(price >= MAX_PRICE) return;

    //Grab a blank order from the memory pool and fill it
    Order* incoming = orderPool.allocate();

    // --- SAFETY CHECK IF WE RUN OUT OF MEMORY --- //
    if (incoming == nullptr) {
        // We are completely out of memory!
        // Drop the order safely instead of crashing the whole engine.
        std::cout << "WARNING: Memory Pool Full! Dropped Order ID: " << orderId << "\n";
        return; 
    }
    
    incoming->orderId = orderId;
    incoming->side = side;
    incoming->price = price;
    incoming->quantity = quantity;
    incoming->next = nullptr;
    incoming->prev = nullptr;

    // Try to match it against the opposite 
    matchOrder(incoming);
    
    //If after doing this matching and trading if we still got quantity
    //put them in List again to wait
    if(incoming -> quantity > 0){
        if(side == Side::BUY) {
            addOrderToLevel(bids[price] , incoming);
            //update the tracker if this is a new highest buyer
            if(price > currentBestBid){
                currentBestBid = price;
            }
        } else {
            addOrderToLevel(asks[price] , incoming);
            //update the tracker if this is a new lowest seller
            if(price < currentBestAsk){
                currentBestAsk = price;
            }
        }
    } else {
        
        //means it was complete
        orderPool.deallocate(incoming);
    }
}

// 3. The Matching Engine Loop
void OrderBook::matchOrder(Order* incoming) {
    if(incoming->side == Side::BUY){
        // While we still want to buy , AND there is a seller cheaper or equal to our price
        // Golden Rule : Keep looping as long as Alice wants share and there is a seller willing to sell for a price lesser or equal to ALice's maximum..
        while(incoming -> quantity > 0 && incoming -> price >= currentBestAsk){
            PriceLevel& bestLevel = asks[currentBestAsk];
            //here we must have to use & for referencing else later when we remove it wont get removed from the asks list
            Order* seller = bestLevel.firstInLine;

            // Find how many shares we can actually trade
            uint32_t tradeQty = std::min(incoming->quantity , seller->quantity);

            incoming -> quantity -= tradeQty;
            seller -> quantity -= tradeQty;

            // If seller is Out of shares
            if (seller -> quantity == 0){
                removeOrderFromLevel(bestLevel , seller);
                //we used & up there to do this
                orderPool.deallocate(seller); //Recycle the Order

                //Also now if that Price level is complety empty, find the next best seller
                if(bestLevel.firstInLine == nullptr) {
                    currentBestAsk ++;
                    //keep increasing until find the level with someone with in it
                    while (currentBestAsk < MAX_PRICE && asks[currentBestAsk].firstInLine == nullptr){
                        currentBestAsk ++;
                    }
                }
            }
        }
    } else {

        // While we still want to sell , and there is a buyer who is paying equal or higher than that !
        while (incoming -> quantity > 0 && incoming -> price <= currentBestBid) {
            PriceLevel& bestLevel = bids[currentBestBid];
            Order* buyer = bestLevel.firstInLine;

            uint32_t tradeQty = std::min(incoming -> quantity , buyer -> quantity);

            incoming -> quantity -= tradeQty;
            buyer -> quantity -= tradeQty;

            //If buyer is out of Shares and we still want to share
            if(buyer -> quantity == 0){
                removeOrderFromLevel(bestLevel, buyer);
                orderPool.deallocate(buyer); // Recycle the Order

                //If that price of bids is now empty 
                if(bestLevel.firstInLine == nullptr){
                    if(currentBestBid > 0){
                        currentBestBid--;
                        //Keep checking
                        while (currentBestBid > 0 && bids[currentBestBid].firstInLine == nullptr){
                            currentBestBid--;
                        }
                    }
                } 
            }

        }
    }
}

// 4. LinkedList Mechanics (Putting someone at the back of the line)
void OrderBook::addOrderToLevel(PriceLevel& level, Order* order) {
    if(level.firstInLine == nullptr) {
        //Line is empty , it is the first and last person
        level.firstInLine = order;
        level.lastInLine = order;
    } else {
        //Someone is already there
        level.lastInLine -> next = order;
        order -> prev = level.lastInLine;
        level.lastInLine = order;
    }
}

// 5. LinkedList Mechanics (Removing someone from the line)
void OrderBook::removeOrderFromLevel(PriceLevel& level, Order* order) {
    if(order -> prev != nullptr) {
        order -> prev -> next = order -> next;
    } else {
        level.firstInLine = order -> next;
    }

    if(order -> next != nullptr) {
        order -> next -> prev = order -> prev;
    } else {
        level.lastInLine = order -> prev;
    }
}