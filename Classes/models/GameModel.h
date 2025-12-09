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
 * 说明：数据层，仅存储运行时数据与序列化，不承载业务操作。
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
    std::vector<std::shared_ptr<CardModel>>& getStackCards();
    const std::vector<std::shared_ptr<CardModel>>& getStackCards() const;

    void addPlayfieldCard(const std::shared_ptr<CardModel>& card);
    void addReserveCard(const std::shared_ptr<CardModel>& card);
    void addHandCard(const std::shared_ptr<CardModel>& card);

    int allocateCardId();

    int findPlayfieldIndexById(int cardId) const;
    int findReserveIndexById(int cardId) const;
    int findHandIndexById(int cardId) const;

    // Serialization
    json toJson() const;
    static GameModel fromJson(const json& j);

private:
    std::vector<std::shared_ptr<CardModel>> _playfieldCards;
    std::vector<std::shared_ptr<CardModel>> _reserveCards;
    std::vector<std::shared_ptr<CardModel>> _handCards;

    int _nextCardId = 1;
};

#endif // GameModel_h