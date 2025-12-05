// Classes/models/CardModel.cpp
#include "CardModel.h"

// 构造函数实现
CardModel::CardModel(int id, CardFaceType faceType, CardSuitType suitType, cocos2d::Vec2 position, CardStatus status)
    : _id(id), _cardFace(faceType), _cardSuit(suitType), _position(position), _status(status)
{
    // 构造函数体可以为空，因为所有成员都在初始化列表中初始化了
}

// Getter 实现
int CardModel::getId() const
{
    return _id;
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

// Setter 实现
void CardModel::setPosition(cocos2d::Vec2 pos)
{
    _position = pos;
}

void CardModel::setStatus(CardStatus status)
{
    _status = status;
}