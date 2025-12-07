#pragma once
#include "GameController.h"
#include "configs/Loaders/LevelConfigLoader.h"
#include "services/GameModelFromLevelGenerator.h"
#include "models/CardModel.h"
#include "models/UndoModel.h"
#include "utils/CardEnums.h"
#include <cassert>
#include <algorithm>

using namespace cocos2d;

GameController::GameController(Node* parentNode)
    : _parentNode(parentNode)
{
    assert(parentNode && "GameController requires a valid parent node");

    // Attach playfield / reserve / hand nodes to parent; set Z-order
    _playfieldNode = Node::create();
    _reserveNode = Node::create();
    _handNode = Node::create();

    _playfieldNode->setName("playfield_node");
    _reserveNode->setName("reserve_node");
    _handNode->setName("hand_node");

    _parentNode->addChild(_playfieldNode, 0);
    _parentNode->addChild(_reserveNode, 5);
    _parentNode->addChild(_handNode, 10);
}

GameController::~GameController()
{
    reset();
    _playfieldNode = nullptr;
    _reserveNode = nullptr;
    _handNode = nullptr;
}

void GameController::startGame(const std::string& levelId)
{
    reset();
    _undoModel.clear();

    // 1) Load level config
    LevelConfig config = LevelConfigLoader::loadLevelConfig(levelId);

    // 2) Build model
    _gameModel = GameModelFromLevelGenerator::generateGameModel(config);

    // 3) Build views from model
    createViewsFromModel();

    // 4) Auto draw the top reserve card to hand (no animation)
    drawInitialReserveTopToHand(false);
}

void GameController::createViewsFromModel()
{
    // Playfield
    const auto& playfield = _gameModel.getPlayfieldCards();
    for (size_t i = 0; i < playfield.size(); ++i) {
        auto cardPtr = playfield[i];
        if (!cardPtr) continue;
        CardView* v = CardView::create(cardPtr);
        if (!v) continue;
        _playfieldNode->addChild(v);
        _cardViews[cardPtr->getId()] = v;

        v->setClickCallback([this](int cardId) {
            this->handlePlayfieldCardClick(cardId);
            });

        // Sync face-up with model
        v->setFaceUp(cardPtr->isFaceUp(), false);
    }

    // After creating playfield views, recompute coverage
    updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);

    // Reserve
    const auto& reserve = _gameModel.getReserveCards();
    for (size_t i = 0; i < reserve.size(); ++i) {
        auto cardPtr = reserve[i];
        if (!cardPtr) continue;
        CardView* v = CardView::create(cardPtr);
        if (!v) continue;

        _reserveNode->addChild(v);
        _cardViews[cardPtr->getId()] = v;

        v->setClickCallback([this](int /*cardId*/) {
            this->handleReserveClick();
            });

        v->setCardVisible(cardPtr->isFaceUp());
    }

    // Hand
    const auto& hand = _gameModel.getHandCards();
    for (size_t i = 0; i < hand.size(); ++i) {
        auto cardPtr = hand[i];
        if (!cardPtr) continue;
        CardView* v = CardView::create(cardPtr);
        if (!v) continue;
        _handNode->addChild(v);
        _cardViews[cardPtr->getId()] = v;

        v->setClickCallback([this](int /*cardId*/) {
            // no-op
            });

        v->setCardVisible(cardPtr->isFaceUp());
    }
}

/**
 * Recalculate playfield coverage: a card is face-down if any later card overlaps it (simple top-order overlap).
 */
