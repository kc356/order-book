//
// Created by Kin on 8/15/2025.
//
#pragma once

#include <map>
#include <numeric>
#include <unordered_map>
#include <fstream> // Added for file loading
#include <sstream> // Added for string parsing
#include <iostream> // Added for console output

#include "OrderBookLevelInfos.h"
#include "OrderModify.h"
#include "Trade.h"

using Trades = std::pmr::vector<Trade>;
using OrderPointer = std::shared_ptr<Order>;
// TODO: make list of pointer into vector
using OrderPointers = std::list<OrderPointer>;

class OrderBook {
private:
    struct OrderEntry {
        OrderPointer order_ {nullptr};
        OrderPointers::iterator location_;
    };

    std::map<Price, OrderPointers, std::greater<>> bids_;
    std::map<Price, OrderPointers, std::less<>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;
    OrderId nextOrderId_ = 1;  // Add this for preloading

    bool CanMatch(const Side side, const Price price) const {
        if (side == Side::Buy) {
            if (asks_.empty())
                return false;

            const auto& [bestAsk, _] = *asks_.begin();
            return price >= bestAsk;
        } else {
            if (bids_.empty())
                return false;
            const auto& [bestBid, _] = *bids_.begin();
            return price <= bestBid;
        }
    }

    Trades MatchOrders() {
        Trades trades;
        trades.reserve(orders_.size());

        while (true) {
            if (bids_.empty() || asks_.empty())
                break;

            auto& [bidPrice, bids] = *bids_.begin();
            auto& [askPrice, asks] = *asks_.begin();

            if (bidPrice < askPrice)
                break;

            while (!bids.empty() && !asks.empty()) {
                const auto& bid = bids.front();
                const auto& ask = asks.front();

                const Quantity quantity = std::min(bid->getRemainingQuantity(), ask->getRemainingQuantity());

                bid->fill(quantity);
                ask->fill(quantity);

                if (bid->isFilled()) {
                    bids.pop_front();
                    orders_.erase(bid->getOrderId());
                }

                if (ask->isFilled()) {
                    asks.pop_front();
                    orders_.erase(ask->getOrderId());
                }

                if (bids.empty())
                    bids_.erase(bidPrice);

                if (asks.empty())
                    asks_.erase(askPrice);

                trades.emplace_back(
                    TradeInfo { bid->getOrderId(), bid->getPrice(), quantity},
                    TradeInfo { ask->getOrderId(), ask->getPrice(), quantity }
                );

            }

            if (!bids_.empty()) {
                auto& [_, bids] = *bids_.begin();
                auto& order = bids.front();
                if (order->getOrderType() == OrderType::FillAndKill)
                    CancelOrder(order->getOrderId());
            }

            if (!asks_.empty()) {
                auto& [_, asks] = *asks_.begin();
                auto& order = asks.front();
                if (order->getOrderType() == OrderType::FillAndKill)
                    CancelOrder(order->getOrderId());
            }
        }

        return trades;
    }

public:
    Trades addOrder(const OrderPointer& order) {
        if (orders_.contains(order->getOrderId()))
            return {};

        if (order->getOrderType() == OrderType::Market) {
            if (order->getSide() == Side::Buy && !asks_.empty()) {
                const auto& [worstAsk, _] = *asks_.begin();
                order->toGoodTillCancel(worstAsk);
            } else if (order->getSide() == Side::Sell && !bids_.empty()) {
                const auto& [worstBid, _] = *bids_.begin();
                order->toGoodTillCancel(worstBid);
            } else return {};
        }

        if (order->getOrderType() == OrderType
            ::FillAndKill && !CanMatch(order->getSide(), order->getPrice()))
            return {};

        OrderPointers::iterator iterator;

        if (order->getSide() == Side::Buy) {
            auto& orders = bids_[order->getPrice()];
            orders.push_back(order);
            iterator = std::next(orders.begin(), orders.size() - 1);
        } else {
            auto& orders = asks_[order->getPrice()];
            orders.push_back(order);
            iterator = std::next(orders.begin(), orders.size() - 1);
        }

        orders_.insert({ order->getOrderId(), OrderEntry{ order, iterator} });
        return MatchOrders();

    }

