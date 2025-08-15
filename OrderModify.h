//
// Created by Kin on 8/15/2025.
//
#pragma once

#include <memory>
#include "Order.h"
#include "OrderType.h"
#include "Side.h"
#include "Usings.h"

using OrderPointer = std::shared_ptr<Order>;
// TODO: make list of pointer into vector
using OrderPointers = std::list<OrderPointer>;

class OrderModify {
public:
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity): orderId_{ orderId }, price_{ price }, side_{ side }, quantity_{ quantity } {};

    OrderId GetOrderId() const { return orderId_;}
    Price GetPrice() const { return price_;}
    Side GetSide() const { return side_;}
    Quantity GetQuantity() const { return quantity_;}

    OrderPointer ToOrderPointer(OrderType type) const {
        return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity());
    }

private:
    OrderId orderId_;
    Price price_;
    Side side_;
    Quantity quantity_;
};
