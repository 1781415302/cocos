// Classes/views/CardView.cpp
#include "CardView.h"
#include "models/CardModel.h"
#include "utils/CardEnums.h"
#include "cocos2d.h"
#include <string>
#include <sstream>
#include <iomanip>

using namespace cocos2d;

CardView* CardView::create(const std::shared_ptr<CardModel>& cardModel)
{
    CardView* pRet = new (std::nothrow) CardView();
    if (pRet && pRet->init(cardModel))
    {
        pRet->autorelease();
        return pRet;
    }
    delete pRet;
    return nullptr;
}

bool CardView::init(const std::shared_ptr<CardModel>& cardModel)
{
    if (!Node::init()) return false;
    if (!cardModel) {
        CCLOG("CardView::init - null cardModel");
        return false;
    }

    _cardModel = cardModel;
    auto modelLock = _cardModel.lock();
    if (!modelLock) return false;

    CardFaceType face = modelLock->getCardFace();
    CardSuitType suit = modelLock->getCardSuit();

    // 创建正面复合节点（或占位）
    _frontNode = createFrontNode(face, suit);
    if (!_frontNode) {
        // fallback: 空节点
        _frontNode = Node::create();
    }

    // 背面精灵（使用 Resources 根目录下的 card_back.png）
    _backSprite = Sprite::create(cardBackFilename());
    if (!_backSprite) {
        CCLOG("CardView::init - failed to load '%s', using placeholder", cardBackFilename().c_str());
        _backSprite = Sprite::create();
    }

    // 内容尺寸设为正面 background 大小（如果存在），否则使用背面
    Size contentSz = Size::ZERO;
    // 若 front 包含背景 sprite，尝试读取其 size
    if (_frontNode->getChildrenCount() > 0) {
        // 假设第一个子节点是背景 sprite
        Node* first = _frontNode->getChildren().front();
        if (auto sp = dynamic_cast<Sprite*>(first)) {
            contentSz = sp->getContentSize();
        }
    }
    if (contentSz.equals(Size::ZERO)) {
        contentSz = _backSprite->getContentSize();
    }
    if (contentSz.equals(Size::ZERO)) {
        contentSz = Size(150.0f, 200.0f);
    }
    setContentSize(contentSz);

    // 把正面/背面节点添加到本节点，背面放在下（z-order）
    _backSprite->setPosition(getContentSize() * 0.5f);
    _frontNode->setPosition(getContentSize() * 0.5f);
    addChild(_backSprite);
    addChild(_frontNode);

    // 根据模型状态设置初始正/背面
    _isFaceUp = (modelLock->getStatus() != CardStatus::COVERED);
    setFaceUp(_isFaceUp, false);

    // 设置本节点初始位置为 model 的 position
    setPosition(modelLock->getPosition());

    // 触摸监听
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = CC_CALLBACK_2(CardView::onTouchBegan, this);
    listener->onTouchEnded = CC_CALLBACK_2(CardView::onTouchEnded, this);
    listener->onTouchCancelled = CC_CALLBACK_2(CardView::onTouchCancelled, this);
    getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);

    return true;
}

std::string CardView::cardBackFilename() const
{
    return "card_back.png";
}
std::string CardView::cardGeneralFilename() const
{
    return "card_general.png";
}

std::string CardView::suitFilename(CardSuitType suit) const
{
    switch (suit) {
    case CardSuitType::CST_CLUBS:   return "suits/club.png";
    case CardSuitType::CST_DIAMONDS:return "suits/diamond.png";
    case CardSuitType::CST_HEARTS:  return "suits/heart.png";
    case CardSuitType::CST_SPADES:  return "suits/spade.png";
    default: return "";
    }
}

static std::string faceToStr(CardFaceType face) {
    int v = static_cast<int>(face);
    if (v == 1) return "A";
    if (v == 11) return "J";
    if (v == 12) return "Q";
    if (v == 13) return "K";
    return std::to_string(v);
}

