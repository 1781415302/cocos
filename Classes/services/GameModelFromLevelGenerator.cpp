// test/Classes/services/GameModelFromLevelGenerator.cpp
#include "GameModelFromLevelGenerator.h"
#include "models/CardModel.h"
#include <memory>

GameModel GameModelFromLevelGenerator::generateGameModel(const LevelConfig& config)
{
    GameModel gameModel;

    // Playfield 卡牌生成
    const auto& playfieldConfig = config.getPlayfieldConfig();
    for (const auto& cardConfig : playfieldConfig.cards) {
        // 兼容现有 CardModel 的便捷构造： (suit, face, position, isFaceUp)
        bool isFaceUp = false;
        // 如果配置需要默认翻开可以在 CardConfig 中添加字段，这里假设 playfield 中默认翻开
        // 如果需要覆面，可根据配置调整
        isFaceUp = true; // 主牌区通常是展示的牌（如果不是，请调整）
        auto card = std::make_shared<CardModel>(cardConfig.suit, cardConfig.face, cardConfig.position, isFaceUp);
        gameModel.addPlayfieldCard(card);
    }

    // Stack（备用牌堆）生成
    const auto& stackConfig = config.getStackConfig();
    // 采用索引遍历，明确判断最后一个元素作为初始顶部（翻开）
    for (size_t i = 0; i < stackConfig.cards.size(); ++i) {
        const auto& cardConfig = stackConfig.cards[i];
        bool isFaceUp = false;
        if (!stackConfig.cards.empty() && i == stackConfig.cards.size() - 1) {
            isFaceUp = true; // 将最后一张作为初始翻开的顶牌
        }
        auto card = std::make_shared<CardModel>(cardConfig.suit, cardConfig.face, cardConfig.position, isFaceUp);
        gameModel.addStackCard(card);
    }

    return gameModel;
}