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
#include <array>
#include <atomic>

// This is the " Lock Free Conveyer belt ".
// IT is generic and can pass any type T , and has a fixed 'Size' .
template <typename T , size_t Size>
class RingBuffer {
    private:
        std::array<T,Size> buffer;
        //The Trackers
        alignas(64) std::atomic<size_t> head{0}; // Read Index (Consumer/Auctioneer)
        alignas(64) std::atomic<size_t> tail{0}; // Write Index (Producer/Mailroom)
        // allows multiple threads to read/write safely without using a mutex.
        //'alignas(64)' is an advance HFT trick to prevent "False Sharing" in CPU caches.
        //atomic guarantees the multi threading

    public:
        // Called only by the Producer
        bool push(const T& item) {
            size_t currentTail = tail.load(std::memory_order_relaxed);
            // We only need the latest value. so no synchronization needed ! Relaxed is the fastest atomic load.
            size_t nextTail = currentTail + 1 ;

            // If the next write position hits the current read position, the belt is full!
            // We use '% Size' (modulo) to wrap the index back to 0 when it reaches the end.
            if (nextTail - head.load(std::memory_order_acquire) > Size) {
                return false; // truly full — Size items already in buffer
            }

            //Drop the Item into conveyer belt
            buffer[currentTail % Size] = item;
            
            // 'Publish' the new tail position so the Consumer instantly sees it
            tail.store(nextTail, std::memory_order_release);
            return true;
        }

        bool pop(T& item){
            // Find out where we are currently reading
            size_t currentHead = head.load(std::memory_order_relaxed);

            // If the read position is exactly at the write position, the belt is empty!
            if(currentHead == tail.load(std::memory_order_acquire)) {
                return false; // Queue is compeletely empty
            }

            // Grab item off the conveyer belt
            item = buffer[currentHead % Size];

            // 'Publish' the new head position so the Producer knows this slot is empty again
            head.store(currentHead + 1, std::memory_order_release);
            return true;
        }
};