std::string CardView::bigNumberFilename(CardFaceType face, CardSuitType suit) const
{
    // big_red_A.png / big_black_2.png etc.
    bool isRed = (suit == CardSuitType::CST_DIAMONDS || suit == CardSuitType::CST_HEARTS);
    std::string color = isRed ? "big_red_" : "big_black_";
    std::string key;
    int v = static_cast<int>(face);
    if (v == 1) key = "A";
    else if (v == 11) key = "J";
    else if (v == 12) key = "Q";
    else if (v == 13) key = "K";
    else key = std::to_string(v);
    return "number/" + color + key + ".png";
}

std::string CardView::smallNumberFilename(CardFaceType face, CardSuitType suit) const
{
    bool isRed = (suit == CardSuitType::CST_DIAMONDS || suit == CardSuitType::CST_HEARTS);
    std::string color = isRed ? "small_red_" : "small_black_";
    std::string key;
    int v = static_cast<int>(face);
    if (v == 1) key = "A";
    else if (v == 11) key = "J";
    else if (v == 12) key = "Q";
    else if (v == 13) key = "K";
    else key = std::to_string(v);
    return "number/" + color + key + ".png";
}

Node* CardView::createFrontNode(CardFaceType faceType, CardSuitType suitType)
{
    // 背景
    std::string bgFile = cardGeneralFilename();
    Sprite* bg = Sprite::create(bgFile);
    if (!bg) {
        CCLOG("CardView::createFrontNode - missing background '%s'", bgFile.c_str());
        bg = Sprite::create();
    }

    Node* front = Node::create();
    front->addChild(bg);

    Size bgSize = bg->getContentSize();
    if (bgSize.width <= 0 || bgSize.height <= 0) {
        // 如果 background 没有尺寸，使用一个合理默认并让子元素基于该默认布局
        bgSize = Size(150.0f, 200.0f);
        bg->setContentSize(bgSize);
    }

    // 中央大数字
    std::string bigFile = bigNumberFilename(faceType, suitType);
    Sprite* bigNum = nullptr;
    if (!bigFile.empty()) {
        if (FileUtils::getInstance()->isFileExist(bigFile)) {
            bigNum = Sprite::create(bigFile);
        }
        else {
            CCLOG("CardView::createFrontNode - big number missing: %s", bigFile.c_str());
        }
    }
    if (!bigNum) {
        // 作为回退，用文字 label（不常见）
        bigNum = Sprite::create();
    }
    bigNum->setPosition(0,0);
    front->addChild(bigNum);

    // 左上小数字
    std::string smallFile = smallNumberFilename(faceType, suitType);
    Sprite* smallNum = nullptr;
    if (!smallFile.empty()) {
        if (FileUtils::getInstance()->isFileExist(smallFile)) {
            smallNum = Sprite::create(smallFile);
        }
        else {
            CCLOG("CardView::createFrontNode - small number missing: %s", smallFile.c_str());
        }
    }
    if (!smallNum) smallNum = Sprite::create();
    smallNum->setAnchorPoint(Vec2(0, 1)); // 左上为锚点
    smallNum->setPosition(-bgSize.width*0.45f, bgSize.height*0.45f);
    front->addChild(smallNum);

    // 右上花色图标
    std::string suitFile = suitFilename(suitType);
    Sprite* suitSp = nullptr;
    if (!suitFile.empty()) {
        if (FileUtils::getInstance()->isFileExist(suitFile)) {
            suitSp = Sprite::create(suitFile);
        }
        else {
            CCLOG("CardView::createFrontNode - suit image missing: %s", suitFile.c_str());
        }
    }
    if (!suitSp) suitSp = Sprite::create();
    suitSp->setAnchorPoint(Vec2(1, 1)); // 右上锚点
    suitSp->setPosition(bgSize.width * 0.45f, bgSize.height * 0.45f);
    front->addChild(suitSp);

    // 把 bg 置为 front 的第一个子节点（已添加），并将 front 的 contentSize 与 bg 一致
    front->setContentSize(bgSize);

    return front;
}

