//
// Created by Kin on 8/15/2025.
//
#pragma once
#include <format>
#include <stdexcept>

#include "Constants.h"
#include "OrderType.h"
#include "Side.h"
#include "Usings.h"

class Order {
public:
    Order(const OrderType orderType, const OrderId orderId, const Side side, const Price price, const Quantity quantity):
        orderType_{orderType},
        orderId_{orderId},
        side_{side},
        price_{price},
        initialQuantity_{quantity},
        remainingQuantity_{quantity} { };

    // Market order
    Order(const OrderId orderId, const Side side,const Quantity quantity)
        :Order(OrderType::Market, orderId, side, Constants::InvalidPrice, quantity) {}

    OrderId getOrderId() const {return orderId_;}
    Side getSide() const { return side_;}
    Price getPrice() const {return price_;}
    OrderType getOrderType() const { return orderType_;}
    Quantity getInitialQuantity() const { return initialQuantity_;}
    Quantity getRemainingQuantity() const {return remainingQuantity_;}
    Quantity getFilledQuantity() const {return getInitialQuantity() - getRemainingQuantity();}
    bool isFilled() const {return getRemainingQuantity()==0;}
    void fill(const Quantity quantity) {
        if (quantity > getRemainingQuantity())
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", getOrderId()));
        remainingQuantity_ -= quantity;
    }
    void toGoodTillCancel(Price price) {
        if (getOrderType() != OrderType::Market)
            throw std::logic_error(std::format("Order ({}) cannot have its price adjusted, only market orders can.", getOrderId()));

        price_ = price;
        orderType_ = OrderType::GoodTillCancel;
    }

private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
};
