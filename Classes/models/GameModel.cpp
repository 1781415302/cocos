#include "GameModel.h"
#include "CardModel.h"
#include <algorithm>

using namespace cocos2d;

GameModel::GameModel()
{
    // 默认初始化
}

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

bool GameModel::drawReserveToHand()
{
    if (_reserveCards.empty()) return false;

    auto cardPtr = _reserveCards.back();
    _reserveCards.pop_back();

    cardPtr->setFaceUp(true); // 抽到 hand 末尾，并翻开
    _handCards.push_back(cardPtr);
    return true;
}

bool GameModel::movePlayfieldCardToHand(int playfieldIndex)
{
    if (playfieldIndex < 0 || playfieldIndex >= static_cast<int>(_playfieldCards.size())) {
        return false;
    }

    auto cardPtr = _playfieldCards[playfieldIndex];
    if (!cardPtr->isFaceUp() || !cardPtr->isVisible()) {
        return false;
    }

    _handCards.push_back(cardPtr);
    _playfieldCards.erase(_playfieldCards.begin() + playfieldIndex);
    return true;
}

bool GameModel::moveTopHandCardToPlayfieldAt(int playfieldIndex, Vec2 position, CardStatus status)
{
    // 默认 visible=true，faceUp 保持当前
    if (_handCards.empty()) return false;
    bool visible = true;
    bool faceUp = _handCards.back()->isFaceUp();
    return moveTopHandCardToPlayfieldAt(playfieldIndex, position, status, visible, faceUp);
}

bool GameModel::moveTopHandCardToPlayfieldAt(int playfieldIndex, Vec2 position, CardStatus status, bool visible)
{
    // faceUp 保持当前
    if (_handCards.empty()) return false;
    bool faceUp = _handCards.back()->isFaceUp();
    return moveTopHandCardToPlayfieldAt(playfieldIndex, position, status, visible, faceUp);
}

bool GameModel::moveTopHandCardToPlayfieldAt(int playfieldIndex, Vec2 position, CardStatus status, bool visible, bool faceUp)
{
    if (_handCards.empty()) return false;

    auto cardPtr = _handCards.back();
    _handCards.pop_back();

    cardPtr->setPosition(position);
    cardPtr->setStatus(status);
    cardPtr->setVisible(visible);
    cardPtr->setFaceUp(faceUp);

    if (playfieldIndex < 0 || playfieldIndex > static_cast<int>(_playfieldCards.size())) {
        _playfieldCards.push_back(cardPtr);
    }
    else {
        _playfieldCards.insert(_playfieldCards.begin() + playfieldIndex, cardPtr);
    }
    return true;
}

bool GameModel::flipTopHandCard()
{
    if (_handCards.empty()) return false;
    _handCards.back()->setFaceUp(true);
    return true;
}

bool GameModel::moveTopHandCardBackToReserve(Vec2 position, CardStatus status, bool visible)
{
    // faceUp 保持当前
    if (_handCards.empty()) return false;
    bool faceUp = _handCards.back()->isFaceUp();
    return moveTopHandCardBackToReserve(position, status, visible, faceUp);
}

bool GameModel::moveTopHandCardBackToReserve(Vec2 position, CardStatus status, bool visible, bool faceUp)
{
    if (_handCards.empty()) return false;

    auto cardPtr = _handCards.back();
    _handCards.pop_back();

    cardPtr->setPosition(position);
    cardPtr->setStatus(status);
    cardPtr->setVisible(visible);
    cardPtr->setFaceUp(faceUp);

    _reserveCards.push_back(cardPtr); // 退回 reserve 尾部
    return true;
}

bool GameModel::hasMovablePlayfieldCard() const
{
    return std::any_of(_playfieldCards.begin(), _playfieldCards.end(), [](const std::shared_ptr<CardModel>& card) {
        return card->isFaceUp() && card->isVisible();
        });
}

bool GameModel::canMatchWithHandTop() const
{
    if (_handCards.empty()) return false;
    return _handCards.back()->isFaceUp();
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