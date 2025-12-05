// test/Classes/models/GameModel.h
#ifndef GameModel_h
#define GameModel_h

#include "cocos2d.h"
#include "CardModel.h"
#include <vector>
#include <memory>

// @brief 存储游戏运行时状态的数据模型
class GameModel
{
public:
    // @brief 构造
    GameModel();

    // Playfield / Stack 的容器现在存放 shared_ptr<CardModel>
    std::vector<std::shared_ptr<CardModel>>& getPlayfieldCards();
    const std::vector<std::shared_ptr<CardModel>>& getPlayfieldCards() const;

    std::vector<std::shared_ptr<CardModel>>& getStackCards();
    const std::vector<std::shared_ptr<CardModel>>& getStackCards() const;

    // 辅助：向 playfield / stack 添加卡牌（用于生成器）
    void addPlayfieldCard(const std::shared_ptr<CardModel>& card);
    void addStackCard(const std::shared_ptr<CardModel>& card);

    // 将 playfield 中索引为 playfieldIndex 的卡移动（逻辑上）到 stack（底牌堆）
    bool movePlayfieldCardToStack(int playfieldIndex);

    // 翻开 stack 顶牌
    bool flipTopStackCard();

    // 判断是否存在可移动的 playfield 卡
    bool hasMovablePlayfieldCard() const;

    // 判断 stack 顶牌是否翻开（并可用于匹配）
    bool canMatchWithTopStackCard() const;

    // 分配一个新的唯一 id（最简单实现：递增）
    int allocateCardId();

private:
    std::vector<std::shared_ptr<CardModel>> _playfieldCards; // 主牌区
    std::vector<std::shared_ptr<CardModel>> _stackCards;     // 备用/底牌堆

    int _nextCardId = 1; // 用于分配唯一 id（从 1 开始）
};

#endif // GameModel_h