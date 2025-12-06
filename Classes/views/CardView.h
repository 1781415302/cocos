#pragma once
// Classes/views/CardView.h
#ifndef CardView_h
#define CardView_h

#include "cocos2d.h"
#include "utils/CardEnums.h"
#include <functional> // For std::function
#include <memory>

// 前向声明
class CardModel;

/**
 * @brief 卡牌视图
 * @details 卡牌视图负责呈现一张牌的正/背面、触摸、移动动画等
 */
class CardView : public cocos2d::Node
{
public:
    static CardView* create(const std::shared_ptr<CardModel>& cardModel);

    bool init(const std::shared_ptr<CardModel>& cardModel);

    // 切换显示正/背面（animate=true 做简单翻转动画）
    void setFaceUp(bool faceUp, bool animate = true);
    bool isFaceUp() const;

    // 兼容旧接口：根据模型状态显示/隐藏（仍保留）
    void setCardVisible(bool visible);

    int getCardId() const;
    CardFaceType getCardFace() const;
    CardSuitType getCardSuit() const;
    cocos2d::Vec2 getCurrentPosition() const;

    void playMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback = nullptr);
    void playReverseMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback = nullptr);

    void setClickCallback(std::function<void(int)> callback);

private:
    // weak_ptr 避免生命周期依赖
    std::weak_ptr<CardModel> _cardModel;
    cocos2d::Node* _frontNode = nullptr; ///< 正面（compound node）
    cocos2d::Sprite* _backSprite = nullptr; ///< 背面精灵
    std::function<void(int)> _clickCallback;

    bool _isFaceUp = false;

    // 创建正面组合视图（background + big number + small number + suit）
    cocos2d::Node* createFrontNode(CardFaceType faceType, CardSuitType suitType);

    // 资源文件名生成
    std::string bigNumberFilename(CardFaceType face, CardSuitType suit) const;
    std::string smallNumberFilename(CardFaceType face, CardSuitType suit) const;
    std::string suitFilename(CardSuitType suit) const;
    std::string cardBackFilename() const;
    std::string cardGeneralFilename() const;

    // 触摸回调
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);

    virtual void onExit() override;
};

#endif /* CardView_h */