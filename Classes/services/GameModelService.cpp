#include "services/GameModelService.h"
#include "models/CardModel.h"
#include <algorithm>

using namespace cocos2d;

bool GameModelService::drawReserveToHand(GameModel& model)
{
    auto& reserve = model.getReserveCards();
    if (reserve.empty()) return false;

    auto cardPtr = reserve.back();
    reserve.pop_back();

    cardPtr->setFaceUp(true); // hand 中应保持正面
    model.getHandCards().push_back(cardPtr);
    return true;
}

bool GameModelService::movePlayfieldCardToHand(GameModel& model, int playfieldIndex)
{
    auto& playfield = model.getPlayfieldCards();
    if (playfieldIndex < 0 || playfieldIndex >= static_cast<int>(playfield.size())) {
        return false;
    }

    auto cardPtr = playfield[playfieldIndex];
    if (!cardPtr->isFaceUp() || !cardPtr->isVisible()) {
        return false;
    }

    model.getHandCards().push_back(cardPtr);
    playfield.erase(playfield.begin() + playfieldIndex);
    return true;
}

bool GameModelService::moveTopHandCardToPlayfieldAt(GameModel& model, int playfieldIndex, Vec2 position, CardStatus status)
{
    bool visible = true;
    bool faceUp = false;
    auto& hand = model.getHandCards();
    if (!hand.empty()) {
        faceUp = hand.back()->isFaceUp();
    }
    return moveTopHandCardToPlayfieldAt(model, playfieldIndex, position, status, visible, faceUp);
}

bool GameModelService::moveTopHandCardToPlayfieldAt(GameModel& model, int playfieldIndex, Vec2 position, CardStatus status, bool visible)
{
    bool faceUp = false;
    auto& hand = model.getHandCards();
    if (!hand.empty()) {
        faceUp = hand.back()->isFaceUp();
    }
    return moveTopHandCardToPlayfieldAt(model, playfieldIndex, position, status, visible, faceUp);
}

bool GameModelService::moveTopHandCardToPlayfieldAt(GameModel& model, int playfieldIndex, Vec2 position, CardStatus status, bool visible, bool faceUp)
{
    auto& hand = model.getHandCards();
    if (hand.empty()) return false;

    auto cardPtr = hand.back();
    hand.pop_back();

    cardPtr->setPosition(position);
    cardPtr->setStatus(status);
    cardPtr->setVisible(visible);
    cardPtr->setFaceUp(faceUp);

    auto& playfield = model.getPlayfieldCards();
    if (playfieldIndex < 0 || playfieldIndex > static_cast<int>(playfield.size())) {
        playfield.push_back(cardPtr);
    }
    else {
        playfield.insert(playfield.begin() + playfieldIndex, cardPtr);
    }
    return true;
}

bool GameModelService::flipTopHandCard(GameModel& model)
{
    auto& hand = model.getHandCards();
    if (hand.empty()) return false;
    hand.back()->setFaceUp(true);
    return true;
}

bool GameModelService::moveTopHandCardBackToReserve(GameModel& model, Vec2 position, CardStatus status, bool visible)
{
    bool faceUp = false;
    auto& hand = model.getHandCards();
    if (!hand.empty()) {
        faceUp = hand.back()->isFaceUp();
    }
    return moveTopHandCardBackToReserve(model, position, status, visible, faceUp);
}

bool GameModelService::moveTopHandCardBackToReserve(GameModel& model, Vec2 position, CardStatus status, bool visible, bool faceUp)
{
    auto& hand = model.getHandCards();
    if (hand.empty()) return false;

    auto cardPtr = hand.back();
    hand.pop_back();

    cardPtr->setPosition(position);
    cardPtr->setStatus(status);
    cardPtr->setVisible(visible);
    cardPtr->setFaceUp(faceUp);

    model.getReserveCards().push_back(cardPtr); // 返回 reserve 底部
    return true;
}

bool GameModelService::hasMovablePlayfieldCard(const GameModel& model)
{
    const auto& playfield = model.getPlayfieldCards();
    return std::any_of(playfield.begin(), playfield.end(), [](const std::shared_ptr<CardModel>& card) {
        return card->isFaceUp() && card->isVisible();
        });
}

bool GameModelService::canMatchWithHandTop(const GameModel& model)
{
    const auto& hand = model.getHandCards();
    if (hand.empty()) return false;
    return hand.back()->isFaceUp();
}