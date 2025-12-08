// Classes/models/CardModel.cpp
#include "CardModel.h"
#include <utils/json.hpp>

using json = nlohmann::json;

// 构造函数（带 id）
CardModel::CardModel(int id, CardFaceType faceType, CardSuitType suitType, cocos2d::Vec2 position, CardStatus status)
    : _id(id), _cardFace(faceType), _cardSuit(suitType), _position(position), _status(status), _visible(true)
{
}

// 兼容便捷构造函数（suit, face, position, isFaceUp）
CardModel::CardModel(CardSuitType suitType, CardFaceType faceType, cocos2d::Vec2 position, bool isFaceUp)
    : _id(-1), _cardFace(faceType), _cardSuit(suitType), _position(position),
    _status(isFaceUp ? CardStatus::EXPOSED : CardStatus::COVERED), _visible(true)
{
}

// Getter 实现
int CardModel::getId() const { return _id; }
void CardModel::setId(int id) { _id = id; }

CardFaceType CardModel::getCardFace() const { return _cardFace; }
CardSuitType CardModel::getCardSuit() const { return _cardSuit; }
cocos2d::Vec2 CardModel::getPosition() const { return _position; }
CardStatus CardModel::getStatus() const { return _status; }

bool CardModel::isFaceUp() const { return _status == CardStatus::EXPOSED; }
void CardModel::setFaceUp(bool faceUp) { _status = faceUp ? CardStatus::EXPOSED : CardStatus::COVERED; }

bool CardModel::isVisible() const { return _visible; }
void CardModel::setVisible(bool visible) { _visible = visible; }

void CardModel::setPosition(cocos2d::Vec2 pos) { _position = pos; }
void CardModel::setStatus(CardStatus status) { _status = status; }

// Serialization
json CardModel::toJson() const
{
    json j;
    j["id"] = _id;
    j["face"] = static_cast<int>(_cardFace);
    j["suit"] = static_cast<int>(_cardSuit);
    j["pos"] = { {"x", _position.x}, {"y", _position.y} };
    j["status"] = static_cast<int>(_status);
    j["visible"] = _visible;
    j["faceUp"] = isFaceUp();
    return j;
}

CardModel CardModel::fromJson(const json& j)
{
    int id = j.value("id", -1);
    CardFaceType face = static_cast<CardFaceType>(j.value("face", 0));
    CardSuitType suit = static_cast<CardSuitType>(j.value("suit", 0));
    float x = 0.0f, y = 0.0f;
    if (j.contains("pos")) {
        x = j["pos"].value("x", 0.0f);
        y = j["pos"].value("y", 0.0f);
    }
    CardStatus status = static_cast<CardStatus>(j.value("status", static_cast<int>(CardStatus::COVERED)));
    CardModel cm(id, face, suit, cocos2d::Vec2(x, y), status);
    cm.setVisible(j.value("visible", true));
    return cm;
}