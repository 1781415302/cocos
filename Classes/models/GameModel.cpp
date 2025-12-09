#include "GameModel.h"
#include "CardModel.h"
#include <utils/json.hpp>

using namespace cocos2d;
using json = nlohmann::json;

GameModel::GameModel()
{
}

// existing getters/setters unchanged...
std::vector<std::shared_ptr<CardModel>>& GameModel::getPlayfieldCards() { return _playfieldCards; }
const std::vector<std::shared_ptr<CardModel>>& GameModel::getPlayfieldCards() const { return _playfieldCards; }

std::vector<std::shared_ptr<CardModel>>& GameModel::getReserveCards() { return _reserveCards; }
const std::vector<std::shared_ptr<CardModel>>& GameModel::getReserveCards() const { return _reserveCards; }

std::vector<std::shared_ptr<CardModel>>& GameModel::getHandCards() { return _handCards; }
const std::vector<std::shared_ptr<CardModel>>& GameModel::getHandCards() const { return _handCards; }

std::vector<std::shared_ptr<CardModel>>& GameModel::getStackCards() { return getHandCards(); }
const std::vector<std::shared_ptr<CardModel>>& GameModel::getStackCards() const { return getHandCards(); }

void GameModel::addPlayfieldCard(const std::shared_ptr<CardModel>& card)
{
    if (card->getId() == -1) {
        card->setId(allocateCardId());
    }
    _playfieldCards.push_back(card);
}

void GameModel::addReserveCard(const std::shared_ptr<CardModel>& card)
{
    if (card->getId() == -1) {
        card->setId(allocateCardId());
    }
    _reserveCards.push_back(card);
}

void GameModel::addHandCard(const std::shared_ptr<CardModel>& card)
{
    if (card->getId() == -1) {
        card->setId(allocateCardId());
    }
    _handCards.push_back(card);
}

int GameModel::allocateCardId()
{
    return _nextCardId++;
}

int GameModel::findPlayfieldIndexById(int cardId) const
{
    for (size_t i = 0; i < _playfieldCards.size(); ++i) {
        if (_playfieldCards[i] && _playfieldCards[i]->getId() == cardId) return static_cast<int>(i);
    }
    return -1;
}

int GameModel::findReserveIndexById(int cardId) const
{
    for (size_t i = 0; i < _reserveCards.size(); ++i) {
        if (_reserveCards[i] && _reserveCards[i]->getId() == cardId) return static_cast<int>(i);
    }
    return -1;
}

int GameModel::findHandIndexById(int cardId) const
{
    for (size_t i = 0; i < _handCards.size(); ++i) {
        if (_handCards[i] && _handCards[i]->getId() == cardId) return static_cast<int>(i);
    }
    return -1;
}

// Serialization
json GameModel::toJson() const
{
    json j;
    j["version"] = 1;
    // levelId should be filled by caller if needed
    j["nextCardId"] = _nextCardId;

    auto arr = json::array();
    for (const auto& c : _playfieldCards) {
        if (c) arr.push_back(c->toJson());
    }
    j["playfield"] = arr;

    arr = json::array();
    for (const auto& c : _reserveCards) {
        if (c) arr.push_back(c->toJson());
    }
    j["reserve"] = arr;

    arr = json::array();
    for (const auto& c : _handCards) {
        if (c) arr.push_back(c->toJson());
    }
    j["hand"] = arr;

    return j;
}

GameModel GameModel::fromJson(const json& j)
{
    GameModel gm;
    gm._nextCardId = j.value("nextCardId", 1);

    if (j.contains("playfield") && j["playfield"].is_array()) {
        for (const auto& el : j["playfield"]) {
            CardModel cm = CardModel::fromJson(el);
            auto p = std::make_shared<CardModel>(cm);
            gm._playfieldCards.push_back(p);
        }
    }
    if (j.contains("reserve") && j["reserve"].is_array()) {
        for (const auto& el : j["reserve"]) {
            CardModel cm = CardModel::fromJson(el);
            auto p = std::make_shared<CardModel>(cm);
            gm._reserveCards.push_back(p);
        }
    }
    if (j.contains("hand") && j["hand"].is_array()) {
        for (const auto& el : j["hand"]) {
            CardModel cm = CardModel::fromJson(el);
            auto p = std::make_shared<CardModel>(cm);
            gm._handCards.push_back(p);
        }
    }

    return gm;
}