void GameController::updatePlayfieldCoverage(float overlapAreaThreshold)
{
    const auto& playfield = _gameModel.getPlayfieldCards();
    const size_t n = playfield.size();
    if (n == 0) return;

    auto rectInPlayfield = [this](CardView* v) -> Rect {
        if (!v) return Rect::ZERO;
        Rect b = v->getBoundingBox();
        Node* parent = v->getParent();
        if (!parent || !_playfieldNode) return b;

        Vec2 originWorld = parent->convertToWorldSpace(Vec2(b.origin.x, b.origin.y));
        Vec2 originInPlay = _playfieldNode->convertToNodeSpace(originWorld);
        return Rect(originInPlay.x, originInPlay.y, b.size.width, b.size.height);
        };

    std::vector<Rect> rects;
    rects.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        auto cardPtr = playfield[i];
        Rect r = Rect::ZERO;
        if (cardPtr) {
            auto it = _cardViews.find(cardPtr->getId());
            if (it != _cardViews.end() && it->second) {
                r = rectInPlayfield(it->second);
            }
        }
        rects.push_back(r);
    }

    for (size_t i = 0; i < n; ++i) {
        bool covered = false;
        const Rect& a = rects[i];
        if (!a.equals(Rect::ZERO)) {
            for (size_t j = i + 1; j < n; ++j) {
                const Rect& b = rects[j];
                if (b.equals(Rect::ZERO)) continue;

                float xi1 = std::max(a.getMinX(), b.getMinX());
                float xi2 = std::min(a.getMaxX(), b.getMaxX());
                float yi1 = std::max(a.getMinY(), b.getMinY());
                float yi2 = std::min(a.getMaxY(), b.getMaxY());
                if (xi2 > xi1 && yi2 > yi1) {
                    float interArea = (xi2 - xi1) * (yi2 - yi1);
                    if (interArea > overlapAreaThreshold) {
                        covered = true;
                        break;
                    }
                }
            }
        }
        auto cardPtr = playfield[i];
        if (cardPtr) {
            cardPtr->setFaceUp(!covered);
            auto itv = _cardViews.find(cardPtr->getId());
            if (itv != _cardViews.end() && itv->second) {
                itv->second->setFaceUp(!covered, true);
            }
        }
    }
}

/**
 * Auto draw from reserve to hand. When animate=false, perform an immediate model/view sync (used at start).
 */
void GameController::drawInitialReserveTopToHand(bool animate)
{
    if (animate) {
        // Animated path
        if (!_busy) {
            _busy = true;
            animateReserveTopToHand();
            // animateReserveTopToHand resets _busy
        }
        return;
    }

    // Non-animated path for init
    const auto& reserve = _gameModel.getReserveCards();
    if (reserve.empty()) return;

    bool ok = _gameModel.drawReserveToHand();
    if (!ok) return;

    const auto& handRef = _gameModel.getHandCards();
    if (handRef.empty()) return;
    auto moved = handRef.back();
    if (!moved) return;

    // Position in design-space
    Vec2 targetPos = _defaultHandPosition;
    if (!handRef.empty() && handRef.size() > 1) {
        targetPos = handRef[handRef.size() - 2]->getPosition();
    }
    moved->setPosition(targetPos);
    moved->setFaceUp(true);

    // Reparent view to hand
    auto itv = _cardViews.find(moved->getId());
    if (itv != _cardViews.end()) {
        CardView* v = itv->second;
        if (v) {
            Vec2 worldTarget = targetPos;
            if (_parentNode) {
                worldTarget = _parentNode->convertToWorldSpace(targetPos);
            }
            Vec2 handLocal = _handNode ? _handNode->convertToNodeSpace(worldTarget) : worldTarget;
            if (v->getParent() != _handNode) {
                v->removeFromParent();
                if (_handNode) _handNode->addChild(v);
            }
            v->setPosition(handLocal);
            v->setCardVisible(true);
            // Disable click on hand cards
            v->setClickCallback([](int /*cardId*/) {});
        }
    }

    // Ensure playfield coverage is consistent
    updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);
}

int GameController::findPlayfieldIndexByCardId(int cardId) const
{
    return _gameModel.findPlayfieldIndexById(cardId);
}

int GameController::findReserveIndexByCardId(int cardId) const
{
    return _gameModel.findReserveIndexById(cardId);
}

int GameController::findHandIndexByCardId(int cardId) const
{
    return _gameModel.findHandIndexById(cardId);
}

bool GameController::facesAreAdjacent(CardFaceType a, CardFaceType b) const
{
    if (a == CardFaceType::CFT_NONE || b == CardFaceType::CFT_NONE) return false;
    int ia = static_cast<int>(a);
    int ib = static_cast<int>(b);
    return std::abs(ia - ib) == 1;
}

