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

    // 创建前面节点（点数和花色）
    _frontNode = createFrontNode(face, suit);
    if (!_frontNode) {
        // 兜底：创建空节点
        _frontNode = Node::create();
    }

    // 加载卡背图片（Resources 目录下的 card_back.png）
    _backSprite = Sprite::create(cardBackFilename());
    if (!_backSprite) {
        CCLOG("CardView::init - failed to load '%s', using placeholder", cardBackFilename().c_str());
        _backSprite = Sprite::create();
    }

    // 根据 front 或 back 的大小来设置 contentSize
    Size contentSz = Size::ZERO;
    // 如果 front 的第一个子节点是 Sprite，则尝试使用其 size
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

    // 将 back/front 添加到节点并居中
    _backSprite->setPosition(getContentSize() * 0.5f);
    _frontNode->setPosition(getContentSize() * 0.5f);
    addChild(_backSprite);
    addChild(_frontNode);

    // 根据 model 的状态初始化正反面显示（不做动画）
    _isFaceUp = (modelLock->getStatus() != CardStatus::COVERED);
    //setFaceUp(_isFaceUp, false);
    if (!_isFaceUp) {
		_backSprite->setVisible(true);  
		_frontNode->setVisible(false);
    }
    else {
		_backSprite->setVisible(false);
		_frontNode->setVisible(true);
    }

    // 将视图初始位置设置为 model 中的 position
    setPosition(modelLock->getPosition());

    // 触摸事件监听器
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

// 将 CardFaceType 映射为显示字符串："A","2",...,"10","J","Q","K"
// 对应 CardEnums.h 中的枚举：CFT_ACE=0, CFT_TWO=1, ..., CFT_TEN=9, CFT_JACK=10, CFT_QUEEN=11, CFT_KING=12
static std::string faceToStr(CardFaceType face) {
    int v = static_cast<int>(face);
    if (v == 0) return "A";
    if (v >= 1 && v <= 9) {
        // CFT_TWO (1) -> "2", ..., CFT_TEN (9) -> "10"
        return std::to_string(v + 1);
    }
    if (v == 10) return "J";
    if (v == 11) return "Q";
    if (v == 12) return "K";
    // 非法值返回空字符串，上层会处理资源缺失情况
    return "";
}

std::string CardView::bigNumberFilename(CardFaceType face, CardSuitType suit) const
{
    // big_red_A.png / big_black_2.png 等
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
        // 背景尺寸无效，使用默认值并设置到背景 Sprite
        bgSize = Size(150.0f, 200.0f);
        bg->setContentSize(bgSize);
    }

    // 大号数字
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
        // 若图片缺失，使用占位 Sprite（空）
        bigNum = Sprite::create();
    }
    bigNum->setPosition(0, 0);
    front->addChild(bigNum);

    // 小号数字（左上角）
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
    smallNum->setAnchorPoint(Vec2(0, 1)); // 锚点左上
    smallNum->setPosition(-bgSize.width * 0.45f, bgSize.height * 0.45f);
    front->addChild(smallNum);

    // 花色图标（右上角）
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
    suitSp->setAnchorPoint(Vec2(1, 1)); // 锚点右上
    suitSp->setPosition(bgSize.width * 0.45f, bgSize.height * 0.45f);
    front->addChild(suitSp);

    // 设置 front 的 contentSize 为背景大小
    front->setContentSize(bgSize);

    return front;
}

void CardView::setFaceUp(bool faceUp, bool animate)
{
    // 状态相同时直接返回
    if (_isFaceUp == faceUp) return;
    _isFaceUp = faceUp;

    // 如果不动画，直接切换可见性
    if (!animate) {
        _frontNode->setVisible(faceUp);
        _backSprite->setVisible(!faceUp);
        return;
    }

    // 通过缩放 X 轴实现翻转动画（1->0, 切换可见性, 0->1）
    float half = 0.2f;
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
    // visible 表示显示正面
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
    // 暂无处理
}

void CardView::onExit()
{
    getEventDispatcher()->removeEventListenersForTarget(this);
    Node::onExit();
}
