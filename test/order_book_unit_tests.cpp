#include "pch.h"
#include "OrderBook.h"
#include <memory>


namespace googletest = ::testing;

class OrderBookUnitTests : public googletest::Test {
protected:
    void SetUp() override {
        orderBook = std::make_unique<OrderBook>();
    }

    void TearDown() override {
        orderBook.reset();
    }

    std::unique_ptr<OrderBook> orderBook;

    // Helper methods to create orders
    OrderPointer createOrder(OrderType type, Side side, Price price, Quantity quantity) {
        return std::make_shared<Order>(type, nextOrderId_++, side, price, quantity);
    }

    OrderPointer createMarketOrder(Side side, Quantity quantity) {
        return std::make_shared<Order>(nextOrderId_++, side, quantity);
    }

    OrderPointer createOrderWithId(OrderType type, OrderId id, Side side, Price price, Quantity quantity) {
        return std::make_shared<Order>(type, id, side, price, quantity);
    }

private:
    OrderId nextOrderId_ = 1;
};

// ============================================================================
// ORDER ADDITION TESTS
// ============================================================================

TEST_F(OrderBookUnitTests, AddSingleBuyOrder) {
    auto order = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 50);
    auto trades = orderBook->addOrder(order);

    EXPECT_EQ(orderBook->Size(), 1);
    EXPECT_EQ(trades.size(), 0); // No matching

    auto infos = orderBook->GetOrderInfos();
    EXPECT_EQ(infos.GetBids().size(), 1);
    EXPECT_EQ(infos.GetAsks().size(), 0);
    EXPECT_EQ(infos.GetBids()[0].price_, 100);
    EXPECT_EQ(infos.GetBids()[0].quantity_, 50);
}

TEST_F(OrderBookUnitTests, AddSingleSellOrder) {
    auto order = createOrder(OrderType::GoodTillCancel, Side::Sell, 200, 30);
    auto trades = orderBook->addOrder(order);

    EXPECT_EQ(orderBook->Size(), 1);
    EXPECT_EQ(trades.size(), 0); // No matching

    auto infos = orderBook->GetOrderInfos();
    EXPECT_EQ(infos.GetBids().size(), 0);
    EXPECT_EQ(infos.GetAsks().size(), 1);
    EXPECT_EQ(infos.GetAsks()[0].price_, 200);
    EXPECT_EQ(infos.GetAsks()[0].quantity_, 30);
}

// ============================================================================
// ORDER MATCHING TESTS
// ============================================================================

TEST_F(OrderBookUnitTests, SimpleMatchingBuySell) {
    auto buyOrder = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 50);
    auto sellOrder = createOrder(OrderType::GoodTillCancel, Side::Sell, 100, 50);

    orderBook->addOrder(buyOrder);
    auto trades = orderBook->addOrder(sellOrder);

    EXPECT_EQ(orderBook->Size(), 0); // Both orders filled
    EXPECT_EQ(trades.size(), 1);

    auto infos = orderBook->GetOrderInfos();
    EXPECT_EQ(infos.GetBids().size(), 0);
    EXPECT_EQ(infos.GetAsks().size(), 0);
}

TEST_F(OrderBookUnitTests, PartialMatching) {
    auto buyOrder = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 100);
    auto sellOrder = createOrder(OrderType::GoodTillCancel, Side::Sell, 100, 60);

    orderBook->addOrder(buyOrder);
    auto trades = orderBook->addOrder(sellOrder);

    EXPECT_EQ(orderBook->Size(), 1); // Buy order partially filled
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(buyOrder->getRemainingQuantity(), 40);
    EXPECT_EQ(sellOrder->getRemainingQuantity(), 0); // Sell order fully filled

    auto infos = orderBook->GetOrderInfos();
    EXPECT_EQ(infos.GetBids().size(), 1);
    EXPECT_EQ(infos.GetBids()[0].quantity_, 40);
    EXPECT_EQ(infos.GetAsks().size(), 0);
}

TEST_F(OrderBookUnitTests, PriceTimePriority) {
    // Add orders in different order to test price-time priority
    auto buyOrder1 = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 50);
    auto buyOrder2 = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 30);
    auto sellOrder = createOrder(OrderType::GoodTillCancel, Side::Sell, 100, 60);

    orderBook->addOrder(buyOrder1);
    orderBook->addOrder(buyOrder2);
    auto trades = orderBook->addOrder(sellOrder);

    EXPECT_EQ(orderBook->Size(), 1); // buyOrder2 partially filled
    EXPECT_EQ(trades.size(), 2);
    EXPECT_EQ(buyOrder1->getRemainingQuantity(), 0); // First order fully filled
    EXPECT_EQ(buyOrder2->getRemainingQuantity(), 20); // Second order partially filled
}

// ============================================================================
// ORDER TYPE TESTS
// ============================================================================

