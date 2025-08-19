//
// Created by Kin on 8/15/2025.
//
#pragma once

#include "TradeInfo.h"

class Trade {
public:
    Trade(const TradeInfo& bitTrade, const TradeInfo& askTrade): bidTrade_{bitTrade}, askTrade_{askTrade} {};

    const TradeInfo& getBitTrade() const {return bidTrade_;}
    const TradeInfo& getAskTrade() const {return askTrade_;}

private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
};
