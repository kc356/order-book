cmake_minimum_required(VERSION 3.20)
project(order_book1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
        Constants.h
        OrderBookCLI.h
)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(order_book_tests
        test/order_book_tests.cpp
)
target_link_libraries(order_book_tests PRIVATE GTest::gtest_main)
target_include_directories(order_book_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(order_book_tests)

# Copy sample data file to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sample_orders.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})