TEST_F(OrderBookUnitTests, FillAndKillOrderMatching) {
    auto buyOrder = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 50);
    auto sellOrder = createOrder(OrderType::FillAndKill, Side::Sell, 100, 50);
    
    orderBook->addOrder(buyOrder);
    auto trades = orderBook->addOrder(sellOrder);
    
    EXPECT_EQ(orderBook->Size(), 0);
    EXPECT_EQ(trades.size(), 1);
}

// ============================================================================
// ORDER CANCELLATION TESTS
// ============================================================================

TEST_F(OrderBookUnitTests, CancelExistingOrder) {
    auto order = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 50);
    orderBook->addOrder(order);
    
    EXPECT_EQ(orderBook->Size(), 1);
    
    orderBook->CancelOrder(order->getOrderId());
    EXPECT_EQ(orderBook->Size(), 0);
    
    auto infos = orderBook->GetOrderInfos();
    EXPECT_EQ(infos.GetBids().size(), 0);
}

// ============================================================================
// ORDER MODIFICATION TESTS
// ============================================================================

TEST_F(OrderBookUnitTests, ModifyExistingOrder) {
    auto order = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 50);
    orderBook->addOrder(order);
    
    OrderModify modify(order->getOrderId(), Side::Buy, 90, 40);
    auto trades = orderBook->ModifyOrder(modify);
    
    EXPECT_EQ(orderBook->Size(), 1);
    
    auto infos = orderBook->GetOrderInfos();
    EXPECT_EQ(infos.GetBids().size(), 1);
    EXPECT_EQ(infos.GetBids()[0].price_, 90);
    EXPECT_EQ(infos.GetBids()[0].quantity_, 40);
}

// ============================================================================
// COMPLEX SCENARIOS
// ============================================================================

TEST_F(OrderBookUnitTests, MultiplePriceLevels) {
    // Create multiple price levels
    auto buy1 = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 50);
    auto buy2 = createOrder(OrderType::GoodTillCancel, Side::Buy, 90, 30);
    auto buy3 = createOrder(OrderType::GoodTillCancel, Side::Buy, 80, 20);
    
    auto sell1 = createOrder(OrderType::GoodTillCancel, Side::Sell, 200, 40);
    auto sell2 = createOrder(OrderType::GoodTillCancel, Side::Sell, 210, 60);
    
    orderBook->addOrder(buy1);
    orderBook->addOrder(buy2);
    orderBook->addOrder(buy3);
    orderBook->addOrder(sell1);
    orderBook->addOrder(sell2);
    
    EXPECT_EQ(orderBook->Size(), 5);
    
    auto infos = orderBook->GetOrderInfos();
    EXPECT_EQ(infos.GetBids().size(), 3);
    EXPECT_EQ(infos.GetAsks().size(), 2);
    
    // Verify bid prices are in descending order
    EXPECT_EQ(infos.GetBids()[0].price_, 100);
    EXPECT_EQ(infos.GetBids()[1].price_, 90);
    EXPECT_EQ(infos.GetBids()[2].price_, 80);
    
    // Verify ask prices are in ascending order
    EXPECT_EQ(infos.GetAsks()[0].price_, 200);
    EXPECT_EQ(infos.GetAsks()[1].price_, 210);
}


// ============================================================================
// TRADE VERIFICATION TESTS
// ============================================================================

TEST_F(OrderBookUnitTests, TradeExecutionDetails) {
    auto buyOrder = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 50);
    auto sellOrder = createOrder(OrderType::GoodTillCancel, Side::Sell, 100, 50);
    
    orderBook->addOrder(buyOrder);
    auto trades = orderBook->addOrder(sellOrder);
    
    EXPECT_EQ(trades.size(), 1);
    
    const auto& trade = trades[0];
    EXPECT_EQ(trade.getBitTrade().orderId_, buyOrder->getOrderId());
    EXPECT_EQ(trade.getAskTrade().orderId_, sellOrder->getOrderId());
    EXPECT_EQ(trade.getBitTrade().price_, 100);
    EXPECT_EQ(trade.getAskTrade().price_, 100);
    EXPECT_EQ(trade.getBitTrade().quantity_, 50);
    EXPECT_EQ(trade.getAskTrade().quantity_, 50);
}

TEST_F(OrderBookUnitTests, PartialTradeExecutionDetails) {
    auto buyOrder = createOrder(OrderType::GoodTillCancel, Side::Buy, 100, 100);
    auto sellOrder = createOrder(OrderType::GoodTillCancel, Side::Sell, 100, 60);
    
    orderBook->addOrder(buyOrder);
    auto trades = orderBook->addOrder(sellOrder);
    
    EXPECT_EQ(trades.size(), 1);
    
    const auto& trade = trades[0];
    EXPECT_EQ(trade.getBitTrade().quantity_, 60);
    EXPECT_EQ(trade.getAskTrade().quantity_, 60);
}
