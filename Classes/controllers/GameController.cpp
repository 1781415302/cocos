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

    // 创建三个子节点并添加到 parent
    _playfieldNode = Node::create();
    _reserveNode = Node::create();
    _handNode = Node::create();

    _playfieldNode->setName("playfield_node");
    _reserveNode->setName("reserve_node");
    _handNode->setName("hand_node");

    // ZOrder: playfield 下面, reserve 中间, hand 最上
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

    // 1) 加载关卡配置
    LevelConfig config = LevelConfigLoader::loadLevelConfig(levelId);

    // 2) 生成游戏 model：注意用 GameModelFromLevelGenerator
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

    // Reserve（备用堆），点击为 draw
    const auto& reserve = _gameModel.getReserveCards();
    for (size_t i = 0; i < reserve.size(); ++i) {
        auto cardPtr = reserve[i];
        if (!cardPtr) continue;
        CardView* v = CardView::create(cardPtr);
        if (!v) continue;

        _reserveNode->addChild(v);
        _cardViews[cardPtr->getId()] = v;

        // reserve 的点击仅作为 draw 操作（不传入 id）
        v->setClickCallback([this](int /*cardId*/) {
            this->handleReserveClick();
            });

        // 根据模型状态显示
        v->setCardVisible(cardPtr->isFaceUp());
    }

    // Hand（手牌区）
    const auto& hand = _gameModel.getHandCards();
    for (size_t i = 0; i < hand.size(); ++i) {
        auto cardPtr = hand[i];
        if (!cardPtr) continue;
        CardView* v = CardView::create(cardPtr);
        if (!v) continue;
        _handNode->addChild(v);
        _cardViews[cardPtr->getId()] = v;

        // hand 的点击目前 noop（将来可以扩展）
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

    // 确认 hand (栈顶) 可用于匹配
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

    // 通过动画把 playfield 卡移动到 hand
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

    // 目标位置：hand 顶部或默认位置
    Vec2 targetPos = _defaultHandPosition;
    const auto& hand = _gameModel.getHandCards();
    if (!hand.empty()) {
        targetPos = hand.back()->getPosition();
    }

    view->playMoveAnimation(targetPos, _moveDuration, [this, playfieldIndex, cardId, targetPos]() {
        // 更新 model：把 playfield 卡移入 hand（model 方法名为 movePlayfieldCardToHand）
        bool ok = _gameModel.movePlayfieldCardToHand(playfieldIndex);
        if (!ok) {
            CCLOG("GameController::animatePlayfieldCardToHand - model move failed for id %d", cardId);
            _busy = false;
            return;
        }

        // 更新被移动卡的 model 属性（位置/翻开）
        if (!_gameModel.getStackCards().empty()) {
            auto moved = _gameModel.getStackCards().back();
            moved->setPosition(targetPos);
            moved->setFaceUp(true);

            // 更新对应的 CardView：从 playfieldNode 移动到 handNode
            auto itv = _cardViews.find(moved->getId());
            if (itv != _cardViews.end()) {
                CardView* v = itv->second;
                if (v->getParent() == _playfieldNode) {
                    v->removeFromParent();
                    _handNode->addChild(v);
                }
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

    // 启动抽牌动画
    _busy = true;
    animateReserveTopToHand();
}

void GameController::animateReserveTopToHand()
{
    // 取 reserve 顶部（back）视图
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

    // 目标位置：hand 顶部或默认位置
    Vec2 targetPos = _defaultHandPosition;
    const auto& hand = _gameModel.getHandCards();
    if (!hand.empty()) {
        targetPos = hand.back()->getPosition();
    }

    view->playMoveAnimation(targetPos, _moveDuration, [this, cardId, targetPos]() {
        // 更新 model：从 reserve 抽取到 hand（使用 drawReserveToHand）
        bool ok = _gameModel.drawReserveToHand();
        if (!ok) {
            CCLOG("GameController::animateReserveTopToHand - model draw failed for id %d", cardId);
            _busy = false;
            return;
        }

        // 更新 model->view：刚被抽取的卡现在位于 hand 的末尾
        const auto& handRef = _gameModel.getHandCards();
        if (!handRef.empty()) {
            auto moved = handRef.back();
            moved->setPosition(targetPos);
            moved->setFaceUp(true);

            auto itv = _cardViews.find(moved->getId());
            if (itv != _cardViews.end()) {
                CardView* v = itv->second;
                if (v->getParent() == _reserveNode) {
                    v->removeFromParent();
                    _handNode->addChild(v);
                }
                v->setCardVisible(true);
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