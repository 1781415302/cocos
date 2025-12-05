#pragma once
// Classes/models/GameModel.h
#ifndef GameModel_h
#define GameModel_h

#include "cocos2d.h"
#include "CardModel.h"
#include <vector>
#include <memory>

/**
 * @brief 游戏运行时数据模型（支持 playfield / reserve / hand 三个区域）
 *
 * 说明：
 * - playfield: 主牌区（可点击消除的卡牌集合）
 * - reserve:  备用牌堆（玩家从中抽牌到 hand）
 * - hand:     底牌堆（用于与 playfield 卡牌匹配）
 *
 * 该类提供了控制器需要的基本动作接口（如 drawReserveToHand、movePlayfieldCardToHand 等）。
 */
class GameModel
{
public:
    GameModel();

    // 访问三个区域的容器
    std::vector<std::shared_ptr<CardModel>>& getPlayfieldCards();
    const std::vector<std::shared_ptr<CardModel>>& getPlayfieldCards() const;

    std::vector<std::shared_ptr<CardModel>>& getReserveCards();
    const std::vector<std::shared_ptr<CardModel>>& getReserveCards() const;

    std::vector<std::shared_ptr<CardModel>>& getHandCards();
    const std::vector<std::shared_ptr<CardModel>>& getHandCards() const;

    // 兼容旧代码：getStackCards() 作为 getHandCards() 的别名
    std::vector<std::shared_ptr<CardModel>>& getStackCards();
    const std::vector<std::shared_ptr<CardModel>>& getStackCards() const;

    // 添加卡片到相应区域（用于生成器）
    void addPlayfieldCard(const std::shared_ptr<CardModel>& card);
    void addReserveCard(const std::shared_ptr<CardModel>& card);
    void addHandCard(const std::shared_ptr<CardModel>& card);

    // 基本操作：
    // 将 reserve 的顶牌（尾部）移动到 hand（尾部），并把该卡设置为翻开（face up）
    bool drawReserveToHand();

    // 将 playfield 指定索引的卡移动到 hand（尾部）
    bool movePlayfieldCardToHand(int playfieldIndex);

    // 将 hand 顶部卡移回 playfield 指定位置（用于未来 Undo）
    bool moveTopHandCardToPlayfieldAt(int playfieldIndex, cocos2d::Vec2 position, CardStatus status);

    // 翻开 hand 顶牌（若需要）
    bool flipTopHandCard();

    // 判断 playfield 是否存在可移动的卡（翻开并可见）
    bool hasMovablePlayfieldCard() const;

    // 判断 hand 顶牌是否存在并翻开（可作为匹配目标）
    bool canMatchWithHandTop() const;

    // 分配唯一 id（被 CardModel 使用）
    int allocateCardId();

    // 查找指定 cardId 在各区域的索引（不存在返回 -1）
    int findPlayfieldIndexById(int cardId) const;
    int findReserveIndexById(int cardId) const;
    int findHandIndexById(int cardId) const;

private:
    std::vector<std::shared_ptr<CardModel>> _playfieldCards;
    std::vector<std::shared_ptr<CardModel>> _reserveCards;
    std::vector<std::shared_ptr<CardModel>> _handCards;

    int _nextCardId = 1;
};

#endif // GameModel_h