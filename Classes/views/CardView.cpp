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

    // 创建正面节点（包含数字和花色）
    _frontNode = createFrontNode(face, suit);
    if (!_frontNode) {
        // 如果创建失败，使用空节点占位
        _frontNode = Node::create();
    }

    // 创建背面图
    _backSprite = Sprite::create(cardBackFilename());
    if (!_backSprite) {
        CCLOG("CardView::init - failed to load '%s', using placeholder", cardBackFilename().c_str());
        _backSprite = Sprite::create();
    }

    // 根据 front 或 back 的尺寸确定 contentSize
    Size contentSz = Size::ZERO;
    if (_frontNode->getChildrenCount() > 0) {
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

    // 将 front/back 添加为子节点并居中
    _backSprite->setPosition(getContentSize() * 0.5f);
    _frontNode->setPosition(getContentSize() * 0.5f);
    addChild(_backSprite);
    addChild(_frontNode);

    // 根据模型状态初始化正反面显示
    _isFaceUp = (modelLock->getStatus() != CardStatus::COVERED);
    if (!_isFaceUp) {
        _backSprite->setVisible(true);
        _frontNode->setVisible(false);
    }
    else {
        _backSprite->setVisible(false);
        _frontNode->setVisible(true);
    }

    // 设置初始位置为模型中记录的位置
    setPosition(modelLock->getPosition());

    // 注意：不要在 init 中注册触摸监听器，因为重父化（remove/add）会触发 onExit/onEnter，
    // 如果在 init 注册则在重加时不会自动恢复监听器。监听器在 onEnter 中注册，在 onExit 中移除。

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

// 将 CardFaceType 转为显示字符串 "A","2",...,"10","J","Q","K"
static std::string faceToStr(CardFaceType face) {
    int v = static_cast<int>(face);
    if (v == 0) return "A";
    if (v >= 1 && v <= 9) {
        return std::to_string(v + 1);
    }
    if (v == 10) return "J";
    if (v == 11) return "Q";
    if (v == 12) return "K";
    return "";
}

std::string CardView::bigNumberFilename(CardFaceType face, CardSuitType suit) const
{
    bool isRed = (suit == CardSuitType::CST_DIAMONDS || suit == CardSuitType::CST_HEARTS);
    std::string color = isRed ? "big_red_" : "big_black_";
    std::string key = faceToStr(face);
    if (key.empty()) return "";
    return "number/" + color + key + ".png";
}

std::string CardView::smallNumberFilename(CardFaceType face, CardSuitType suit) const
{
    bool isRed = (suit == CardSuitType::CST_DIAMONDS || suit == CardSuitType::CST_HEARTS);
    std::string color = isRed ? "small_red_" : "small_black_";
    std::string key = faceToStr(face);
    if (key.empty()) return "";
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
        bgSize = Size(150.0f, 200.0f);
        bg->setContentSize(bgSize);
    }

    // 大数字
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
        bigNum = Sprite::create();
    }
    bigNum->setPosition(0, 0);
    front->addChild(bigNum);

    // 小数字
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
    smallNum->setAnchorPoint(Vec2(0, 1));
    smallNum->setPosition(-bgSize.width * 0.45f, bgSize.height * 0.45f);
    front->addChild(smallNum);

    // 花色图标
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
    suitSp->setAnchorPoint(Vec2(1, 1));
    suitSp->setPosition(bgSize.width * 0.45f, bgSize.height * 0.45f);
    front->addChild(suitSp);

    front->setContentSize(bgSize);

    return front;
}

void CardView::setFaceUp(bool faceUp, bool animate)
{
    if (_isFaceUp == faceUp) return;
    _isFaceUp = faceUp;

    if (!animate) {
        _frontNode->setVisible(faceUp);
        _backSprite->setVisible(!faceUp);
        return;
    }

    float half = 0.15f;
    auto shrink = ScaleTo::create(half, 0.0f, 1.0f);
    auto expand = ScaleTo::create(half, 1.0f, 1.0f);
    auto cb = CallFunc::create([this, faceUp]() {
        _frontNode->setVisible(faceUp);
        _backSprite->setVisible(!faceUp);
        });
    auto seq = Sequence::create(shrink, cb, expand, nullptr);
    this->runAction(seq);
}

bool CardView::isFaceUp() const
{
    return _isFaceUp;
}

void CardView::setCardVisible(bool visible)
{
    setFaceUp(visible, true);
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
    // 不做额外处理
}

// 在 onEnter 中创建并注册触摸监听器；在 onExit 中移除（保证重父化时会被正确重建）
void CardView::onEnter()
{
    Node::onEnter();

    if (!_touchListener) {
        auto listener = EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = CC_CALLBACK_2(CardView::onTouchBegan, this);
        listener->onTouchEnded = CC_CALLBACK_2(CardView::onTouchEnded, this);
        listener->onTouchCancelled = CC_CALLBACK_2(CardView::onTouchCancelled, this);
        _touchListener = listener;
        getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);
    }
}

void CardView::onExit()
{
    if (_touchListener) {
        getEventDispatcher()->removeEventListener(_touchListener);
        _touchListener = nullptr;
    }
    else {
        // 额外保险：移除所有与该 target 关联的 listener（防止外部意外添加）
        getEventDispatcher()->removeEventListenersForTarget(this);
    }
    Node::onExit();
}