void GameController::handlePlayfieldCardClick(int cardId)
{
    if (_busy) {
        CCLOG("GameController::handlePlayfieldCardClick - busy, ignore click");
        return;
    }

    int playIndex = findPlayfieldIndexByCardId(cardId);
    if (playIndex < 0) {
        CCLOG("GameController::handlePlayfieldCardClick - not found cardId %d in playfield", cardId);
        return;
    }

    auto cardPtr = _gameModel.getPlayfieldCards()[playIndex];
    if (!cardPtr) return;

    if (!cardPtr->isFaceUp() || !cardPtr->isVisible()) {
        CCLOG("GameController::handlePlayfieldCardClick - card not movable id=%d", cardId);
        return;
    }

    // Check hand top can match
    if (!_gameModel.canMatchWithHandTop()) {
        CCLOG("GameController::handlePlayfieldCardClick - no matching hand top");
        return;
    }

    auto handTop = _gameModel.getHandCards().back();
    if (!handTop) return;

    if (!facesAreAdjacent(cardPtr->getCardFace(), handTop->getCardFace())) {
        CCLOG("GameController::handlePlayfieldCardClick - faces not adjacent cardId=%d", cardId);
        return;
    }

    // Begin move playfield -> hand
    _busy = true;
    animatePlayfieldCardToHand(playIndex, cardId);
}

void GameController::animatePlayfieldCardToHand(int playfieldIndex, int cardId)
{
    auto it = _cardViews.find(cardId);
    if (it == _cardViews.end()) {
        CCLOG("GameController::animatePlayfieldCardToHand - CardView not found for id %d", cardId);
        _busy = false;
        return;
    }
    CardView* view = it->second;

    // Target position = current hand top (stacked)
    Vec2 targetPos = _defaultHandPosition;
    const auto& hand = _gameModel.getHandCards();
    if (!hand.empty()) {
        targetPos = hand.back()->getPosition();
    }

    // design-space -> world
    Vec2 worldTarget = targetPos;
    if (_parentNode) {
        worldTarget = _parentNode->convertToWorldSpace(targetPos);
    }

    Node* currentParent = view->getParent();
    if (!currentParent) {
        CCLOG("GameController::animatePlayfieldCardToHand - view has no parent, aborting");
        _busy = false;
        return;
    }
    Vec2 localTargetForCurrentParent = currentParent->convertToNodeSpace(worldTarget);

    // Record Undo before model mutation
    UndoModel::Action undoAction = UndoModel::makeMovePlayfieldToHand(*_gameModel.getPlayfieldCards()[playfieldIndex], playfieldIndex);

    view->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, playfieldIndex, cardId, worldTarget, targetPos, undoAction]() mutable {
        // After animation: update model
        bool ok = _gameModel.movePlayfieldCardToHand(playfieldIndex);
        if (!ok) {
            CCLOG("GameController::animatePlayfieldCardToHand - model move failed for id %d", cardId);
            _busy = false;
            return;
        }

        // Sync moved card model state
        const auto& handRef = _gameModel.getHandCards();
        if (handRef.empty()) {
            CCLOG("GameController::animatePlayfieldCardToHand - model reports empty hand after move (cardId=%d)", cardId);
            _busy = false;
            return;
        }
        auto moved = handRef.back();
        if (!moved) {
            CCLOG("GameController::animatePlayfieldCardToHand - moved card ptr null (cardId=%d)", cardId);
            _busy = false;
            return;
        }
        moved->setPosition(targetPos);
        moved->setFaceUp(true);

        // Reparent view to hand
        auto itv = _cardViews.find(moved->getId());
        if (itv != _cardViews.end()) {
            CardView* v = itv->second;
            if (v) {
                Vec2 handLocal = _handNode ? _handNode->convertToNodeSpace(worldTarget) : worldTarget;
                if (v->getParent() != _handNode) {
                    v->removeFromParent();
                    if (_handNode) _handNode->addChild(v);
                }
                v->setPosition(handLocal);
                v->setCardVisible(true);
                // Disable click on hand cards
                v->setClickCallback([](int /*cardId*/) {});
            }
        }

        // Push Undo
        _undoModel.push(undoAction);

        // Recompute coverage
        updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);

        _busy = false;
        });
}

void GameController::handleReserveClick()
{
    if (_busy) {
        CCLOG("GameController::handleReserveClick - busy, ignore");
        return;
    }

    const auto& reserve = _gameModel.getReserveCards();
    if (reserve.empty()) {
        CCLOG("GameController::handleReserveClick - reserve empty");
        return;
    }

    _busy = true;
    animateReserveTopToHand();
}

