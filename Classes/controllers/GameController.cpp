#pragma once
#include "GameController.h"
#include "configs/Loaders/LevelConfigLoader.h"
#include "services/GameModelFromLevelGenerator.h"
#include "models/CardModel.h"
#include "models/UndoModel.h"
#include "utils/CardEnums.h"
#include "views/UndoView.h"
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

    if (_undoView && _undoView->getParent()) {
        _undoView->removeFromParent();
    }
    _undoView = nullptr;
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

    // 5) Create undo view AFTER reset() and after views are ready, then position it next to hand
    if (!_undoView) {
        _undoView = UndoView::create("undo.png");
        if (_undoView) {
            _undoView->setName("undo_view");
            _undoView->setClickCallback([this]() {
                this->handleUndo();
                });
            // add to parent with high z to be above everything else
            _parentNode->addChild(_undoView, 20);
        }
    }

    // Ensure undo is positioned relative to the hand top
    repositionUndoToRightOfHand();
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
            // no-op for hand
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

        // Reparent view to hand (safe)
        auto itv = _cardViews.find(moved->getId());
        if (itv != _cardViews.end()) {
            CardView* v = itv->second;
            if (v) {
                Vec2 handLocal = _handNode ? _handNode->convertToNodeSpace(worldTarget) : worldTarget;
                if (v->getParent() != _handNode) {
                    reparentView(v, _handNode);
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
                        reparentView(v, _handNode);
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
        // 把手牌顶部移回 reserve（带动画），动画完成后 model 执行并翻转到 prevFaceUp
        const auto& hand = _gameModel.getHandCards();
        if (hand.empty()) break;
        int cardId = hand.back()->getId();

        auto itv = _cardViews.find(cardId);
        if (itv == _cardViews.end() || !itv->second) {
            // 回退到原有即时逻辑
            ok = _gameModel.moveTopHandCardBackToReserve(action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
            if (ok) {
                auto itv2 = _cardViews.find(cardId);
                if (itv2 != _cardViews.end()) {
                    CardView* v = itv2->second;
                    if (v) {
                        Vec2 worldPos = toWorld(action.prevPosition);
                        Vec2 localPos = toLocal(_reserveNode, worldPos);
                        if (v->getParent() != _reserveNode) {
                            reparentView(v, _reserveNode);
                        }
                        v->setPosition(localPos);
                        v->setFaceUp(action.prevFaceUp, false);
                        v->setVisible(action.prevVisible);
                    }
                }
            }
            break;
        }

        CardView* v = itv->second;
        Node* currentParent = v->getParent();
        Vec2 worldTarget = toWorld(action.prevPosition);

        if (!currentParent) {
            // fallback immediate
            ok = _gameModel.moveTopHandCardBackToReserve(action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
            if (ok) {
                auto itv2 = _cardViews.find(cardId);
                if (itv2 != _cardViews.end()) {
                    CardView* vv = itv2->second;
                    if (vv) {
                        Vec2 localPos = toLocal(_reserveNode, worldTarget);
                        if (vv->getParent() != _reserveNode) {
                            reparentView(vv, _reserveNode);
                        }
                        vv->setPosition(localPos);
                        vv->setFaceUp(action.prevFaceUp, false);
                        vv->setVisible(action.prevVisible);
                    }
                }
            }
            break;
        }

        // 启动动画：在当前父节点坐标系内移动到目标的世界坐标对应位置
        Vec2 localTargetForCurrentParent = currentParent->convertToNodeSpace(worldTarget);
        _busy = true;
        v->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, cardId, action, worldTarget]() mutable {
            bool okInner = _gameModel.moveTopHandCardBackToReserve(action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
            if (!okInner) {
                CCLOG("GameController::handleUndo(DrawReserveToHand) - model move failed for id %d", action.cardId);
                _busy = false;
                return;
            }

            auto itv2 = _cardViews.find(cardId);
            if (itv2 != _cardViews.end()) {
                CardView* vv = itv2->second;
                if (vv) {
                    // 重父化到 reserve 并设置位置/状态
                    reparentView(vv, _reserveNode);
                    Vec2 localPos = _reserveNode ? _reserveNode->convertToNodeSpace(worldTarget) : worldTarget;
                    vv->setPosition(localPos);
                    vv->setVisible(action.prevVisible);
                    // 移动到 reserve 后播放翻转动画（恢复 prevFaceUp）
                    vv->setFaceUp(action.prevFaceUp, true);
                    // reparentView 已设置了 reserve 的点击回调
                }
            }
            _busy = false;
            });
        ok = true;
        break;
    }
    case UndoModel::ActionType::MovePlayfieldToHand:
    {
        // 把手牌顶部移回原来的 playfield slot（带动画），动画后 model 执行并重父化，恢复状态
        const auto& hand = _gameModel.getHandCards();
        if (hand.empty()) break;
        int cardId = hand.back()->getId();

        auto itv = _cardViews.find(cardId);
        if (itv == _cardViews.end() || !itv->second) {
            ok = _gameModel.moveTopHandCardToPlayfieldAt(action.playfieldIndex, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
            if (ok) {
                auto itv2 = _cardViews.find(cardId);
                if (itv2 != _cardViews.end()) {
                    CardView* v = itv2->second;
                    if (v) {
                        Vec2 worldPos = toWorld(action.prevPosition);
                        Vec2 localPos = toLocal(_playfieldNode, worldPos);
                        if (v->getParent() != _playfieldNode) {
                            reparentView(v, _playfieldNode);
                        }
                        v->setPosition(localPos);
                        v->setFaceUp(action.prevFaceUp, false);
                        v->setVisible(action.prevVisible);
                        v->setClickCallback([this](int id) { this->handlePlayfieldCardClick(id); });
                    }
                }
                updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);
            }
            break;
        }

        CardView* v = itv->second;
        Node* currentParent = v->getParent();
        Vec2 worldTarget = toWorld(action.prevPosition);

        if (!currentParent) {
            ok = _gameModel.moveTopHandCardToPlayfieldAt(action.playfieldIndex, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
            if (ok) {
                auto itv2 = _cardViews.find(cardId);
                if (itv2 != _cardViews.end()) {
                    CardView* vv = itv2->second;
                    if (vv) {
                        Vec2 localPos = toLocal(_playfieldNode, worldTarget);
                        if (vv->getParent() != _playfieldNode) {
                            reparentView(vv, _playfieldNode);
                        }
                        vv->setPosition(localPos);
                        vv->setFaceUp(action.prevFaceUp, false);
                        vv->setVisible(action.prevVisible);
                    }
                }
                updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);
            }
            break;
        }

        Vec2 localTargetForCurrentParent = currentParent->convertToNodeSpace(worldTarget);
        _busy = true;
        v->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, cardId, action, worldTarget]() mutable {
            bool okInner = _gameModel.moveTopHandCardToPlayfieldAt(action.playfieldIndex, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
            if (!okInner) {
                CCLOG("GameController::handleUndo(MovePlayfieldToHand) - model move failed for id %d", action.cardId);
                _busy = false;
                return;
            }

            auto itv2 = _cardViews.find(cardId);
            if (itv2 != _cardViews.end()) {
                CardView* vv = itv2->second;
                if (vv) {
                    reparentView(vv, _playfieldNode);
                    Vec2 localPos = _playfieldNode ? _playfieldNode->convertToNodeSpace(worldTarget) : worldTarget;
                    vv->setPosition(localPos);
                    vv->setFaceUp(action.prevFaceUp, false);
                    vv->setVisible(action.prevVisible);
                    vv->setClickCallback([this](int id) { this->handlePlayfieldCardClick(id); });
                }
            }
            updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);
            _busy = false;
            });
        ok = true;
        break;
    }
    case UndoModel::ActionType::MoveHandToPlayfield:
    {
        // 原操作是 hand -> playfield，undo 要把 playfield 中对应卡片移回 hand（带动画）
        // 在 undo 前 card 应该在 playfield 的 action.playfieldIndex 处或者可以按 action.cardId 找到 view
        int cardId = action.cardId;
        auto itv = _cardViews.find(cardId);
        if (itv == _cardViews.end() || !itv->second) {
            // fallback to immediate model change
            ok = _gameModel.movePlayfieldCardToHand(action.playfieldIndex);
            if (ok) {
                const auto& hand = _gameModel.getHandCards();
                if (!hand.empty()) {
                    int newId = hand.back()->getId();
                    auto card = hand.back();
                    card->setPosition(action.prevPosition);
                    card->setStatus(action.prevStatus);
                    card->setVisible(action.prevVisible);
                    card->setFaceUp(action.prevFaceUp);

                    auto itv2 = _cardViews.find(newId);
                    if (itv2 != _cardViews.end()) {
                        CardView* v = itv2->second;
                        if (v) {
                            Vec2 worldPos = toWorld(action.prevPosition);
                            Vec2 localPos = toLocal(_handNode, worldPos);
                            if (v->getParent() != _handNode) {
                                reparentView(v, _handNode);
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

        // 有视图，当前父节点应是 playfield
        CardView* v = itv->second;
        Node* currentParent = v->getParent();
        if (!currentParent) {
            ok = _gameModel.movePlayfieldCardToHand(action.playfieldIndex);
            if (ok) {
                const auto& hand = _gameModel.getHandCards();
                if (!hand.empty()) {
                    int newId = hand.back()->getId();
                    auto card = hand.back();
                    card->setPosition(action.prevPosition);
                    card->setStatus(action.prevStatus);
                    card->setVisible(action.prevVisible);
                    card->setFaceUp(action.prevFaceUp);

                    auto itv2 = _cardViews.find(newId);
                    if (itv2 != _cardViews.end()) {
                        CardView* vv = itv2->second;
                        if (vv) {
                            Vec2 worldPos = toWorld(action.prevPosition);
                            Vec2 localPos = toLocal(_handNode, worldPos);
                            if (vv->getParent() != _handNode) {
                                reparentView(vv, _handNode);
                            }
                            vv->setPosition(localPos);
                            vv->setFaceUp(action.prevFaceUp, false);
                            vv->setVisible(action.prevVisible);
                        }
                    }
                }
                updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);
            }
            break;
        }

        // 动画：从 playfield（当前Parent）移动到 hand 目标位置
        Vec2 worldTarget = toWorld(action.prevPosition);
        Vec2 localTargetForCurrentParent = currentParent->convertToNodeSpace(worldTarget);
        _busy = true;
        v->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, cardId, action, worldTarget]() mutable {
            bool okInner = _gameModel.movePlayfieldCardToHand(action.playfieldIndex);
            if (!okInner) {
                CCLOG("GameController::handleUndo(MoveHandToPlayfield) - model move failed for id %d", action.cardId);
                _busy = false;
                return;
            }

            const auto& hand = _gameModel.getHandCards();
            if (!hand.empty()) {
                int newId = hand.back()->getId();
                auto card = hand.back();
                card->setPosition(action.prevPosition);
                card->setStatus(action.prevStatus);
                card->setVisible(action.prevVisible);
                card->setFaceUp(action.prevFaceUp);

                auto itv2 = _cardViews.find(newId);
                if (itv2 != _cardViews.end()) {
                    CardView* vv = itv2->second;
                    if (vv) {
                        reparentView(vv, _handNode);
                        Vec2 localPos = _handNode ? _handNode->convertToNodeSpace(worldTarget) : worldTarget;
                        vv->setPosition(localPos);
                        vv->setFaceUp(action.prevFaceUp, false);
                        vv->setVisible(action.prevVisible);
                        vv->setClickCallback([](int /*cardId*/) {});
                    }
                }
            }
            updatePlayfieldCoverage(/*overlapAreaThreshold=*/0.0f);
            _busy = false;
            });
        ok = true;
        break;
    }
    case UndoModel::ActionType::MoveHandToReserve:
    {
        // undo: reserve -> hand（也就是把 reserve 顶部返回到手牌），需要从 reserve 做动画到 hand
        const auto& reserve = _gameModel.getReserveCards();
        if (reserve.empty()) break;
        int cardId = reserve.back()->getId();

        auto itv = _cardViews.find(cardId);
        if (itv == _cardViews.end() || !itv->second) {
            // fallback immediate draw
            ok = _gameModel.drawReserveToHand();
            if (ok) {
                auto& handRef = _gameModel.getHandCards();
                if (!handRef.empty()) {
                    auto card = handRef.back();
                    card->setPosition(action.prevPosition);
                    card->setStatus(action.prevStatus);
                    card->setVisible(action.prevVisible);
                    card->setFaceUp(action.prevFaceUp);

                    auto itv2 = _cardViews.find(cardId);
                    if (itv2 != _cardViews.end()) {
                        CardView* v = itv2->second;
                        if (v) {
                            Vec2 worldPos = toWorld(action.prevPosition);
                            Vec2 localPos = toLocal(_handNode, worldPos);
                            if (v->getParent() != _handNode) {
                                reparentView(v, _handNode);
                            }
                            v->setPosition(localPos);
                            v->setFaceUp(action.prevFaceUp, false);
                            v->setVisible(action.prevVisible);
                        }
                    }
                }
            }
            break;
        }

        CardView* v = itv->second;
        Node* currentParent = v->getParent();
        if (!currentParent) {
            ok = _gameModel.drawReserveToHand();
            if (ok) {
                auto& handRef = _gameModel.getHandCards();
                if (!handRef.empty()) {
                    auto card = handRef.back();
                    card->setPosition(action.prevPosition);
                    card->setStatus(action.prevStatus);
                    card->setVisible(action.prevVisible);
                    card->setFaceUp(action.prevFaceUp);

                    auto itv2 = _cardViews.find(cardId);
                    if (itv2 != _cardViews.end()) {
                        CardView* vv = itv2->second;
                        if (vv) {
                            Vec2 worldPos = toWorld(action.prevPosition);
                            Vec2 localPos = toLocal(_handNode, worldPos);
                            if (vv->getParent() != _handNode) {
                                reparentView(vv, _handNode);
                            }
                            vv->setPosition(localPos);
                            vv->setFaceUp(action.prevFaceUp, false);
                            vv->setVisible(action.prevVisible);
                        }
                    }
                }
            }
            break;
        }

        // 动画：从 reserve 当前父节点移动到 hand 目标位置
        Vec2 worldTarget = toWorld(action.prevPosition);
        Vec2 localTargetForCurrentParent = currentParent->convertToNodeSpace(worldTarget);
        _busy = true;
        v->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, cardId, action, worldTarget]() mutable {
            bool okInner = _gameModel.drawReserveToHand();
            if (!okInner) {
                CCLOG("GameController::handleUndo(MoveHandToReserve) - model draw failed for id %d", action.cardId);
                _busy = false;
                return;
            }

            auto& handRef = _gameModel.getHandCards();
            if (!handRef.empty()) {
                auto card = handRef.back();
                card->setPosition(action.prevPosition);
                card->setStatus(action.prevStatus);
                card->setVisible(action.prevVisible);
                card->setFaceUp(action.prevFaceUp);

                auto itv2 = _cardViews.find(cardId);
                if (itv2 != _cardViews.end()) {
                    CardView* vv = itv2->second;
                    if (vv) {
                        reparentView(vv, _handNode);
                        Vec2 localPos = _handNode ? _handNode->convertToNodeSpace(worldTarget) : worldTarget;
                        vv->setPosition(localPos);
                        vv->setFaceUp(action.prevFaceUp, false);
                        vv->setVisible(action.prevVisible);
                        vv->setClickCallback([](int /*cardId*/) {});
                    }
                }
            }
            _busy = false;
            });
        ok = true;
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

    // remove undo view
    if (_undoView && _undoView->getParent()) {
        _undoView->removeFromParent();
    }
    _undoView = nullptr;

    // reset model & undo
    _gameModel = GameModel();
    _undoModel.clear();
    _busy = false;
}

/**
 * Reposition undo button to the right of the hand top (design-space _defaultHandPosition).
 * spacing is the gap in pixels between the hand and the undo button.
 */
void GameController::repositionUndoToRightOfHand(float spacing)
{
    if (!_undoView || !_parentNode) return;

    // Convert the hand design position to world coordinates
    Vec2 worldHand = _parentNode->convertToWorldSpace(_defaultHandPosition);

    // Place undo to the right: offset by half button width + spacing
    Size btnSz = _undoView->getButtonSize();
    Vec2 worldPos = worldHand + Vec2(btnSz.width * 5.0f + spacing, 0.0f);

    // Convert back to parent-local coords and set position
    Vec2 parentLocal = _parentNode->convertToNodeSpace(worldPos);
    _undoView->setPosition(parentLocal);
}

// ----------------- 新增：安全 reparent 实现 -----------------
void GameController::reparentView(CardView* v, Node* newParent)
{
    if (!v) return;
    if (v->getParent() == newParent) {
        // Still ensure callback is correct for this parent
    }

    // 保证在 removeFromParent 期间对象不会被销毁
    v->retain();
    v->removeFromParent();
    if (newParent) {
        newParent->addChild(v);
    }
    v->release();

    // 根据目标父节点设置合适的点击回调
    if (newParent == _handNode) {
        // Hand: no-op
        v->setClickCallback([](int /*cardId*/) {});
    }
    else if (newParent == _reserveNode) {
        // Reserve: clicking reserve triggers reserve draw
        v->setClickCallback([this](int /*cardId*/) {
            this->handleReserveClick();
            });
    }
    else if (newParent == _playfieldNode) {
        // Playfield: clicking a playfield card passes its id
        v->setClickCallback([this](int id) {
            this->handlePlayfieldCardClick(id);
            });
    }
    else {
        // Unknown parent: clear to be safe
        v->setClickCallback([](int /*cardId*/) {});
    }
}