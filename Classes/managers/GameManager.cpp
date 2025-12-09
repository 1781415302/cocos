// Classes/managers/GameManager.cpp
#include "GameManager.h"
#include "models/CardModel.h"

using namespace cocos2d;

// ππ‘Ï
GameManager::GameManager(GameModel& model, UndoModel* undoModel)
    : _model(model), _undoModel(undoModel)
{
}

// allocate id
int GameManager::allocateCardId()
{
    int id = _model.getNextCardId();
    _model.setNextCardId(id + 1);
    return id;
}

// adds
void GameManager::addPlayfieldCard(const std::shared_ptr<CardModel>& card)
{
    if (!card) return;
    if (card->getId() == -1) card->setId(allocateCardId());
    _model.getPlayfieldCards().push_back(card);
}

void GameManager::addReserveCard(const std::shared_ptr<CardModel>& card)
{
    if (!card) return;
    if (card->getId() == -1) card->setId(allocateCardId());
    _model.getReserveCards().push_back(card);
}

void GameManager::addHandCard(const std::shared_ptr<CardModel>& card)
{
    if (!card) return;
    if (card->getId() == -1) card->setId(allocateCardId());
    _model.getHandCards().push_back(card);
}

// draw reserve -> hand
bool GameManager::drawReserveToHand()
{
    auto& reserve = _model.getReserveCards();
    if (reserve.empty()) return false;

    auto cardPtr = reserve.back();
    reserve.pop_back();

    cardPtr->setFaceUp(true);
    _model.getHandCards().push_back(cardPtr);

    if (_undoModel) {
        auto action = UndoModel::makeDrawReserveToHand(*cardPtr);
        _undoModel->push(action);
    }
    return true;
}

// move playfield -> hand (by index)
bool GameManager::movePlayfieldCardToHand(int playfieldIndex)
{
    auto& pf = _model.getPlayfieldCards();
    if (playfieldIndex < 0 || playfieldIndex >= static_cast<int>(pf.size())) return false;
    auto cardPtr = pf[playfieldIndex];
    if (!cardPtr) return false;
    if (!cardPtr->isFaceUp() || !cardPtr->isVisible()) {
        return false;
    }

    // º«¬º action before change
    if (_undoModel) {
        auto a = UndoModel::makeMovePlayfieldToHand(*cardPtr, playfieldIndex);
        _undoModel->push(a);
    }

    _model.getHandCards().push_back(cardPtr);
    pf.erase(pf.begin() + playfieldIndex);
    return true;
}

// move top hand card to playfield at index (inserts at index or push_back if index out of range)
bool GameManager::moveTopHandCardToPlayfieldAt(int playfieldIndex, Vec2 position, CardStatus status, bool visible, bool faceUp)
{
    auto& hand = _model.getHandCards();
    if (hand.empty()) return false;

    auto cardPtr = hand.back();
    // record action
    if (_undoModel) {
        auto a = UndoModel::makeMoveHandToPlayfield(*cardPtr, playfieldIndex);
        _undoModel->push(a);
    }

    hand.pop_back();

    cardPtr->setPosition(position);
    cardPtr->setStatus(status);
    cardPtr->setVisible(visible);
    cardPtr->setFaceUp(faceUp);

    auto& pf = _model.getPlayfieldCards();
    if (playfieldIndex < 0 || playfieldIndex > static_cast<int>(pf.size())) {
        pf.push_back(cardPtr);
    }
    else {
        pf.insert(pf.begin() + playfieldIndex, cardPtr);
    }
    return true;
}

// flip top hand card -> faceUp
bool GameManager::flipTopHandCard()
{
    auto& hand = _model.getHandCards();
    if (hand.empty()) return false;

    // record action (capture previous state)
    if (_undoModel) {
        auto a = UndoModel::makeFlipHandTop(*hand.back());
        _undoModel->push(a);
    }

    hand.back()->setFaceUp(true);
    return true;
}

// move top hand card back to reserve (and restore properties)
bool GameManager::moveTopHandCardBackToReserve(Vec2 position, CardStatus status, bool visible, bool faceUp)
{
    auto& hand = _model.getHandCards();
    if (hand.empty()) return false;

    auto cardPtr = hand.back();
    hand.pop_back();

    cardPtr->setPosition(position);
    cardPtr->setStatus(status);
    cardPtr->setVisible(visible);
    cardPtr->setFaceUp(faceUp);

    _model.getReserveCards().push_back(cardPtr);
    if (_undoModel) {
        // this is inverse of DrawReserveToHand, but when recording a "manual" push we typically record before performing
        // For safety we can record the action that was undone (optional)
        // Here we don't auto-record moveTopHandCardBackToReserve because it is typically used during undo.
    }
    return true;
}

// queries
bool GameManager::hasMovablePlayfieldCard() const
{
    const auto& pf = _model.getPlayfieldCards();
    for (const auto& c : pf) {
        if (c && c->isFaceUp() && c->isVisible()) return true;
    }
    return false;
}

bool GameManager::canMatchWithHandTop() const
{
    const auto& hand = _model.getHandCards();
    if (hand.empty()) return false;
    return hand.back()->isFaceUp();
}

int GameManager::findPlayfieldIndexById(int cardId) const
{
    const auto& pf = _model.getPlayfieldCards();
    for (size_t i = 0; i < pf.size(); ++i) {
        if (pf[i] && pf[i]->getId() == cardId) return static_cast<int>(i);
    }
    return -1;
}

int GameManager::findReserveIndexById(int cardId) const
{
    const auto& r = _model.getReserveCards();
    for (size_t i = 0; i < r.size(); ++i) {
        if (r[i] && r[i]->getId() == cardId) return static_cast<int>(i);
    }
    return -1;
}

int GameManager::findHandIndexById(int cardId) const
{
    const auto& h = _model.getHandCards();
    for (size_t i = 0; i < h.size(); ++i) {
        if (h[i] && h[i]->getId() == cardId) return static_cast<int>(i);
    }
    return -1;
}

// private helper
void GameManager::recordAction(const UndoModel::Action& a)
{
    if (_undoModel) _undoModel->push(a);
}