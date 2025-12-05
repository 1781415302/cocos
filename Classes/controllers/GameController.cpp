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

    // 创建区域子节点并添加到 parent
    _playfieldNode = Node::create();
    _reserveNode = Node::create();
    _handNode = Node::create();

    _playfieldNode->setName("playfield_node");
    _reserveNode->setName("reserve_node");
    _handNode->setName("hand_node");

    // ZOrder：playfield 最下，reserve 中间，hand 最高（确保 hand 覆盖 playfield）
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

    // 1) 加载关卡文档
    LevelConfig config = LevelConfigLoader::loadLevelConfig(levelId);

    // 2) 由服务生成 model（注意：GameModelFromLevelGenerator 需将 playfield/reserve/hand 生成到 model）
    _gameModel = GameModelFromLevelGenerator::generateGameModel(config);

    // 3) 从 model 创建视图（CardView）
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

        // 绑定点击
        v->setClickCallback([this](int cardId) {
            this->handlePlayfieldCardClick(cardId);
            });

        // front/back 显示
        v->setCardVisible(cardPtr->isFaceUp());
    }

    // Reserve（备用牌堆）：通常显示为背面图，点击 reserve 区会触发 draw
    const auto& reserve = _gameModel.getReserveCards();
    for (size_t i = 0; i < reserve.size(); ++i) {
        auto cardPtr = reserve[i];
        if (!cardPtr) continue;
        CardView* v = CardView::create(cardPtr);
        if (!v) continue;

        _reserveNode->addChild(v);
        _cardViews[cardPtr->getId()] = v;

        // 绑定点击 reserve 区（也可以只对顶部卡绑定，但这里简单绑定全部 reserve card 点击同样调用 draw）
        v->setClickCallback([this](int /*cardId*/) {
            this->handleReserveClick();
            });

        // 备用牌通常以背面显示（除非约定顶牌为正面）
        v->setCardVisible(cardPtr->isFaceUp());
    }

    // Hand（底牌堆）
    const auto& hand = _gameModel.getHandCards();
    for (size_t i = 0; i < hand.size(); ++i) {
        auto cardPtr = hand[i];
        if (!cardPtr) continue;
        CardView* v = CardView::create(cardPtr);
        if (!v) continue;
        _handNode->addChild(v);
        _cardViews[cardPtr->getId()] = v;

        // hand 的卡牌通常不响应 playfield 匹配点击（除非规则允许）
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

    // 确保 hand (底牌堆) 有可匹配的顶牌
    if (!_gameModel.canMatchWithTopStackCard()) {
        CCLOG("GameController::handlePlayfieldCardClick - no matching hand top");
        return;
    }

    auto handTop = _gameModel.getHandCards().back();
    if (!handTop) return;

    if (!facesAreAdjacent(cardPtr->getCardFace(), handTop->getCardFace())) {
        CCLOG("GameController::handlePlayfieldCardClick - faces not adjacent cardId=%d", cardId);
        return;
    }

    // 通过动画将 playfield 卡移动到 hand
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

    // 目标位置：hand 顶牌位置（若 hand 非空）或默认 hand 位置
    Vec2 targetPos = _defaultHandPosition;
    const auto& hand = _gameModel.getHandCards();
    if (!hand.empty()) {
        targetPos = hand.back()->getPosition();
    }

    view->playMoveAnimation(targetPos, _moveDuration, [this, playfieldIndex, cardId, targetPos]() {
        // 动画完成 -> 更新 model (将 playfield 卡移到 hand)
        bool ok = _gameModel.movePlayfieldCardToStack(playfieldIndex); // 你可改名为 movePlayfieldCardToHand
        if (!ok) {
            CCLOG("GameController::animatePlayfieldCardToHand - model move failed for id %d", cardId);
            _busy = false;
            return;
        }

        // 更新刚移动到 hand 的 card model（位置/faceUp）
        if (!_gameModel.getStackCards().empty()) {
            auto moved = _gameModel.getStackCards().back();
            moved->setPosition(targetPos);
            moved->setFaceUp(true);

            // 更新视图：将 CardView 从 playfieldNode 移到 handNode（若需要）
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

    // 先检查 reserve 是否有卡
    const auto& reserve = _gameModel.getReserveCards();
    if (reserve.empty()) {
        CCLOG("GameController::handleReserveClick - reserve empty");
        return;
    }

    // 简单逻辑：取 reserve 顶牌并动画移动到 hand 顶位置，动画完成后在 Model 中执行 draw
    _busy = true;
    animateReserveTopToHand();
}

void GameController::animateReserveTopToHand()
{
    // 取 reserve 顶牌视图（假定 reserve.back() 是顶）
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

    // 目标位置：hand 顶牌位置或默认
    Vec2 targetPos = _defaultHandPosition;
    const auto& hand = _gameModel.getHandCards();
    if (!hand.empty()) {
        targetPos = hand.back()->getPosition();
    }

    view->playMoveAnimation(targetPos, _moveDuration, [this, cardId, targetPos]() {
        // 动画完成后：从 reserve 移到 hand（model）
        bool ok = _gameModel.movePlayfieldCardToStack(0); // 占位：请替换为 _gameModel.drawReserveToHand() 或实现相应方法
        // 注意：上面调用只是占位，实际应使用 GameModel::drawReserveToHand()
        // 例如： bool ok = _gameModel.drawReserveToHand();
        if (!ok) {
            CCLOG("GameController::animateReserveTopToHand - model draw failed for id %d", cardId);
            _busy = false;
            return;
        }

        // 更新刚加入 hand 的 card 的 model 状态 & 对应 view
        // 这里假设 model 将新卡放在 getHandCards().back()
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

    // 清空 model
    _gameModel = GameModel();
    _busy = false;
}