#include "OrderBook.h"
#include "RingBuffer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>
#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <x86intrin.h>
#else
#error "Unsupported compiler for __rdtsc"
#endif
using namespace std::chrono;

struct RawOrder {
    uint64_t id;
    Side side;
    uint32_t price;
    uint32_t quantity;
};

int main() {
    std::cout << "=======================================\n";
    std::cout << "  LITHIUM MATCHER - QUANT BENCHMARK    \n";
    std::cout << "=======================================\n\n";

    // ---------------------------------------------------------
    // STAGE 1: File I/O (Disk to RAM - No Parsing)
    // ---------------------------------------------------------
    std::cout << "[Stage 1] Measuring pure Disk I/O...\n";
    std::vector<std::string> rawLines;
    rawLines.reserve(1000000);

    auto ioStart = steady_clock::now();
    std::ifstream file("orders.csv");
    if (!file.is_open()) {
        std::cout << "ERROR: Could not open orders.csv!\n";
        return 1;
    }

    std::string line;
    std::getline(file, line); // Skip header
    while (std::getline(file, line)) {
        rawLines.push_back(line);
    }
    auto ioEnd = steady_clock::now();
    double ioTimeMs = duration<double, std::milli>(ioEnd - ioStart).count();
    std::cout << "  -> Disk I/O Time: " << ioTimeMs << " ms\n\n";

    // ---------------------------------------------------------
    // STAGE 2: Parsing (String to Struct)
    // ---------------------------------------------------------
    std::cout << "[Stage 2] Measuring String Parsing...\n";
    std::vector<RawOrder> parsedOrders;
    parsedOrders.reserve(rawLines.size());

    auto parseStart = steady_clock::now();
    for (const auto& rawLine : rawLines) {
        std::stringstream ss(rawLine);
        std::string token;
        RawOrder order;

        std::getline(ss, token, ',');
        order.id = std::stoull(token);

        std::getline(ss, token, ',');
        order.side = (token == "0") ? Side::BUY : Side::SELL;

        std::getline(ss, token, ',');
        order.price = std::stoul(token);

        std::getline(ss, token, ',');
        order.quantity = std::stoul(token);

        parsedOrders.push_back(order);
    }
    auto parseEnd = steady_clock::now();
    double parseTimeMs = duration<double, std::milli>(parseEnd - parseStart).count();
    std::cout << "  -> Parsing Time: " << parseTimeMs << " ms\n\n";

    // ---------------------------------------------------------
    // STAGE 3: Memory Allocation Microbenchmark (new vs Pool)
    // ---------------------------------------------------------
    std::cout << "[Stage 3] Microbenchmark: Memory Pool vs 'new'...\n";
    int allocCount = 1000000;
    
    std::vector<Order*> systemPointers(allocCount);
    auto newStart = steady_clock::now();
    for(int i = 0; i < allocCount; i++) systemPointers[i] = new Order();
    for(int i = 0; i < allocCount; i++) delete systemPointers[i];
    auto newEnd = steady_clock::now();
    double newTimeMs = duration<double, std::milli>(newEnd - newStart).count();

    MemoryPool<Order> benchmarkPool(allocCount);
    std::vector<Order*> poolPointers(allocCount);
    auto poolStart = steady_clock::now();
    for(int i = 0; i < allocCount; i++) poolPointers[i] = benchmarkPool.allocate();
    for(int i = 0; i < allocCount; i++) benchmarkPool.deallocate(poolPointers[i]);
    auto poolEnd = steady_clock::now();
    double poolTimeMs = duration<double, std::milli>(poolEnd - poolStart).count();

    std::cout << "  -> OS 'new/delete' Time: " << newTimeMs << " ms\n";
    std::cout << "  -> Custom Memory Pool Time: " << poolTimeMs << " ms\n";
    std::cout << "  -> Memory Pool is " << (newTimeMs / poolTimeMs) << "x faster.\n\n";

    // ---------------------------------------------------------
    // STAGE 4: Engine Latency (True Per-Order hardware cycle counter)
    // ---------------------------------------------------------
    std::cout << "[Stage 4] Measuring Core Matching Engine Latency (via __rdtsc)...\n";
    
    // 1. Calibrate the CPU Cycle Counter
    // We need to figure out exactly how many nanoseconds one CPU cycle takes on your specific machine.
    std::cout << "  -> Calibrating CPU cycle counter...\n";
    auto calibStartT = steady_clock::now();
    uint64_t calibStartC = __rdtsc();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Sleep for 50ms
    
    uint64_t calibEndC = __rdtsc();
    auto calibEndT = steady_clock::now();
    
    double nsPerCycle = (double)duration_cast<nanoseconds>(calibEndT - calibStartT).count() / (calibEndC - calibStartC);
    std::cout << "  -> Calibration: 1 CPU Cycle = " << nsPerCycle << " nanoseconds.\n\n";

    OrderBook* isolatedEngine = new OrderBook();
    std::vector<double> latenciesNs; 
    latenciesNs.reserve(parsedOrders.size());

    // 2. The True Microbenchmark
    // We surround the processOrder function with raw hardware cycle reads
    for (const auto& order : parsedOrders) {
        uint64_t startCycle = __rdtsc();
        
        isolatedEngine->processOrder(order.id, order.side, order.price, order.quantity);
        
        uint64_t endCycle = __rdtsc();
        
        // Convert the cycles taken into exact nanoseconds
        latenciesNs.push_back((endCycle - startCycle) * nsPerCycle);
    }

    // 3. Calculate Percentiles safely
    std::sort(latenciesNs.begin(), latenciesNs.end());
    double p50 = latenciesNs[latenciesNs.size() * 0.50];
    double p99 = latenciesNs[latenciesNs.size() * 0.99];
    double p99_9 = latenciesNs[latenciesNs.size() * 0.999];

    std::cout << "--- Latency Percentiles (Per Order) ---\n";
    std::cout << "  -> Median (p50): " << p50 << " nanoseconds\n";
    std::cout << "  -> 99th % (p99): " << p99 << " nanoseconds\n";
    std::cout << "  -> 99.9th (p99.9): " << p99_9 << " nanoseconds\n\n";

    // ---------------------------------------------------------
    // STAGE 5: Full Multithreaded Pipeline (The Real Deal)
    // ---------------------------------------------------------
    std::cout << "[Stage 5] Measuring Full Multithreaded Pipeline...\n";
    
    // Here are your threads and Ring Buffer back in action!
    OrderBook* threadedEngine = new OrderBook();
    RingBuffer<RawOrder, 1024> conveyorBelt;
    std::atomic<bool> isFinished = false;

    auto threadStart = steady_clock::now();

    // The Auctioneer Thread
    std::thread consumerThread([&]() {
        RawOrder incoming;
        int processedCount = 0;
        
        while (!isFinished.load(std::memory_order_acquire)) {
            while (conveyorBelt.pop(incoming)) {
                threadedEngine->processOrder(incoming.id, incoming.side, incoming.price, incoming.quantity);
                processedCount++;
            }
        }
        
        while(conveyorBelt.pop(incoming)) {
            threadedEngine->processOrder(incoming.id, incoming.side, incoming.price, incoming.quantity);
            processedCount++;
        }
    });

    // The Mailroom Thread
    std::thread producerThread([&]() {
        for (const auto& order : parsedOrders) {
            while (!conveyorBelt.push(order)) {
                // Spin if belt is full
            }
        }
        isFinished.store(true, std::memory_order_release);
    });

    producerThread.join();
    consumerThread.join();

    auto threadEnd = steady_clock::now();
    double threadTimeMs = duration<double, std::milli>(threadEnd - threadStart).count();

    std::cout << "  -> Multithreaded Time: " << threadTimeMs << " ms\n";
    std::cout << "  -> TRUE Multithreaded Throughput: " << (parsedOrders.size() / (threadTimeMs / 1000.0)) << " orders/second\n";
    std::cout << "=======================================\n";

    return 0;
}