void GameController::animateReserveTopToHand()
{
    const auto& reserve = _gameModel.getReserveCards();
    if (reserve.empty()) {
        _busy = false;
        return;
    }
    auto cardPtr = reserve.back();
    if (!cardPtr) {
        _busy = false;
        return;
    }
    int cardId = cardPtr->getId();

    auto it = _cardViews.find(cardId);
    if (it == _cardViews.end()) {
        CCLOG("GameController::animateReserveTopToHand - CardView not found id %d", cardId);
        _busy = false;
        return;
    }
    CardView* view = it->second;

    Vec2 targetPos = _defaultHandPosition;
    const auto& hand = _gameModel.getHandCards();
    if (!hand.empty()) {
        targetPos = hand.back()->getPosition();
    }

    Vec2 worldTarget = targetPos;
    if (_parentNode) {
        worldTarget = _parentNode->convertToWorldSpace(targetPos);
    }

    Node* currentParent = view->getParent();
    if (!currentParent) {
        CCLOG("GameController::animateReserveTopToHand - view has no parent, aborting");
        _busy = false;
        return;
    }
    Vec2 localTargetForCurrentParent = currentParent->convertToNodeSpace(worldTarget);

    // Record Undo
    UndoModel::Action undoAction = UndoModel::makeDrawReserveToHand(*cardPtr);

    view->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, cardId, worldTarget, targetPos, undoAction]() mutable {
        bool ok = _gameModel.drawReserveToHand();
        if (!ok) {
            CCLOG("GameController::animateReserveTopToHand - model draw failed for id %d", cardId);
            _busy = false;
            return;
        }

        const auto& handRef = _gameModel.getHandCards();
        if (!handRef.empty()) {
            auto moved = handRef.back();
            moved->setPosition(targetPos);
            moved->setFaceUp(true);

            auto itv = _cardViews.find(moved->getId());
            if (itv != _cardViews.end()) {
                CardView* v = itv->second;
                if (v) {
                    Vec2 handLocal = _handNode ? _handNode->convertToNodeSpace(worldTarget) : worldTarget;
                    if (v->getParent() != _handNode) {
                        v->removeFromParent();
                        if (_handNode) _handNode->addChild(v);
                    }
                    v->setPosition(handLocal);
                    v->setCardVisible(true);
                    // Disable click on hand cards
                    v->setClickCallback([](int /*cardId*/) {});
                }
            }
        }

        // Push Undo
        _undoModel.push(undoAction);

        _busy = false;
        });
}

