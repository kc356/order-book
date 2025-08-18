//
// Created by Kin on 8/15/2025.
//
#pragma once
#include <utility>
#include <vector>

#include "LevelInfo.h"


using LevelInfos = std::pmr::vector<LevelInfo>;


class OrderBookLevelInfos {
public:
    OrderBookLevelInfos(LevelInfos  bids, LevelInfos  asks) : bids_(std::move(bids)), asks_(std::move(asks)) {};
    const LevelInfos& GetBids() const { return bids_; }
    const LevelInfos& GetAsks() const { return asks_; }
private:
    LevelInfos bids_;
    LevelInfos asks_;
};