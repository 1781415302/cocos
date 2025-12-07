#pragma once
// Classes/models/GameModel.h
#ifndef GameModel_h
#define GameModel_h

#include "cocos2d.h"
#include "CardModel.h"
#include <vector>
#include <memory>

/**
 * @brief 游戏运行时数据模型，支持 playfield / reserve / hand 三堆牌的操作
 *
 * 说明
 * - playfield: 主牌区，可拖动的可见牌集合
 * - reserve:  备用牌堆，点击抽牌到 hand
 * - hand:     手牌堆（底牌），用于与 playfield 匹配
 *
 * 提供了核心的堆操作接口，如 drawReserveToHand、movePlayfieldCardToHand 等。
 */
class GameModel
{
public:
    GameModel();

    // 访问器
    std::vector<std::shared_ptr<CardModel>>& getPlayfieldCards();
    const std::vector<std::shared_ptr<CardModel>>& getPlayfieldCards() const;

    std::vector<std::shared_ptr<CardModel>>& getReserveCards();
    const std::vector<std::shared_ptr<CardModel>>& getReserveCards() const;

    std::vector<std::shared_ptr<CardModel>>& getHandCards();
    const std::vector<std::shared_ptr<CardModel>>& getHandCards() const;

    // 为兼容旧命名：getStackCards() 视作 hand
    std::vector<std::shared_ptr<CardModel>>& getStackCards();
    const std::vector<std::shared_ptr<CardModel>>& getStackCards() const;

    // 添加牌到各堆
    void addPlayfieldCard(const std::shared_ptr<CardModel>& card);
    void addReserveCard(const std::shared_ptr<CardModel>& card);
    void addHandCard(const std::shared_ptr<CardModel>& card);

    // 正向操作
    bool drawReserveToHand();                      // reserve 顶牌 -> hand，并翻开
    bool movePlayfieldCardToHand(int playfieldIndex); // playfield[i] -> hand

    // hand 顶牌 -> playfield 指定位置（Undo 用）
    bool moveTopHandCardToPlayfieldAt(int playfieldIndex, cocos2d::Vec2 position, CardStatus status);
    bool moveTopHandCardToPlayfieldAt(int playfieldIndex, cocos2d::Vec2 position, CardStatus status, bool visible);
    bool moveTopHandCardToPlayfieldAt(int playfieldIndex, cocos2d::Vec2 position, CardStatus status, bool visible, bool faceUp);

    // hand 顶牌翻面（仅设为正面）
    bool flipTopHandCard();

    // Undo 支撑：hand 顶牌退回 reserve，并恢复属性
    bool moveTopHandCardBackToReserve(cocos2d::Vec2 position, CardStatus status, bool visible);
    bool moveTopHandCardBackToReserve(cocos2d::Vec2 position, CardStatus status, bool visible, bool faceUp);

    // 能否操作判断
    bool hasMovablePlayfieldCard() const;
    bool canMatchWithHandTop() const;

    // 分配唯一 id
    int allocateCardId();

    // 索引查找
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