void GameController::handleUndo()
{
    if (_busy) {
        CCLOG("GameController::handleUndo - busy, ignore");
        return;
    }

    UndoModel::Action action;
    if (!_undoModel.pop(action)) {
        CCLOG("GameController::handleUndo - undo stack empty");
        return;
    }

    auto toWorld = [this](Vec2 designPos) {
        return _parentNode ? _parentNode->convertToWorldSpace(designPos) : designPos;
        };

    auto toLocal = [](Node* node, Vec2 worldPos) {
        return node ? node->convertToNodeSpace(worldPos) : worldPos;
        };

    bool ok = false;

    switch (action.type)
    {
    case UndoModel::ActionType::DrawReserveToHand:
    {
        // Move hand top back to reserve
        const auto& hand = _gameModel.getHandCards();
        if (hand.empty()) break;
        int cardId = hand.back()->getId();

        ok = _gameModel.moveTopHandCardBackToReserve(action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
        if (ok) {
            auto itv = _cardViews.find(cardId);
            if (itv != _cardViews.end()) {
                CardView* v = itv->second;
                if (v) {
                    Vec2 worldPos = toWorld(action.prevPosition);
                    Vec2 localPos = toLocal(_reserveNode, worldPos);
                    if (v->getParent() != _reserveNode) {
                        v->removeFromParent();
                        if (_reserveNode) _reserveNode->addChild(v);
                    }
                    v->setPosition(localPos);
                    v->setFaceUp(action.prevFaceUp, false);
                    v->setVisible(action.prevVisible);
                    v->setClickCallback([this](int /*cardId*/) {
                        this->handleReserveClick();
                        });
                }
            }
        }
        break;
    }
    case UndoModel::ActionType::MovePlayfieldToHand:
    {
        // Move hand top back to original playfield slot
        const auto& hand = _gameModel.getHandCards();
        if (hand.empty()) break;
        int cardId = hand.back()->getId();

        ok = _gameModel.moveTopHandCardToPlayfieldAt(action.playfieldIndex, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
        if (ok) {
            auto itv = _cardViews.find(cardId);
            if (itv != _cardViews.end()) {
                CardView* v = itv->second;
                if (v) {
                    Vec2 worldPos = toWorld(action.prevPosition);
                    Vec2 localPos = toLocal(_playfieldNode, worldPos);
                    if (v->getParent() != _playfieldNode) {
                        v->removeFromParent();
                        if (_playfieldNode) _playfieldNode->addChild(v);
                    }
                    v->setPosition(localPos);
                    v->setFaceUp(action.prevFaceUp, false);
                    v->setVisible(action.prevVisible);
                    v->setClickCallback([this](int id) {
                        this->handlePlayfieldCardClick(id);
                        });
                }
            }
            updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);
        }
        break;
    }
    case UndoModel::ActionType::MoveHandToPlayfield:
    {
        // Bring card back from playfield index to hand top, restoring state
        ok = _gameModel.movePlayfieldCardToHand(action.playfieldIndex);
        if (ok) {
            const auto& hand = _gameModel.getHandCards();
            if (!hand.empty()) {
                int cardId = hand.back()->getId();
                auto card = hand.back();
                card->setPosition(action.prevPosition);
                card->setStatus(action.prevStatus);
                card->setVisible(action.prevVisible);
                card->setFaceUp(action.prevFaceUp);

                auto itv = _cardViews.find(cardId);
                if (itv != _cardViews.end()) {
                    CardView* v = itv->second;
                    if (v) {
                        Vec2 worldPos = toWorld(action.prevPosition);
                        Vec2 localPos = toLocal(_handNode, worldPos);
                        if (v->getParent() != _handNode) {
                            v->removeFromParent();
                            if (_handNode) _handNode->addChild(v);
                        }
                        v->setPosition(localPos);
                        v->setFaceUp(action.prevFaceUp, false);
                        v->setVisible(action.prevVisible);
                        v->setClickCallback([](int /*cardId*/) {});
                    }
                }
            }
            updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);
        }
        break;
    }
    case UndoModel::ActionType::MoveHandToReserve:
    {
        // Move reserve top (the one just returned) back to hand, restoring state
        const auto& reserve = _gameModel.getReserveCards();
        if (reserve.empty()) break;
        int cardId = reserve.back()->getId();

        ok = _gameModel.drawReserveToHand();
        if (ok) {
            auto& handRef = _gameModel.getHandCards();
            if (!handRef.empty()) {
                auto card = handRef.back();
                card->setPosition(action.prevPosition);
                card->setStatus(action.prevStatus);
                card->setVisible(action.prevVisible);
                card->setFaceUp(action.prevFaceUp);

                auto itv = _cardViews.find(cardId);
                if (itv != _cardViews.end()) {
                    CardView* v = itv->second;
                    if (v) {
                        Vec2 worldPos = toWorld(action.prevPosition);
                        Vec2 localPos = toLocal(_handNode, worldPos);
                        if (v->getParent() != _handNode) {
                            v->removeFromParent();
                            if (_handNode) _handNode->addChild(v);
                        }
                        v->setPosition(localPos);
                        v->setFaceUp(action.prevFaceUp, false);
                        v->setVisible(action.prevVisible);
                        v->setClickCallback([](int /*cardId*/) {});
                    }
                }
            }
        }
        break;
    }
    case UndoModel::ActionType::FlipHandTopFaceUp:
    {
        auto& hand = _gameModel.getHandCards();
        if (hand.empty()) break;
        int cardId = hand.back()->getId();
        hand.back()->setFaceUp(action.prevFaceUp);

        auto itv = _cardViews.find(cardId);
        if (itv != _cardViews.end()) {
            CardView* v = itv->second;
            if (v) {
                v->setFaceUp(action.prevFaceUp, false);
            }
        }
        ok = true;
        break;
    }
    default:
        ok = false;
        break;
    }

    if (!ok) {
        CCLOG("GameController::handleUndo - apply failed for action type %d", static_cast<int>(action.type));
    }
}

void GameController::reset()
{
    // remove views
    for (auto& kv : _cardViews) {
        CardView* v = kv.second;
        if (v && v->getParent()) v->removeFromParent();
    }
    _cardViews.clear();

    // reset model & undo
    _gameModel = GameModel();
    _undoModel.clear();
    _busy = false;
}