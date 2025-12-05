// Classes/views/CardView.cpp
#include "CardView.h"
#include "models/CardModel.h" // 包含模型头文件
#include "utils/CardEnums.h"
#include <string> // 用于构造文件路径
#include <sstream> // 用于整数到字符串的转换
#include <memory>

using namespace cocos2d;

CardView* CardView::create(const std::shared_ptr<CardModel>& cardModel)
{
    CardView* pRet = new (std::nothrow) CardView();
    if (pRet && pRet->init(cardModel))
    {
        pRet->autorelease(); // 交给 Cocos2d-x 管理内存
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool CardView::init(const std::shared_ptr<CardModel>& cardModel)
{
    if (!Node::init())
    {
        return false;
    }

    if (!cardModel) {
        CCLOG("CardView::init - null cardModel");
        return false;
    }

    _cardModel = cardModel;

    // 加载正面和背面的精灵
    auto modelLock = _cardModel.lock();
    if (!modelLock) return false;

    auto face = modelLock->getCardFace();
    auto suit = modelLock->getCardSuit();

    _frontSprite = loadCardSprite(face, suit);
    if (!_frontSprite) {
        // 使用占位精灵以避免 contentSize 为空
        _frontSprite = Sprite::create(); // 空精灵
        if (!_frontSprite) {
            CCLOG("CardView::init - failed to create placeholder front sprite");
            return false;
        }
    }

    _backSprite = Sprite::create("res/card_back.png"); // 假设通用的背面图片
    if (!_backSprite) {
        // 回退到空精灵
        _backSprite = Sprite::create();
        CCLOG("CardView::init - failed to load card_back.png, using placeholder");
    }

    // 确保 contentSize 与卡牌精灵匹配，以便触摸区域正确
    Size frontSize = _frontSprite->getContentSize();
    Size backSize = _backSprite->getContentSize();
    if (frontSize.width > 0 && frontSize.height > 0) {
        setContentSize(frontSize);
    }
    else if (backSize.width > 0 && backSize.height > 0) {
        setContentSize(backSize);
    }
    else {
        // 如果两个精灵都为空，则回退到合理的默认大小
        setContentSize(Size(150.0f, 200.0f));
    }

    // 如果卡牌为覆盖状态，初始显示背面
    if (modelLock->getStatus() == CardStatus::COVERED) {
        _backSprite->setVisible(true);
        _frontSprite->setVisible(false);
    }
    else {
        _backSprite->setVisible(false);
        _frontSprite->setVisible(true);
    }

    // 将精灵在节点内居中
    _backSprite->setPosition(getContentSize() * 0.5f);
    _frontSprite->setPosition(getContentSize() * 0.5f);

    // 将精灵添加为子节点（背面在下，正面在上）
    addChild(_backSprite);
    addChild(_frontSprite);

    // 根据模型设置初始位置
    setPosition(modelLock->getPosition());

    // 启用触摸
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true); // 阻止触摸事件向下传递
    listener->onTouchBegan = CC_CALLBACK_2(CardView::onTouchBegan, this);
    listener->onTouchEnded = CC_CALLBACK_2(CardView::onTouchEnded, this);
    listener->onTouchCancelled = CC_CALLBACK_2(CardView::onTouchCancelled, this);

    getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);

    return true;
}

void CardView::setCardVisible(bool visible)
{
    if (visible)
    {
        _frontSprite->setVisible(true);
        _backSprite->setVisible(false);
    }
    else
    {
        _frontSprite->setVisible(false);
        _backSprite->setVisible(true);
    }
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
    // 目前复用 playMoveAnimation
    playMoveAnimation(targetPos, duration, completionCallback);
}

void CardView::setClickCallback(std::function<void(int)> callback)
{
    _clickCallback = callback;
}

cocos2d::Sprite* CardView::loadCardSprite(CardFaceType faceType, CardSuitType suitType)
{
    if (faceType == CardFaceType::CFT_NONE || suitType == CardSuitType::CST_NONE) {
        log("Warning: Invalid card type for loading sprite.");
        return nullptr;
    }

    std::ostringstream oss;
    oss << "res/card_" << static_cast<int>(suitType) << "_" << static_cast<int>(faceType) << ".png";
    std::string filename = oss.str();

    auto sprite = Sprite::create(filename);
    if (!sprite) {
        log("Error loading card sprite: %s", filename.c_str());
    }
    return sprite;
}

bool CardView::onTouchBegan(Touch* touch, Event* event)
{
    auto target = event->getCurrentTarget();
    auto locationInNode = target->convertToNodeSpace(touch->getLocation());
    // 使用节点的 contentSize 来判断是否被点击
    auto s = getContentSize();
    auto rect = Rect(0, 0, s.width, s.height);

    if (rect.containsPoint(locationInNode)) {
        // 可以在此处高亮或表示选中
        // 目前只返回 true，表示处理了触摸按下
        return true;
    }
    return false;
}

void CardView::onTouchEnded(Touch* touch, Event* event)
{
    // 检查触摸是否在该节点内结束
    auto target = event->getCurrentTarget();
    auto locationInNode = target->convertToNodeSpace(touch->getLocation());
    auto s = getContentSize();
    auto rect = Rect(0, 0, s.width, s.height);
    if (rect.containsPoint(locationInNode)) {
        // 触摸在该节点内按下并释放，视为点击
        if (_clickCallback) {
            auto modelLock = _cardModel.lock();
            if (modelLock) {
                _clickCallback(modelLock->getId()); // 传递卡牌 ID
            }
            else {
                // 模型已过期 -> 不处理
                CCLOG("CardView::onTouchEnded - model expired, ignoring click");
            }
        }
    }
}

void CardView::onTouchCancelled(Touch* touch, Event* event)
{
    // 目前取消不做特殊处理，未来可用于移除高亮等
}

void CardView::onExit()
{
    // 移除为该目标注册的监听器，避免悬挂回调
    getEventDispatcher()->removeEventListenersForTarget(this);
    Node::onExit();
}