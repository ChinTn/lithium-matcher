#include <gtest/gtest.h>
#include <memory>
#include "../src/OrderBook.h"

// GTest Fixture: creates a fresh OrderBook on the HEAP before each test.
// We use heap because OrderBook is 3.2 MB (too big for the 1 MB stack).
class OrderBookTest : public ::testing::Test {
protected:
    std::unique_ptr<OrderBook> book;
    void SetUp() override {
        book = std::make_unique<OrderBook>(1000);
    }
};

// TEST 1: Simple Perfect Match
TEST_F(OrderBookTest, SimplePerfectMatch) {
    book->processOrder(1, Side::SELL, 15000, 100);
    book->processOrder(2, Side::BUY, 15000, 100);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 15000), 0);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15000), 0);
}

// TEST 2: No Match (Spread Too Wide)
TEST_F(OrderBookTest, NoMatchSpreadTooWide) {
    book->processOrder(1, Side::BUY, 14000, 100);
    book->processOrder(2, Side::SELL, 16000, 200);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 14000), 100);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 16000), 200);
    EXPECT_EQ(book->getBestBid(), 14000);
    EXPECT_EQ(book->getBestAsk(), 16000);
}

// TEST 3: Partial Fill
TEST_F(OrderBookTest, PartialFill) {
    book->processOrder(1, Side::SELL, 15000, 500);
    book->processOrder(2, Side::BUY, 15000, 200);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 15000), 300);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15000), 0);
}

// TEST 4: FIFO Time Priority
TEST_F(OrderBookTest, FIFOTimePriority) {
    book->processOrder(1, Side::SELL, 15000, 100);
    book->processOrder(2, Side::SELL, 15000, 200);
    book->processOrder(3, Side::SELL, 15000, 300);
    book->processOrder(4, Side::BUY, 15000, 150);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 15000), 450);
    const Order* front = book->getFirstOrderAtPrice(Side::SELL, 15000);
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(front->orderId, 2);
    EXPECT_EQ(front->quantity, 150);
}

// TEST 5: Buyer Sweeps Multiple Ask Levels
TEST_F(OrderBookTest, BuyerSweepsMultipleLevels) {
    book->processOrder(1, Side::SELL, 15000, 100);
    book->processOrder(2, Side::SELL, 15100, 100);
    book->processOrder(3, Side::SELL, 15200, 100);
    book->processOrder(4, Side::BUY, 15200, 250);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 15000), 0);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 15100), 0);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 15200), 50);
    EXPECT_EQ(book->getBestAsk(), 15200);
}

// TEST 6: Seller Sweeps Multiple Bid Levels
TEST_F(OrderBookTest, SellerSweepsMultipleBidLevels) {
    book->processOrder(1, Side::BUY, 15000, 100);
    book->processOrder(2, Side::BUY, 15100, 100);
    book->processOrder(3, Side::BUY, 15200, 100);
    book->processOrder(4, Side::SELL, 15000, 250);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15200), 0);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15100), 0);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15000), 50);
    EXPECT_EQ(book->getBestBid(), 15000);
}

// TEST 7: Price Boundary Rejection
TEST_F(OrderBookTest, PriceBoundaryRejection) {
    book->processOrder(1, Side::BUY, 100000, 100);
    EXPECT_EQ(book->getBestBid(), 0);
}

// TEST 8: Order Rests When No Opposite Side
TEST_F(OrderBookTest, OrderRestsWhenNoOppositeSide) {
    book->processOrder(1, Side::BUY, 15000, 500);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15000), 500);
    EXPECT_EQ(book->getBestBid(), 15000);
    EXPECT_EQ(book->getBestAsk(), MAX_PRICE);
}

// TEST 9: Multiple Orders Same Price Accumulate
TEST_F(OrderBookTest, MultipleOrdersSamePriceAccumulate) {
    for(int i = 1; i <= 5; i++)
        book->processOrder(i, Side::BUY, 15000, 100);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15000), 500);
}

// TEST 10: Best Bid/Ask Tracker Updates
TEST_F(OrderBookTest, BestBidAskTrackerUpdates) {
    book->processOrder(1, Side::BUY, 14000, 100);
    book->processOrder(2, Side::BUY, 14500, 100);
    book->processOrder(3, Side::BUY, 15000, 100);
    EXPECT_EQ(book->getBestBid(), 15000);
    book->processOrder(4, Side::SELL, 16000, 100);
    book->processOrder(5, Side::SELL, 15500, 100);
    book->processOrder(6, Side::SELL, 15100, 100);
    EXPECT_EQ(book->getBestAsk(), 15100);
}

