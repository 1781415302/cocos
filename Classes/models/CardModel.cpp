// test/Classes/models/CardModel.cpp
#include "CardModel.h"

CardModel::CardModel(CardSuitType suit, CardFaceType face, cocos2d::Vec2 position, bool isFaceUp)
    : _suit(suit), _face(face), _position(position), _isFaceUp(isFaceUp), _isVisible(true)
{
    // 构造函数体
}

CardSuitType CardModel::getSuit() const
{
    return _suit;
}

CardFaceType CardModel::getFace() const
{
    return _face;
}

cocos2d::Vec2 CardModel::getPosition() const
{
    return _position;
}

void CardModel::setPosition(const cocos2d::Vec2& position)
{
    _position = position;
}

bool CardModel::isFaceUp() const
{
    return _isFaceUp;
}

void CardModel::setFaceUp(bool isFaceUp)
{
    _isFaceUp = isFaceUp;
}

bool CardModel::isVisible() const
{
    return _isVisible;
}

void CardModel::setVisible(bool visible)
{
    _isVisible = visible;
}