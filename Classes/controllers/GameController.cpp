#pragma once
// Classes/controllers/GameController.cpp
#include "GameController.h"
#include "configs/Loaders/LevelConfigLoader.h"
#include "services/GameModelFromLevelGenerator.h"
#include "models/CardModel.h"
#include "utils/CardEnums.h"
#include <cassert>
#include <algorithm>

using namespace cocos2d;

GameController::GameController(Node* parentNode)
    : _parentNode(parentNode)
{
    assert(parentNode && "GameController requires a valid parent node");

    // 创建 playfield / reserve / hand node 并 attach 到 parent，ZOrder 控制层级
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

    // 1) 读取关卡配置
    LevelConfig config = LevelConfigLoader::loadLevelConfig(levelId);

    // 2) 生成 model
    _gameModel = GameModelFromLevelGenerator::generateGameModel(config);

    // 3) 从 model 生成 view
    createViewsFromModel();

    // 4) 自动把 reserve 顶一张牌的 hand（无动画版本）
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

        // 点击回调
        v->setClickCallback([this](int cardId) {
            this->handlePlayfieldCardClick(cardId);
            });

        // 根据 model 状态显示（无动画）
        v->setFaceUp(cardPtr->isFaceUp(), false);
    }

    // 创建完 playfield CardView 后，调用一次覆盖判定更新
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
 * 根据当前 playfield 的 CardView 计算实际包围盒，映射到 _playfieldNode，判断覆盖关系，同步 model/view。
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
                itv->second->setFaceUp(!covered, false);
            }
        }
    }
}

/**
 * 开局时自动把 reserve 顶一张牌的 hand。
 * 如果 animate=true，则调用动画版本 animateReserveTopToHand。
 * 默认 animate=false，不做动画；适合初始化阶段。
 */
void GameController::drawInitialReserveTopToHand(bool animate)
{
    if (animate) {
        // 动画版本
        if (!_busy) {
            _busy = true;
            animateReserveTopToHand();
            // animateReserveTopToHand 结束后会把 _busy 置 false
        }
        return;
    }

    // 非动画路径：直接在 model 中执行 draw，并把对应 view reparent 到 hand
    const auto& reserve = _gameModel.getReserveCards();
    if (reserve.empty()) return;

    bool ok = _gameModel.drawReserveToHand();
    if (!ok) return;

    const auto& handRef = _gameModel.getHandCards();
    if (handRef.empty()) return;
    auto moved = handRef.back();
    if (!moved) return;

    // 从 model 拿到位置与状态（design-space）
    Vec2 targetPos = _defaultHandPosition;
    if (!handRef.empty() && handRef.size() > 1) {
        // 使用 hand 原最后一张的位置，保持排布的一致性
        targetPos = handRef[handRef.size() - 2]->getPosition();
    }
    moved->setPosition(targetPos);
    moved->setFaceUp(true);

    // 将对应 CardView reparent 到 handNode 并更新位置/显示
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
            // 进入手牌后，点击应为手牌逻辑（此处为 no-op，防止触发 reserve 抽牌）
            v->setClickCallback([](int /*cardId*/) {});
        }
    }

    // 虽然 draw 不直接影响 playfield 覆盖，但调用一次确保状态一致
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

    // 确认 hand (栈顶) 是否可用于匹配
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

    // 开始动画：playfield -> hand
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

    // 目标位置：hand 的默认位置或末尾
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

    // world -> view 当前 parent 的局部坐标
    Node* currentParent = view->getParent();
    if (!currentParent) {
        CCLOG("GameController::animatePlayfieldCardToHand - view has no parent, aborting");
        _busy = false;
        return;
    }
    Vec2 localTargetForCurrentParent = currentParent->convertToNodeSpace(worldTarget);

    // 执行动画
    view->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, playfieldIndex, cardId, worldTarget, targetPos]() {
        // 动画完成后，先更新 model：playfield 移动到 hand
        bool ok = _gameModel.movePlayfieldCardToHand(playfieldIndex);
        if (!ok) {
            CCLOG("GameController::animatePlayfieldCardToHand - model move failed for id %d", cardId);
            _busy = false;
            return;
        }

        // 从 model 取到移动的牌，更新 position/status
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

        // view reparent 到 handNode 并更新显示
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
                // 进入手牌后，点击应为手牌逻辑（此处为 no-op，防止触发 reserve 抽牌）
                v->setClickCallback([](int /*cardId*/) {});
            }
        }

        // playfield 覆盖判定
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

    view->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, cardId, worldTarget, targetPos]() {
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
                    // 进入手牌后，点击应为手牌逻辑（此处为 no-op，防止触发 reserve 抽牌）
                    v->setClickCallback([](int /*cardId*/) {});
                }
            }
        }

        // reserve -> hand 不影响 playfield，但保持 busy 状态正确
        _busy = false;
        });
}

void GameController::handleUndo()
{
    CCLOG("GameController::handleUndo - not implemented");
}

void GameController::reset()
{
    // remove views
    for (auto& kv : _cardViews) {
        CardView* v = kv.second;
        if (v && v->getParent()) v->removeFromParent();
    }
    _cardViews.clear();

    // reset model
    _gameModel = GameModel();
    _busy = false;
}