void CardView::setFaceUp(bool faceUp, bool animate)
{
    // 如果状态相同，直接返回
    if (_isFaceUp == faceUp) return;
    _isFaceUp = faceUp;

    // 简单翻转动画：scaleX 1 -> 0, 切换, 0 -> 1
    if (!animate) {
        // 直接显示或隐藏
        _frontNode->setVisible(faceUp);
        _backSprite->setVisible(!faceUp);
        return;
    }

    float half = 0.12f;
    auto shrink = ScaleTo::create(half, 0.0f, 1.0f);
    auto expand = ScaleTo::create(half, 1.0f, 1.0f);
    auto cb = CallFunc::create([this, faceUp]() {
        // 切换可见性
        _frontNode->setVisible(faceUp);
        _backSprite->setVisible(!faceUp);
        });
    // 为避免视觉闪烁，先把后方节点确保可见到结束切换
    // 先收缩（视觉上 X 缩放到 0），切换贴图，再扩展
    // NOTE: 这里改变的是本节点的 scaleX，前后子节点都随之缩放
    auto seq = Sequence::create(shrink, cb, expand, nullptr);
    this->runAction(seq);
}

bool CardView::isFaceUp() const
{
    return _isFaceUp;
}

void CardView::setCardVisible(bool visible)
{
    // 保留兼容行为（visible -> front visible）
    setFaceUp(visible, false);
}

int CardView::getCardId() const
{
    auto modelLock = _cardModel.lock();
    return modelLock ? modelLock->getId() : -1;
}

CardFaceType CardView::getCardFace() const
{
    auto modelLock = _cardModel.lock();
    return modelLock ? modelLock->getCardFace() : CardFaceType::CFT_NONE;
}

CardSuitType CardView::getCardSuit() const
{
    auto modelLock = _cardModel.lock();
    return modelLock ? modelLock->getCardSuit() : CardSuitType::CST_NONE;
}

cocos2d::Vec2 CardView::getCurrentPosition() const
{
    return getPosition();
}

void CardView::playMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback)
{
    auto moveAction = MoveTo::create(duration, targetPos);
    if (completionCallback) {
        auto sequence = Sequence::create(moveAction, CallFunc::create(completionCallback), nullptr);
        runAction(sequence);
    }
    else {
        runAction(moveAction);
    }
}

void CardView::playReverseMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback)
{
    playMoveAnimation(targetPos, duration, completionCallback);
}

void CardView::setClickCallback(std::function<void(int)> callback)
{
    _clickCallback = callback;
}

bool CardView::onTouchBegan(Touch* touch, Event* event)
{
    auto target = event->getCurrentTarget();
    auto locationInNode = target->convertToNodeSpace(touch->getLocation());
    auto s = getContentSize();
    Rect rect(0, 0, s.width, s.height);
    if (rect.containsPoint(locationInNode)) {
        return true;
    }
    return false;
}

void CardView::onTouchEnded(Touch* touch, Event* event)
{
    auto target = event->getCurrentTarget();
    auto locationInNode = target->convertToNodeSpace(touch->getLocation());
    auto s = getContentSize();
    Rect rect(0, 0, s.width, s.height);
    if (rect.containsPoint(locationInNode)) {
        if (_clickCallback) {
            auto modelLock = _cardModel.lock();
            if (modelLock) {
                _clickCallback(modelLock->getId());
            }
            else {
                CCLOG("CardView::onTouchEnded - model expired, ignoring click");
            }
        }
    }
}

void CardView::onTouchCancelled(Touch* touch, Event* event)
{
    // no-op for now
}

void CardView::onExit()
{
    getEventDispatcher()->removeEventListenersForTarget(this);
    Node::onExit();
}