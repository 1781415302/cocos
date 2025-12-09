#pragma once
// Classes/models/GameModel.h
#ifndef GameModel_h
#define GameModel_h

#include "cocos2d.h"
#include "CardModel.h"
#include <vector>
#include <memory>
#include <utils/json.hpp>

using json = nlohmann::json;

/**
 * GameModel 现在只作为数据容器（状态/持久化），并提供少量兼容性方法。
 * 复杂业务逻辑/操作应放到 controllers/managers 或 services。
 */
class GameModel
{
public:
    GameModel();

    std::vector<std::shared_ptr<CardModel>>& getPlayfieldCards();
    const std::vector<std::shared_ptr<CardModel>>& getPlayfieldCards() const;
    std::vector<std::shared_ptr<CardModel>>& getReserveCards();
    const std::vector<std::shared_ptr<CardModel>>& getReserveCards() const;
    std::vector<std::shared_ptr<CardModel>>& getHandCards();
    const std::vector<std::shared_ptr<CardModel>>& getHandCards() const;

    // Compatibility helpers: push/add methods and id allocation (lightweight)
    void addPlayfieldCard(const std::shared_ptr<CardModel>& card);
    void addReserveCard(const std::shared_ptr<CardModel>& card);
    void addHandCard(const std::shared_ptr<CardModel>& card);

    int allocateCardId();

    // nextCardId 作为状态一部分（分配策略可由 manager 控制）
    int getNextCardId() const;
    void setNextCardId(int v);

    // Serialization via free functions to_json/from_json
    json toJson() const; // 可保留兼容方法
    static GameModel fromJson(const json& j);

    // 查找帮助
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