// ============================================================
// TEST 11: The $0.00 Bug (Missing hasBids flag)
// ============================================================
// Scenario: A seller places an order at $0.00 (Market Order simulation).
// Because currentBestBid starts at 0, the engine will think there is 
// a buyer at $0.00 and try to match them, reading a null pointer!
TEST_F(OrderBookTest, ZeroPriceBug) {
    // This will likely crash or fail because it will try to trade with 
    // bids[0] even though no buyer actually placed an order at $0.00.
    book->processOrder(1, Side::SELL, 0, 100);

    // If it didn't crash, the seller should just be resting on the book.
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 0), 100);
    
    // And no shares should have mysteriously traded
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 0), 0);
}

// ============================================================
// TEST 12: Zero Quantity Order
// ============================================================
// Scenario: Someone accidentally (or maliciously) sends an order
// with a quantity of 0. The engine should instantly reject it.
TEST_F(OrderBookTest, ZeroQuantityRejection) {
    book->processOrder(1, Side::BUY, 15000, 0);

    // The order should NOT be resting on the book.
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15000), 0);
    
    // The best bid should still be 0 (untouched).
    EXPECT_EQ(book->getBestBid(), 0);
}

// ============================================================
// TEST 13: Simple Cancel
// ============================================================
// Place an order, cancel it, verify the book is empty.
TEST_F(OrderBookTest, SimpleCancelOrder) {
    book->processOrder(1, Side::BUY, 15000, 100);
    
    // Order should be resting
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15000), 100);
    EXPECT_EQ(book->getRestingOrderCount(), 1);
    
    // Cancel it
    bool result = book->cancelOrder(1);
    
    // Should succeed and book should be empty
    EXPECT_TRUE(result);
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::BUY, 15000), 0);
    EXPECT_EQ(book->getRestingOrderCount(), 0);
}

// ============================================================
// TEST 14: Cancel Non-Existent Order
// ============================================================
// Try to cancel an order that was never placed. Should return false.
TEST_F(OrderBookTest, CancelNonExistentOrder) {
    bool result = book->cancelOrder(999);
    EXPECT_FALSE(result);
}

// ============================================================
// TEST 15: Cancel Middle of Queue (FIFO Integrity)
// ============================================================
// Three orders at the same price. Cancel the MIDDLE one.
// The first and last should still be linked correctly.
TEST_F(OrderBookTest, CancelMiddleOfQueue) {
    book->processOrder(1, Side::SELL, 15000, 100);  // Alice
    book->processOrder(2, Side::SELL, 15000, 200);  // Bob
    book->processOrder(3, Side::SELL, 15000, 300);  // Charlie
    
    // Cancel Bob (the middle person)
    bool result = book->cancelOrder(2);
    EXPECT_TRUE(result);
    
    // Total should be 100 + 300 = 400 (Bob's 200 is gone)
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 15000), 400);
    
    // First in line should still be Alice
    const Order* front = book->getFirstOrderAtPrice(Side::SELL, 15000);
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(front->orderId, 1);
    
    // Alice's next should be Charlie (Bob is gone)
    ASSERT_NE(front->next, nullptr);
    EXPECT_EQ(front->next->orderId, 3);
}

// ============================================================
// TEST 16: Cancel Updates Best Bid Tracker
// ============================================================
// Place bids at $150 and $145. Cancel the $150 bid.
// Best bid should drop to $145.
TEST_F(OrderBookTest, CancelUpdatesBestBid) {
    book->processOrder(1, Side::BUY, 14500, 100);  // $145
    book->processOrder(2, Side::BUY, 15000, 100);  // $150
    
    EXPECT_EQ(book->getBestBid(), 15000);
    
    // Cancel the $150 bid
    book->cancelOrder(2);
    
    // Best bid should fall back to $145
    EXPECT_EQ(book->getBestBid(), 14500);
}

// ============================================================
// TEST 17: Cancel Then Match
// ============================================================
// Place two sellers. Cancel the first one. A buyer should match
// with the second seller, not crash on the cancelled one.
TEST_F(OrderBookTest, CancelThenMatch) {
    book->processOrder(1, Side::SELL, 15000, 100);  // Alice
    book->processOrder(2, Side::SELL, 15000, 200);  // Bob
    
    // Cancel Alice
    book->cancelOrder(1);
    
    // A buyer comes in wanting 150 shares
    book->processOrder(3, Side::BUY, 15000, 150);
    
    // Bob had 200, buyer took 150, so Bob should have 50 left
    EXPECT_EQ(book->getTotalQuantityAtPrice(Side::SELL, 15000), 50);
    
    // Front of line should be Bob with 50
    const Order* front = book->getFirstOrderAtPrice(Side::SELL, 15000);
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(front->orderId, 2);
    EXPECT_EQ(front->quantity, 50);
}