
#include "pch.h"

#include "OrderBook.h"

namespace googletest = ::testing;

enum class ActionType
{
    Add,
    Cancel,
    Modify,
};

struct Information
{
    ActionType type_;
    OrderType orderType_;
    Side side_;
    Price price_;
    Quantity quantity_;
    OrderId orderId_;
};

using Informations = std::vector<Information>;

struct Result
{
    std::size_t allCount_;
    std::size_t bidCount_;
    std::size_t askCount_;
};

using Results = std::vector<Result>;

struct InputHandler
{
private:
    static std::uint32_t toNumber(const std::string_view& str)
    {
        std::int64_t value{};
        std::from_chars(str.data(), str.data() + str.size(), value);
        if (value < 0)
            throw std::logic_error("Value is below zero.");
        return static_cast<std::uint32_t>(value);
    }

    static bool tryParseResult(const std::string_view& str, Result& result)
    {
        if (str.at(0) != 'R')
            return false;

        const auto values = Split(str, ' ');
        result.allCount_ = toNumber(values[1]);
        result.bidCount_ = toNumber(values[2]);
        result.askCount_ = toNumber(values[3]);

        return true;
    }

    static bool tryParseInformation(const std::string_view& str, Information& action)
    {
        const auto value = str.at(0);
        const auto values = Split(str, ' ');
        if (value == 'A')
        {
            action.type_ = ActionType::Add;
            action.side_ = ParseSide(values[1]);
            action.orderType_ = ParseOrderType(values[2]);
            action.price_ = ParsePrice(values[3]);
            action.quantity_ = ParseQuantity(values[4]);
            action.orderId_ = ParseOrderId(values[5]);
        }
        else if (value == 'M')
        {
            action.type_ = ActionType::Modify;
            action.orderId_ = ParseOrderId(values[1]);
            action.side_ = ParseSide(values[2]);
            action.price_ = ParsePrice(values[3]);
            action.quantity_ = ParseQuantity(values[4]);
        }
        else if (value == 'C')
        {
            action.type_ = ActionType::Cancel;
            action.orderId_ = ParseOrderId(values[1]);
        }
        else return false;

        return true;
    }

    static std::vector<std::string_view> Split(const std::string_view& str, char delimeter)
    {
        std::vector<std::string_view> columns;
        columns.reserve(5);
        std::size_t start_index{}, end_index{};
        while ((end_index = str.find(delimeter, start_index)) != std::string::npos)
        {
            const auto distance = end_index - start_index;
            auto column = str.substr(start_index, distance);
            start_index = end_index + 1;
            columns.push_back(column);
        }
        columns.push_back(str.substr(start_index));
        return columns;
    }

    static Side ParseSide(const std::string_view& str)
    {
        if (str == "B")
            return Side::Buy;
        else if (str == "S")
            return Side::Sell;
        else throw std::logic_error("Unknown Side");
    }

    static OrderType ParseOrderType(const std::string_view& str)
    {
        if (str == "FillAndKill")
            return OrderType::FillAndKill;
        else if (str == "GoodTillCancel")
            return OrderType::GoodTillCancel;
        else if (str == "GoodForDay")
            return OrderType::GoodForDay;
        else if (str == "FillOrKill")
            return OrderType::FillOrKill;
        else if (str == "Market")
            return OrderType::Market;
        else throw std::logic_error("Unknown OrderType");
    }

    static Price ParsePrice(const std::string_view& str)
    {
        if (str.empty())
            throw std::logic_error("Unknown Price");

        return toNumber(str);
    }

    static Quantity ParseQuantity(const std::string_view& str)
    {
        if (str.empty())
            throw std::logic_error("Unknown Quantity");

        return toNumber(str);
    }

    static OrderId ParseOrderId(const std::string_view& str)
    {
        if (str.empty())
            throw std::logic_error("Empty OrderId");

        return static_cast<OrderId>(toNumber(str));
    }

public:
    static std::tuple<Informations, Result> GetInformations(const std::filesystem::path& path)
    {
        Informations actions;
        actions.reserve(1'000);
        Result result { };
        bool resultFound = false;

        std::string line;
        std::ifstream file{ path };
        
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + path.string());
        }

        while (std::getline(file, line))
        {
            if (line.empty())
                continue;

            if (line.at(0) != 'R')
            {
                Information action { };
                if (!tryParseInformation(line, action))
                    continue;
                actions.push_back(action);
            }
            else
            {
                if (!tryParseResult(line, result))
                    continue;
                resultFound = true;
                break; // Found result, stop reading
            }
        }

        if (!resultFound) {
            throw std::logic_error("No result specified in file: " + path.string());
        }

        return { actions, result };
    }
};


class OrderbookTestsFixture : public googletest::TestWithParam<const char*>
{
private:
    const static inline std::filesystem::path GetTestFilesDir() {
        // __FILE__ gives us the path to the current source file
        const std::filesystem::path currentFile(__FILE__);
        // Go up from test/order_book_tests.cpp to test/, then to testFiles
        return currentFile.parent_path() / "testFiles";
    }
    

    
public:
    const static inline std::filesystem::path TestFolderPath{ GetTestFilesDir() };

    void SetUp() override {
        std::cout << "Looking for test files in: " << TestFolderPath << std::endl;
        
        if (!std::filesystem::exists(TestFolderPath)) {
            std::cout << "Test directory does not exist at: " << TestFolderPath << std::endl;
        }
    }
};

TEST_P(OrderbookTestsFixture, OrderbookTestSuite)
{
    // Arrange
    const auto file = OrderbookTestsFixture::TestFolderPath / GetParam();

    constexpr InputHandler handler;
    const auto [actions, result] = InputHandler::GetInformations(file);

    auto GetOrder = [](const Information& action)
    {
        return std::make_shared<Order>(
            action.orderType_,
            action.orderId_,
            action.side_,
            action.price_,
            action.quantity_);
    };

    auto GetOrderModify = [](const Information& action)
    {
        return OrderModify
        {
            action.orderId_,
            action.side_,
            action.price_,
            action.quantity_,
        };
    };

    // Act
    OrderBook orderbook;
    for (const auto& action : actions)
    {
        switch (action.type_)
        {
        case ActionType::Add:
        {
            const Trades& trades = orderbook.addOrder(GetOrder(action));
        }
        break;
        case ActionType::Modify:
        {
            const Trades& trades = orderbook.ModifyOrder(GetOrderModify(action));
        }
        break;
        case ActionType::Cancel:
        {
            orderbook.CancelOrder(action.orderId_);
        }
        break;
        default:
            throw std::logic_error("Unsupported Action.");
        }
    }

    // Assert
    const auto& orderbookInfos = orderbook.GetOrderInfos();
    ASSERT_EQ(orderbook.Size(), result.allCount_);
    ASSERT_EQ(orderbookInfos.GetBids().size(), result.bidCount_);
    ASSERT_EQ(orderbookInfos.GetAsks().size(), result.askCount_);
}

INSTANTIATE_TEST_SUITE_P(Tests, OrderbookTestsFixture, googletest::ValuesIn({
    "Match_GoodTillCancel.txt",
    "Match_FillAndKill.txt",
    "Match_FillOrKill_Hit.txt",
    // "Match_FillOrKill_Miss.txt",
    "Cancel_Success.txt",
    "Modify_Side.txt",
    "Match_Market.txt"
}));