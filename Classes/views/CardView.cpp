// Classes/views/CardView.cpp
#include "CardView.h"
#include "models/CardModel.h" // Include the model header
#include "utils/CardEnums.h"
#include <string> // For constructing file paths
#include <sstream> // For integer to string conversion

using namespace cocos2d;

CardView* CardView::create(const CardModel* cardModel)
{
    CardView* pRet = new (std::nothrow) CardView();
    if (pRet && pRet->init(cardModel))
    {
        pRet->autorelease(); // Let Cocos2d-x manage memory
        return pRet;
    }
    else
    {
        delete pRet;
        pRet = nullptr;
        return nullptr;
    }
}

bool CardView::init(const CardModel* cardModel)
{
    if (!Node::init())
    {
        return false;
    }

    _cardModel = cardModel;

    // Load front and back sprites
    auto face = _cardModel->getCardFace();
    auto suit = _cardModel->getCardSuit();
    _frontSprite = loadCardSprite(face, suit);
    if (!_frontSprite) return false; // Load failed

    _backSprite = Sprite::create("res/card_back.png"); // Assume a generic back image
    if (!_backSprite) {
        log("Error loading card back sprite.");
        return false;
    }

    // Initially show the back of the card if it's covered
    if (_cardModel->getStatus() == CardStatus::COVERED) {
        _backSprite->setVisible(true);
        _frontSprite->setVisible(false);
    }
    else {
        _backSprite->setVisible(false);
        _frontSprite->setVisible(true);
    }

    // Add sprites as children
    addChild(_backSprite);
    addChild(_frontSprite);

    // Set initial position based on the model
    setPosition(_cardModel->getPosition());

    // Enable touch
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true); // Prevents touch from passing through
    listener->onTouchBegan = CC_CALLBACK_2(CardView::onTouchBegan, this);
    listener->onTouchEnded = CC_CALLBACK_2(CardView::onTouchEnded, this);

    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

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
    return _cardModel ? _cardModel->getId() : -1;
}

CardFaceType CardView::getCardFace() const
{
    return _cardModel ? _cardModel->getCardFace() : CardFaceType::CFT_NONE;
}

CardSuitType CardView::getCardSuit() const
{
    return _cardModel ? _cardModel->getCardSuit() : CardSuitType::CST_NONE;
}

cocos2d::Vec2 CardView::getCurrentPosition() const
{
    return getPosition();
}

void CardView::playMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback)
{
    auto moveAction = MoveTo::create(duration, targetPos);
    auto sequence = Sequence::create(moveAction, CallFunc::create(completionCallback), nullptr);
    runAction(sequence);
}

void CardView::playReverseMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback)
{
    // This function is identical to playMoveAnimation for now.
    // The difference might come in how the controller manages the animation call or passes parameters.
    playMoveAnimation(targetPos, duration, completionCallback);
}

void CardView::setClickCallback(std::function<void(int)> callback)
{
    _clickCallback = callback;
}

cocos2d::Sprite* CardView::loadCardSprite(CardFaceType faceType, CardSuitType suitType)
{
    if (faceType == CardFaceType::CFT_NONE || suitType == CardSuitType::CST_NONE) {
        log("Error: Invalid card type for loading sprite.");
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
    auto s = target->getContentSize();
    auto rect = Rect(0, 0, s.width, s.height);

    if (rect.containsPoint(locationInNode)) {
        // Highlight or indicate selection if needed
        // For now, just return true to indicate we've handled the touch start
        return true;
    }
    return false;
}

void CardView::onTouchEnded(Touch* touch, Event* event)
{
    // Touch started and ended within this node, consider it a click
    if (_clickCallback && _cardModel) { // Ensure callback and model exist
        _clickCallback(_cardModel->getId()); // Pass the card's ID
    }
}
