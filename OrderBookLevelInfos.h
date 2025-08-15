//
// Created by Kin on 8/15/2025.
//
#pragma once
#include <vector>

#include "LevelInfo.h"


using LevelInfos = std::pmr::vector<LevelInfo>;


class OrderBookLevelInfos {
public:
    OrderBookLevelInfos(const LevelInfos& bids, const LevelInfos& asks) : bids_(bids), asks_(asks) {};
private:
    LevelInfos bids_;
    LevelInfos asks_;
};