#pragma once
// test/Classes/configs/LevelConfig.h
#ifndef LevelConfig_h
#define LevelConfig_h

#include "cocos2d.h"
#include "utils/CardEnums.h"
#include <vector>

// @brief 表示一张牌的配置信息
struct CardConfig
{
    CardSuitType suit;        // 花色
    CardFaceType face;        // 点数
    cocos2d::Vec2 position;   // 位置
};

// @brief 表示一个牌堆的配置信息
struct CardPileConfig
{
    std::vector<CardConfig> cards; // 牌列表
};

// @brief 关卡配置数据模型
class LevelConfig
{
public:
    // @brief 获取主牌区配置
    const CardPileConfig& getPlayfieldConfig() const;
    // @brief 获取备用牌堆配置
    const CardPileConfig& getStackConfig() const;

    // @brief 设置主牌区配置
    void setPlayfieldConfig(const CardPileConfig& config);
    // @brief 设置备用牌堆配置
    void setStackConfig(const CardPileConfig& config);

private:
    CardPileConfig _playfieldConfig; // 主牌区配置
    CardPileConfig _stackConfig;     // 备用牌堆配置
};

#endif // LevelConfig_h
