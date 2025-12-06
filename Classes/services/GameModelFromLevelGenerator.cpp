#include "GameModelFromLevelGenerator.h"
#include "models/CardModel.h"
#include <memory>

/*
 * 说明：
 * - 该生成器将 LevelConfig 中的 Playfield 映射到 GameModel::playfield（addPlayfieldCard）
 * - 将 LevelConfig 中的 Stack 映射为 reserve（备用牌堆），使用 addReserveCard。
 * - hand（底牌堆）在默认情况下为空；如果你希望关卡配置包含初始 hand，请扩展 LevelConfig 格式并这里按需填充。
 */

GameModel GameModelFromLevelGenerator::generateGameModel(const LevelConfig& config)
{
    GameModel gameModel;

    // Playfield
    const auto& playfieldConfig = config.getPlayfieldConfig();
    for (const auto& cardConfig : playfieldConfig.cards) {
        bool isFaceUp = true; // playfield 上的牌默认翻开
        auto card = std::make_shared<CardModel>(cardConfig.suit, cardConfig.face, cardConfig.position, isFaceUp);
        gameModel.addPlayfieldCard(card);
    }

    // Stack -> reserve
    const auto& stackConfig = config.getStackConfig();
    for (size_t i = 0; i < stackConfig.cards.size(); ++i) {
        const auto& cardConfig = stackConfig.cards[i];
        // 修改点：所有 reserve 卡在初始时都为背面（未翻开）
        bool isFaceUp = false;
        auto card = std::make_shared<CardModel>(cardConfig.suit, cardConfig.face, cardConfig.position, isFaceUp);
        gameModel.addReserveCard(card);
    }

    // hand 默认为空，GameController 会在 start 时抽一张到 hand
    return gameModel;
}