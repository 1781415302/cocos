// Classes/models/CardModel.cpp
#include "CardModel.h"

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
int CardModel::getId() const
{
    return _id;
}

void CardModel::setId(int id)
{
    _id = id;
}

CardFaceType CardModel::getCardFace() const
{
    return _cardFace;
}

CardSuitType CardModel::getCardSuit() const
{
    return _cardSuit;
}

cocos2d::Vec2 CardModel::getPosition() const
{
    return _position;
}

CardStatus CardModel::getStatus() const
{
    return _status;
}

// 状态查询/设置
bool CardModel::isFaceUp() const
{
    return _status == CardStatus::EXPOSED;
}

void CardModel::setFaceUp(bool faceUp)
{
    _status = faceUp ? CardStatus::EXPOSED : CardStatus::COVERED;
}

bool CardModel::isVisible() const
{
    return _visible;
}

void CardModel::setVisible(bool visible)
{
    _visible = visible;
}

// Setter 实现
void CardModel::setPosition(cocos2d::Vec2 pos)
{
    _position = pos;
}

void CardModel::setStatus(CardStatus status)
{
    _status = status;
}