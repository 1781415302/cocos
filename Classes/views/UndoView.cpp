#include "UndoView.h"
#include "cocos2d.h"

using namespace cocos2d;

UndoView* UndoView::create(const std::string& imageFile)
{
    UndoView* pRet = new (std::nothrow) UndoView();
    if (pRet && pRet->init(imageFile)) {
        pRet->autorelease();
        return pRet;
    }
    delete pRet;
    return nullptr;
}

bool UndoView::init(const std::string& imageFile)
{
    if (!Node::init()) return false;

    if (!imageFile.empty() && FileUtils::getInstance()->isFileExist(imageFile)) {
        _sprite = Sprite::create(imageFile);
    }
    else {
        _sprite = Sprite::create(imageFile); // ÈÔ³¢ÊÔ´´½¨£¬ÈôÊ§°ÜÔòÎª¿Õ
    }

    if (!_sprite) {
        CCLOG("UndoView::init - failed to load '%s', using placeholder", imageFile.c_str());
        _sprite = Sprite::create();
        _sprite->setContentSize(Size(64.0f, 64.0f));
    }

    addChild(_sprite);
    setContentSize(_sprite->getContentSize());
    _sprite->setPosition(getContentSize() * 0.5f);

    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = CC_CALLBACK_2(UndoView::onTouchBegan, this);
    listener->onTouchEnded = CC_CALLBACK_2(UndoView::onTouchEnded, this);
    listener->onTouchCancelled = CC_CALLBACK_2(UndoView::onTouchCancelled, this);
    getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener, this);

    return true;
}

void UndoView::setClickCallback(std::function<void()> callback)
{
    _clickCallback = callback;
}

Size UndoView::getButtonSize() const
{
    return getContentSize();
}

bool UndoView::onTouchBegan(Touch* touch, Event* event)
{
    auto target = event->getCurrentTarget();
    auto locationInNode = target->convertToNodeSpace(touch->getLocation());
    auto s = getContentSize();
    Rect rect(0, 0, s.width, s.height);
    if (rect.containsPoint(locationInNode)) {
        this->setScale(0.95f);
        return true;
    }
    return false;
}

void UndoView::onTouchEnded(Touch* touch, Event* event)
{
    this->setScale(1.0f);
    auto target = event->getCurrentTarget();
    auto locationInNode = target->convertToNodeSpace(touch->getLocation());
    auto s = getContentSize();
    Rect rect(0, 0, s.width, s.height);
    if (rect.containsPoint(locationInNode)) {
        if (_clickCallback) _clickCallback();
    }
}

void UndoView::onTouchCancelled(Touch* touch, Event* event)
{
    this->setScale(1.0f);
}

void UndoView::onExit()
{
    getEventDispatcher()->removeEventListenersForTarget(this);
    Node::onExit();
}