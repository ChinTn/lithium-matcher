#pragma once
#include <vector>
#include <stdexcept>


// 'template <typename T>' means this pool is generic. 
// We can use it to store Orders, Trades, or anything else without rewriting code.

template <typename T>
class MemoryPool {
    private:
        std::vector<T> pool;  // The massive notebook of pre-allocated objects
        std::vector<size_t> freeList;  // The stack of empty page numbers (indices) we can reuse
        // size_t is a C++ type used specifically for sizes and array indices.
    public:
        MemoryPool(size_t initialCapacity){
            // 1. Buy the massive notebook. This reserves the memory from the OS.
            pool.resize(initialCapacity);

            // 2. Add all the page numbers to the Free List stack.
            // We do it backwards so index 0 is at the top of the stack and gets used first.
            //since we are considering here that in vector it is same as stack , last in first out 
            freeList.reserve(initialCapacity);
            //we tell the OS to reserve the space immediately to make the for loop run faster.
            for(size_t i = initialCapacity ; i > 0 ; --i) {
                freeList.push_back(i - 1);
            }
        }

        // Grab a blank object from the pool. (Notice NO 'new' keyword!)
        T* allocate() {
            // Defense Mechanism: If the pool is empty, we must NEVER resize!
            // Resizing a vector invalidates all existing pointers to its elements.
            if(freeList.empty()) {
                // Return a nullptr so the caller (OrderBook) knows we are out of memory.
                // This will trigger the safety check we built in OrderBook::processOrder!
                return nullptr;
            }

            // Take the top page number off the free list stack
            size_t index = freeList.back();
            freeList.pop_back();

            // Return a pointer to that specific blank object in the array
            return &pool[index];
        }

        //When an Order matches , we recycle it instead of using 'delete'
        void deallocate(T* object){
            // Pointer Math Magic: We subtract the memory address of the start of the array 
            // from the memory address of our object. This instantly gives us its index (page number).

            size_t index = object - &pool[0];

            //add that pagenumber into the free list again
            freeList.push_back(index);

        }
};