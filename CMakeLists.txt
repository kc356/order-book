cmake_minimum_required(VERSION 4.0)
project(order_book1)

set(CMAKE_CXX_STANDARD 23)

add_executable(order_book1 main.cpp
        OrderType.h
        Side.h
        Usings.h
        LevelInfo.h
        OrderBookLevelInfos.h
        Order.h
        OrderModify.h
        TradeInfo.h
        Trade.h
        OrderBook.h
)
