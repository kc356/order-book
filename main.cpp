//
// Created by Kin on 8/13/2025.
//

#include <iostream>
#include <memory>
#include "Order.h"
#include "OrderBook.h"
#include "OrderType.h"
#include "Side.h"
#include "Usings.h"


int main() {
    OrderBook orderBook;
    const OrderId orderId = 1;
    orderBook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderId, Side::Buy, 100, 10));
    std::cout << orderBook.Size() << std::endl;
    orderBook.CancelOrder(orderId);
    std::cout << orderBook.Size() << std::endl;
    return 0;
}