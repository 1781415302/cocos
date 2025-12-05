// test/Classes/models/GameModel.cpp
#include "GameModel.h"
#include "CardModel.h"
#include <algorithm> // for std::find_if

GameModel::GameModel()
{
    // 构造函数可以留空
}

std::vector<std::shared_ptr<CardModel>>& GameModel::getPlayfieldCards()
{
    return _playfieldCards;
}

const std::vector<std::shared_ptr<CardModel>>& GameModel::getPlayfieldCards() const
{
    return _playfieldCards;
}

std::vector<std::shared_ptr<CardModel>>& GameModel::getStackCards()
{
    return _stackCards;
}

const std::vector<std::shared_ptr<CardModel>>& GameModel::getStackCards() const
{
    return _stackCards;
}

void GameModel::addPlayfieldCard(const std::shared_ptr<CardModel>& card)
{
    // 如果 card 没有 id，则分配
    if (card->getId() == -1) {
        card->setId(allocateCardId());
    }
    _playfieldCards.push_back(card);
}

void GameModel::addStackCard(const std::shared_ptr<CardModel>& card)
{
    if (card->getId() == -1) {
        card->setId(allocateCardId());
    }
    _stackCards.push_back(card);
}

bool GameModel::movePlayfieldCardToStack(int playfieldIndex)
{
    if (playfieldIndex < 0 || playfieldIndex >= static_cast<int>(_playfieldCards.size())) {
        return false; // 索引无效
    }

    auto cardPtr = _playfieldCards[playfieldIndex];
    if (!cardPtr->isFaceUp() || !cardPtr->isVisible()) {
        return false; // 不能移动的卡
    }

    // 移动到 stack 的尾部 (作为顶牌)
    _stackCards.push_back(cardPtr);
    // 从 playfield 中移除
    _playfieldCards.erase(_playfieldCards.begin() + playfieldIndex);

    return true;
}

bool GameModel::flipTopStackCard()
{
    if (_stackCards.empty()) {
        return false;
    }

    auto& topCard = _stackCards.back();
    topCard->setFaceUp(true);
    return true;
}

bool GameModel::hasMovablePlayfieldCard() const
{
    return std::any_of(_playfieldCards.begin(), _playfieldCards.end(), [](const std::shared_ptr<CardModel>& card) {
        return card->isFaceUp() && card->isVisible();
        });
}

bool GameModel::canMatchWithTopStackCard() const
{
    if (_stackCards.empty()) {
        return false;
    }
    return _stackCards.back()->isFaceUp();
}

int GameModel::allocateCardId()
{
    return _nextCardId++;
}