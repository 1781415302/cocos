#include "GameModelFromLevelGenerator.h"
#include "models/CardModel.h"
#include <memory>

/*
 * 说明：
 * - 该生成器将 LevelConfig 中的 Playfield 映射到 GameModel::playfield（addPlayfieldCard）
 * - 将 LevelConfig 中的 Stack 映射为 reserve（备用牌堆），使用 addReserveCard。
 * - hand（底牌堆）在默认情况下为空；如果你希望关卡配置包含初始 hand，请扩展 LevelConfig 格式并这里按需填充。
 */

GameModel GameModelFromLevelGenerator::generateGameModel(const LevelConfig & config)
{
    GameModel gameModel;

    // Playfield
    const auto& playfieldConfig = config.getPlayfieldConfig();
    for (const auto& cardConfig : playfieldConfig.cards) {
        bool isFaceUp = true; // playfield 中按你的规则可能都是翻开
        auto card = std::make_shared<CardModel>(cardConfig.suit, cardConfig.face, cardConfig.position, isFaceUp);
        gameModel.addPlayfieldCard(card);
    }

    // Stack -> reserve
    const auto& stackConfig = config.getStackConfig();
    for (size_t i = 0; i < stackConfig.cards.size(); ++i) {
        const auto& cardConfig = stackConfig.cards[i];
        bool isFaceUp = false;
        // 若你希望 reserve 顶牌显示为翻开，可将最后一张设为翻开（按原实现）
        if (!stackConfig.cards.empty() && i == stackConfig.cards.size() - 1) {
            isFaceUp = true;
        }
        auto card = std::make_shared<CardModel>(cardConfig.suit, cardConfig.face, cardConfig.position, isFaceUp);
        gameModel.addReserveCard(card);
    }

    // hand 初始为空（也可由关卡配置决定）
    return gameModel;
}