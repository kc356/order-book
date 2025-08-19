#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <memory>
#include <vector>
#include <algorithm>
#include "OrderBook.h"
#include "Order.h"
#include "OrderModify.h"
#include "Side.h"
#include "OrderType.h"

class OrderBookCLI {
private:
    OrderBook orderBook_;
    OrderId nextOrderId_ = 1;
    
    static constexpr int PRICE_WIDTH = 8;
    static constexpr int QUANTITY_WIDTH = 10;
    static constexpr int DEPTH_WIDTH = 15;
    
    static void PrintHeader() {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "                           ORDER BOOK CLI\n";
        std::cout << std::string(80, '=') << "\n";
    }
    
    static void PrintHelp() {
        std::cout << "\nAvailable Commands:\n";
        std::cout << "  add <side> <type> <price> <quantity>  - Add new order\n";
        std::cout << "  modify <id> <side> <price> <quantity> - Modify existing order\n";
        std::cout << "  cancel <id>                           - Cancel order\n";
        std::cout << "  preload <filename>                     - Load orders from file\n";
        std::cout << "  book                                  - Show order book\n";
        std::cout << "  orders                                - Show all orders\n";
        std::cout << "  help                                  - Show this help\n";
        std::cout << "  quit                                  - Exit program\n\n";
        std::cout << "Side: B (Buy) or S (Sell)\n";
        std::cout << "Type: GTC (GoodTillCancel), FAK (FillAndKill), M (Market)\n";
        std::cout << "Price: Integer price (use 0 for Market orders)\n";
        std::cout << "Quantity: Integer quantity\n\n";
    }
    
    void PrintOrderBook() const {
        const auto& orderInfos = orderBook_.GetOrderInfos();
        const auto& bids = orderInfos.GetBids();
        const auto& asks = orderInfos.GetAsks();
        
        std::cout << "\n" << std::string(80, '-') << "\n";
        std::cout << "                                ORDER BOOK\n";
        std::cout << std::string(80, '-') << "\n";
        
        // Print asks (sell side) - reverse order to show lowest ask at top
        std::cout << std::setw(PRICE_WIDTH) << "PRICE" 
                  << std::setw(QUANTITY_WIDTH) << "QUANTITY" 
                  << std::setw(DEPTH_WIDTH) << "DEPTH" << "\n";
        std::cout << std::string(33, '-') << "\n";
        
        std::vector<LevelInfo> asksCopy(asks.begin(), asks.end());
        std::ranges::reverse(asksCopy);
        
        for (const auto&[price_, quantity_] : asksCopy) {
            std::cout << std::setw(PRICE_WIDTH) << price_
                      << std::setw(QUANTITY_WIDTH) << quantity_
                      << std::setw(DEPTH_WIDTH) << "SELL" << "\n";
        }
        
        std::cout << std::string(33, '-') << "\n";
        
        // Print bids (buy side)
        for (const auto&[price_, quantity_] : bids) {
            std::cout << std::setw(PRICE_WIDTH) << price_
                      << std::setw(QUANTITY_WIDTH) << quantity_
                      << std::setw(DEPTH_WIDTH) << "BUY" << "\n";
        }
        
        std::cout << std::string(33, '-') << "\n";
        std::cout << "Total Orders: " << orderBook_.Size() << "\n";
        std::cout << std::string(80, '-') << "\n";
    }
    
    static void PrintOrders() {
        std::cout << "\n" << std::string(80, '-') << "\n";
        std::cout << "                                ALL ORDERS\n";
        std::cout << std::string(80, '-') << "\n";
        
        std::cout << std::setw(8) << "ID" 
                  << std::setw(6) << "SIDE" 
                  << std::setw(8) << "TYPE" 
                  << std::setw(8) << "PRICE" 
                  << std::setw(10) << "INITIAL" 
                  << std::setw(10) << "REMAINING" 
                  << std::setw(10) << "FILLED" << "\n";
        std::cout << std::string(60, '-') << "\n";
        
        // Note: This is a simplified view since OrderBook doesn't expose individual orders
        // You might want to add a method to get all orders for display
        std::cout << "Order details not available in current implementation\n";
        std::cout << std::string(60, '-') << "\n";
    }
    
    static Side ParseSide(const std::string& sideStr) {
        if (sideStr == "B" || sideStr == "b") return Side::Buy;
        if (sideStr == "S" || sideStr == "s") return Side::Sell;
        throw std::invalid_argument("Invalid side. Use B (Buy) or S (Sell)");
    }
    
    static OrderType ParseOrderType(const std::string& typeStr) {
        if (typeStr == "GTC" || typeStr == "gtc") return OrderType::GoodTillCancel;
        if (typeStr == "FAK" || typeStr == "fak") return OrderType::FillAndKill;
        if (typeStr == "M" || typeStr == "m") return OrderType::Market;
        throw std::invalid_argument("Invalid order type. Use GTC, FAK, or M");
    }
    
