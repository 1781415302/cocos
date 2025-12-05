// test/Classes/services/GameModelFromLevelGenerator.cpp
#include "GameModelFromLevelGenerator.h"
#include "models/CardModel.h"

GameModel GameModelFromLevelGenerator::generateGameModel(const LevelConfig& config)
{
    GameModel gameModel;

    // 从配置生成主牌区的牌
    const auto& playfieldConfig = config.getPlayfieldConfig();
    for (const auto& cardConfig : playfieldConfig.cards) {
        CardModel card(cardConfig.suit, cardConfig.face, cardConfig.position, true); // 假设配置的都是翻开的
        gameModel.getPlayfieldCards().push_back(card);
    }

    // 从配置生成备用牌堆的牌
    const auto& stackConfig = config.getStackConfig();
    for (const auto& cardConfig : stackConfig.cards) {
        // 备用牌堆的牌，除了最顶上一张，其他都是背面朝上
        bool isFaceUp = false; // 默认背面朝上
        // 如果是最后一个牌（最顶上），根据需求，可能需要翻开
        // 这里根据需求，备用牌堆初始只翻开一张顶牌
        if (&cardConfig == &stackConfig.cards.back()) { // 如果是最后一个元素
            isFaceUp = true; // 顶牌翻开
        }
        CardModel card(cardConfig.suit, cardConfig.face, cardConfig.position, isFaceUp);
        gameModel.getStackCards().push_back(card);
    }

    return gameModel;
}