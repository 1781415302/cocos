#pragma once
// Classes/controllers/GameController.cpp
#include "GameController.h"
#include "configs/Loaders/LevelConfigLoader.h"
#include "services/GameModelFromLevelGenerator.h"
#include "models/CardModel.h"
#include "utils/CardEnums.h"
#include <cassert>

using namespace cocos2d;

GameController::GameController(Node* parentNode)
    : _parentNode(parentNode)
{
    assert(parentNode && "GameController requires a valid parent node");

    // 创建三个子节点并 attach 到 parent
    _playfieldNode = Node::create();
    _reserveNode = Node::create();
    _handNode = Node::create();

    _playfieldNode->setName("playfield_node");
    _reserveNode->setName("reserve_node");
    _handNode->setName("hand_node");

    // ZOrder: playfield 最下, reserve 中间, hand 最上
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

    // 2) 生成游戏 model（playfield/reserve/hand）
    _gameModel = GameModelFromLevelGenerator::generateGameModel(config);

    // 3) 根据 model 创建视图 CardView
    createViewsFromModel();
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

        // front/back 显示
        v->setCardVisible(cardPtr->isFaceUp());
    }

    // Reserve（备用牌），显示为背面或正面由 model 中的 isFaceUp 决定
    const auto& reserve = _gameModel.getReserveCards();
    for (size_t i = 0; i < reserve.size(); ++i) {
        auto cardPtr = reserve[i];
        if (!cardPtr) continue;
        CardView* v = CardView::create(cardPtr);
        if (!v) continue;

        _reserveNode->addChild(v);
        _cardViews[cardPtr->getId()] = v;

        // reserve 的点击行为为 draw（不传 card id）
        v->setClickCallback([this](int /*cardId*/) {
            this->handleReserveClick();
            });

        // 使用 model 的 isFaceUp 来决定显示（现在 reserve 初始为背面，除非被翻到 hand）
        v->setCardVisible(cardPtr->isFaceUp());
    }

    // Hand 区
    const auto& hand = _gameModel.getHandCards();
    for (size_t i = 0; i < hand.size(); ++i) {
        auto cardPtr = hand[i];
        if (!cardPtr) continue;
        CardView* v = CardView::create(cardPtr);
        if (!v) continue;
        _handNode->addChild(v);
        _cardViews[cardPtr->getId()] = v;

        // hand 的点击当前为 no-op
        v->setClickCallback([this](int /*cardId*/) {
            // no-op
            });

        v->setCardVisible(cardPtr->isFaceUp());
    }
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

    // 确认 hand (堆顶) 有牌并是可匹配状态
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

    // 通过动画把 playfield 卡移到 hand
    _busy = true;
    animatePlayfieldCardToHand(playIndex, cardId);
}

/*
  说明动画相关处理（保持原注释不变）
*/

void GameController::animatePlayfieldCardToHand(int playfieldIndex, int cardId)
{
    auto it = _cardViews.find(cardId);
    if (it == _cardViews.end()) {
        CCLOG("GameController::animatePlayfieldCardToHand - CardView not found for id %d", cardId);
        _busy = false;
        return;
    }
    CardView* view = it->second;

    // 目标位置，hand 的默认位置或手牌最后一张的位置
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

    // world -> view 当前 parent 的 local
    Node* currentParent = view->getParent();
    if (!currentParent) {
        CCLOG("GameController::animatePlayfieldCardToHand - view has no parent, aborting");
        _busy = false;
        return;
    }
    Vec2 localTargetForCurrentParent = currentParent->convertToNodeSpace(worldTarget);

    // 播放移动动画（在当前 parent 下）
    view->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, playfieldIndex, cardId, worldTarget, targetPos]() {
        // 第二阶段回调：更新 model：playfield -> hand
        bool ok = _gameModel.movePlayfieldCardToHand(playfieldIndex);
        if (!ok) {
            CCLOG("GameController::animatePlayfieldCardToHand - model move failed for id %d", cardId);
            _busy = false;
            return;
        }

        // 获取被移动的 card（model）
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
        // 将 model 的位置/状态设置为 design-space 目标
        moved->setPosition(targetPos);
        moved->setFaceUp(true);

        // 更新对应的 CardView：reparent 到 handNode 并设置位置/可见性
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
            }
        }

        _busy = false;
        });
}

void GameController::handleReserveClick()
{
    if (_busy) {
        CCLOG("GameController::handleReserveClick - busy, ignore");
        return;
    }

    // 检查 reserve 是否为空
    const auto& reserve = _gameModel.getReserveCards();
    if (reserve.empty()) {
        CCLOG("GameController::handleReserveClick - reserve empty");
        return;
    }

    // 执行抽牌动作（动画/模型会在 animateReserveTopToHand 里完成）
    _busy = true;
    animateReserveTopToHand();
}

void GameController::animateReserveTopToHand()
{
    // 取 reserve 的 back（顶部）牌
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

    // 目标位置，hand 的默认位置或手牌最后一张的位置
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

    // world -> card 当前 parent 的 local
    Node* currentParent = view->getParent();
    if (!currentParent) {
        CCLOG("GameController::animateReserveTopToHand - view has no parent, aborting");
        _busy = false;
        return;
    }
    Vec2 localTargetForCurrentParent = currentParent->convertToNodeSpace(worldTarget);

    // 播放移动动画
    view->playMoveAnimation(localTargetForCurrentParent, _moveDuration, [this, cardId, worldTarget, targetPos]() {
        // 模型层执行 draw（reserve -> hand）
        bool ok = _gameModel.drawReserveToHand();
        if (!ok) {
            CCLOG("GameController::animateReserveTopToHand - model draw failed for id %d", cardId);
            _busy = false;
            return;
        }

        // 从 model 中获取被移动的 card 并更新 view（reparent 到 handNode）
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
                }
            }
        }

        _busy = false;
        });
}

void GameController::handleUndo()
{
    CCLOG("GameController::handleUndo - not implemented");
}

void GameController::reset()
{
    // 移除所有 views
    for (auto& kv : _cardViews) {
        CardView* v = kv.second;
        if (v && v->getParent()) v->removeFromParent();
    }
    _cardViews.clear();

    // 重置 model
    _gameModel = GameModel();
    _busy = false;
}