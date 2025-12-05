// test/Classes/models/GameModel.cpp
#include "GameModel.h"
#include "CardModel.h"
#include <algorithm> // for std::find_if

GameModel::GameModel()
{
    // 构造函数体
}

std::vector<CardModel>& GameModel::getPlayfieldCards()
{
    return _playfieldCards;
}

const std::vector<CardModel>& GameModel::getPlayfieldCards() const
{
    return _playfieldCards;
}

std::vector<CardModel>& GameModel::getStackCards()
{
    return _stackCards;
}

const std::vector<CardModel>& GameModel::getStackCards() const
{
    return _stackCards;
}

bool GameModel::movePlayfieldCardToStack(int playfieldIndex)
{
    if (playfieldIndex < 0 || playfieldIndex >= static_cast<int>(_playfieldCards.size())) {
        return false; // 索引无效
    }

    auto& card = _playfieldCards[playfieldIndex];
    if (!card.isFaceUp() || !card.isVisible()) {
        return false; // 牌未翻开或不可见，无法移动
    }

    // 移动牌到备用牌堆顶部
    _stackCards.push_back(card);
    // 从主牌区移除该牌
    _playfieldCards.erase(_playfieldCards.begin() + playfieldIndex);

    // 检查是否需要更新被移走牌下方的牌的可见性
    // (这里简化处理，假设所有牌都是独立放置，没有层级覆盖关系)
    // 如果有覆盖逻辑，需要在此处更新下方牌的 _isVisible 状态

    return true;
}

bool GameModel::flipTopStackCard()
{
    if (_stackCards.empty()) {
        return false; // 牌堆为空，无法翻牌
    }

    // 翻开备用牌堆顶部的牌
    auto& topCard = _stackCards.back();
    topCard.setFaceUp(true);
    return true;
}

bool GameModel::hasMovablePlayfieldCard() const
{
    // 检查是否存在翻开且可见的牌
    return std::any_of(_playfieldCards.begin(), _playfieldCards.end(), [](const CardModel& card) {
        return card.isFaceUp() && card.isVisible();
    });
}

bool GameModel::canMatchWithTopStackCard() const
{
    if (_stackCards.empty()) {
        return false; // 牌堆为空，无法匹配
    }
    // 检查备用牌堆顶部的牌是否已翻开
    return _stackCards.back().isFaceUp();
}