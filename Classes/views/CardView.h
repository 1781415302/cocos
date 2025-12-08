#pragma once
// Classes/views/CardView.h
#ifndef CardView_h
#define CardView_h

#include "cocos2d.h"
#include "utils/CardEnums.h"
#include <functional> // For std::function
#include <memory>

// 前置声明
class CardModel;

/**
 * @brief 卡牌视图
 * @details 卡牌视图负责显示单张牌的正面/背面、播放移动动画，并响应点击事件。
 */
class CardView : public cocos2d::Node
{
public:
    static CardView* create(const std::shared_ptr<CardModel>& cardModel);

    bool init(const std::shared_ptr<CardModel>& cardModel);

    // 切换正/反面，animate=true 时播放翻转动画
    void setFaceUp(bool faceUp, bool animate = true);
    bool isFaceUp() const;

    // 根据模型状态显示/隐藏（对于本项目，visible 表示是否显示为“正面”）
    void setCardVisible(bool visible);

    int getCardId() const;
    CardFaceType getCardFace() const;
    CardSuitType getCardSuit() const;
    cocos2d::Vec2 getCurrentPosition() const;

    // 播放移动动画，完成后调用回调（可空）
    void playMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback = nullptr);
    void playReverseMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback = nullptr);

    // 设置点击回调：参数为 cardId
    void setClickCallback(std::function<void(int)> callback);

private:
    // card model 使用 weak_ptr 引用，视图不拥有模型生命周期
    std::weak_ptr<CardModel> _cardModel;
    cocos2d::Node* _frontNode = nullptr;   ///< 正面组合节点（背景 + 数字 + 花色）
    cocos2d::Sprite* _backSprite = nullptr;///< 背面图片
    std::function<void(int)> _clickCallback;

    bool _isFaceUp = false;

    // 将触摸监听器保存为成员，便于在 onEnter/onExit 管理
    cocos2d::EventListenerTouchOneByOne* _touchListener = nullptr;

    // 创建正面子节点（背景 + 大数字 + 小数字 + 花色）
    cocos2d::Node* createFrontNode(CardFaceType faceType, CardSuitType suitType);

    // 资源文件名构造
    std::string bigNumberFilename(CardFaceType face, CardSuitType suit) const;
    std::string smallNumberFilename(CardFaceType face, CardSuitType suit) const;
    std::string suitFilename(CardSuitType suit) const;
    std::string cardBackFilename() const;
    std::string cardGeneralFilename() const;

    // 触摸回调实现
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);

    // 在 onEnter 中创建并注册触摸监听器；在 onExit 中移除监听器
    virtual void onEnter() override;
    virtual void onExit() override;
};

#endif /* CardView_h */