    static Price ParsePrice(const std::string& priceStr) {
        if (priceStr == "0" || priceStr.empty()) return Constants::InvalidPrice;
        try {
            return std::stoi(priceStr);
        } catch (...) {
            throw std::invalid_argument("Invalid price. Use integer or 0 for Market orders");
        }
    }
    
    static Quantity ParseQuantity(const std::string& quantityStr) {
        try {
            const int qty = std::stoi(quantityStr);
            if (qty <= 0) throw std::invalid_argument("Quantity must be positive");
            return static_cast<Quantity>(qty);
        } catch (...) {
            throw std::invalid_argument("Invalid quantity. Use positive integer");
        }
    }
    
    static OrderId ParseOrderId(const std::string& idStr) {
        try {
            return std::stoull(idStr);
        } catch (...) {
            throw std::invalid_argument("Invalid order ID. Use positive integer");
        }
    }
    
    void ProcessAddOrder(const std::vector<std::string>& args) {
        if (args.size() != 5) {
            std::cout << "Usage: add <side> <type> <price> <quantity>\n";
            return;
        }
        
        try {
            Side side = ParseSide(args[1]);
            OrderType type = ParseOrderType(args[2]);
            Price price = ParsePrice(args[3]);
            Quantity quantity = ParseQuantity(args[4]);

            const auto order = std::make_shared<Order>(type, nextOrderId_++, side, price, quantity);
            const auto trades = orderBook_.addOrder(order);
            
            std::cout << "Order added successfully. ID: " << (nextOrderId_ - 1) << "\n";
            
            if (!trades.empty()) {
                std::cout << "Trades executed: " << trades.size() << "\n";
                for (const auto& trade : trades) {
                    std::cout << "  Trade: " << trade.getBitTrade().orderId_ << " -> "
                              << trade.getAskTrade().orderId_ << " @ " << trade.getBitTrade().price_
                              << " x " << trade.getBitTrade().quantity_ << "\n";
                }
            }
            
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    void ProcessModifyOrder(const std::vector<std::string>& args) {
        if (args.size() != 5) {
            std::cout << "Usage: modify <id> <side> <price> <quantity>\n";
            return;
        }
        
        try {
            const OrderId orderId = ParseOrderId(args[1]);
            const Side side = ParseSide(args[2]);
            const Price price = ParsePrice(args[3]);
            const Quantity quantity = ParseQuantity(args[4]);
            
            const OrderModify modify(orderId, side, price, quantity);
            auto const trades = orderBook_.ModifyOrder(modify);
            
            std::cout << "Order modified successfully.\n";
            
            if (!trades.empty()) {
                std::cout << "Trades executed: " << trades.size() << "\n";
            }
            
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    void ProcessCancelOrder(const std::vector<std::string>& args) {
        if (args.size() != 2) {
            std::cout << "Usage: cancel <id>\n";
            return;
        }
        
        try {
            const OrderId orderId = ParseOrderId(args[1]);
            orderBook_.CancelOrder(orderId);
            std::cout << "Order " << orderId << " cancelled successfully.\n";
            
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    void ProcessPreloadOrders(const std::vector<std::string>& args) {
        if (args.size() != 2) {
            std::cout << "Usage: preload <filename>\n";
            return;
        }
        
        try {
            if (const std::string& filename = args[1]; orderBook_.PreloadFromFile(filename)) {
                std::cout << "Orders preloaded successfully from " << filename << "\n";
                // Update the next order ID to avoid conflicts
                nextOrderId_ = orderBook_.GetNextOrderId();
            } else {
                std::cout << "Failed to preload orders from " << filename << "\n";
            }
            
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
public:
    void Run() {
        PrintHeader();
        PrintHelp();
        
        std::string input;
        while (true) {
            std::cout << "\norderbook> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            
            std::istringstream iss(input);
            std::vector<std::string> args;
            std::string arg;
            
            while (iss >> arg) {
                args.push_back(arg);
            }
            
            if (args.empty()) continue;

            if (std::string command = args[0]; command == "quit" || command == "exit") {
                std::cout << "Goodbye!\n";
                break;
            } else if (command == "help") {
                PrintHelp();
            } else if (command == "book") {
                PrintOrderBook();
            } else if (command == "orders") {
                PrintOrders();
            } else if (command == "add") {
                ProcessAddOrder(args);
            } else if (command == "modify") {
                ProcessModifyOrder(args);
            } else if (command == "cancel") {
                ProcessCancelOrder(args);
            } else if (command == "preload") {
                ProcessPreloadOrders(args);
            } else {
                std::cout << "Unknown command: " << command << ". Type 'help' for available commands.\n";
            }
        }
    }
};