    void CancelOrder(const OrderId orderId) {
        if (!orders_.contains(orderId))
            return;

        const auto& [order, iterator] = orders_.at(orderId);
        orders_.erase(orderId);

        if (order->getSide() == Side::Sell) {
            const auto price  = order->getPrice();
            auto& orders = asks_.at(price);
            orders.erase(iterator);
            if (orders.empty())
                asks_.erase(price);
        } else {
            const auto price  = order->getPrice();
            auto& orders = bids_.at(price);
            orders.erase(iterator);
            if (orders.empty())
                bids_.erase(price);
        }
    }

    Trades ModifyOrder(const OrderModify &order) {
        if (!orders_.contains(order.GetOrderId()))
            return {};

        const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
        CancelOrder(order.GetOrderId());
        return addOrder(order.ToOrderPointer(existingOrder->getOrderType()));
    }

    std::size_t Size() const { return orders_.size();}

    OrderBookLevelInfos GetOrderInfos() const {
        LevelInfos bidInfos, askInfos;
        bidInfos.reserve(orders_.size());
        askInfos.reserve(orders_.size());

        auto CreateLevelInfos = [](const Price price, const OrderPointers& orders) {
            return LevelInfo { price, std::accumulate(orders.begin(), orders.end(), static_cast<Quantity>(0),
                [](const Quantity runningSum, const OrderPointer& order) {
                return runningSum + order->getRemainingQuantity();
            })};
        };

        for (const auto& [price, orders]: bids_)
            bidInfos.push_back(CreateLevelInfos(price, orders));

        for (const auto& [price, orders]: asks_)
            askInfos.push_back(CreateLevelInfos(price, orders));

        return OrderBookLevelInfos(bidInfos, askInfos);

    }

    // Add this new method for preloading orders
    bool PreloadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        std::string line;
        int lineNumber = 0;
        int loadedOrders = 0;

        while (std::getline(file, line)) {
            lineNumber++;
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::string sideStr, typeStr, priceStr, quantityStr;
            
            if (!(iss >> sideStr >> typeStr >> priceStr >> quantityStr)) {
                std::cerr << "Warning: Invalid format at line " << lineNumber << ": " << line << std::endl;
                continue;
            }

            try {
                // Parse the order components
                Side side;
                if (sideStr == "B" || sideStr == "b") side = Side::Buy;
                else if (sideStr == "S" || sideStr == "s") side = Side::Sell;
                else {
                    std::cerr << "Warning: Invalid side '" << sideStr << "' at line " << lineNumber << std::endl;
                    continue;
                }

                OrderType orderType;
                if (typeStr == "GTC" || typeStr == "gtc") orderType = OrderType::GoodTillCancel;
                else if (typeStr == "FAK" || typeStr == "fak") orderType = OrderType::FillAndKill;
                else if (typeStr == "M" || typeStr == "m") orderType = OrderType::Market;
                else {
                    std::cerr << "Warning: Invalid order type '" << typeStr << "' at line " << lineNumber << std::endl;
                    continue;
                }

                Price price;
                if (priceStr == "0" || priceStr.empty()) {
                    price = Constants::InvalidPrice;
                } else {
                    try {
                        price = std::stoi(priceStr);
                    } catch (...) {
                        std::cerr << "Warning: Invalid price '" << priceStr << "' at line " << lineNumber << std::endl;
                        continue;
                    }
                }

                Quantity quantity;
                try {
                    int qty = std::stoi(quantityStr);
                    if (qty <= 0) {
                        std::cerr << "Warning: Invalid quantity '" << quantityStr << "' at line " << lineNumber << std::endl;
                        continue;
                    }
                    quantity = static_cast<Quantity>(qty);
                } catch (...) {
                    std::cerr << "Warning: Invalid quantity '" << quantityStr << "' at line " << lineNumber << std::endl;
                    continue;
                }

                // Create and add the order
                auto order = std::make_shared<Order>(orderType, nextOrderId_++, side, price, quantity);
                auto trades = addOrder(order);
                
                if (!trades.empty()) {
                    std::cout << "Trade executed during preload: " << trades.size() << " trades" << std::endl;
                }
                
                loadedOrders++;
                
            } catch (const std::exception& e) {
                std::cerr << "Error processing line " << lineNumber << ": " << e.what() << std::endl;
            }
        }

        file.close();
        std::cout << "Preloaded " << loadedOrders << " orders from " << filename << std::endl;
        return true;
    }

    // Add getter for next order ID (useful for CLI)
    OrderId GetNextOrderId() const { return nextOrderId_; }
};
