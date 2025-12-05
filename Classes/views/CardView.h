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
 * @details 负责显示卡牌图片并处理交互回调，具体业务由 Controller 处理
 */
class CardView : public cocos2d::Node
{
public:
    /**
     * @brief 创建 CardView（使用 shared_ptr<CardModel>）
     * @param cardModel 共享的卡牌模型
     */
    static CardView* create(const std::shared_ptr<CardModel>& cardModel);

    /**
     * @brief 初始化
     */
    bool init(const std::shared_ptr<CardModel>& cardModel);

    /**
     * @brief 设置正面/背面显示
     */
    void setCardVisible(bool visible);

    /**
     * @brief 获取卡牌 ID
     */
    int getCardId() const;

    /**
     * @brief 获取卡牌点数
     */
    CardFaceType getCardFace() const;

    /**
     * @brief 获取卡牌花色
     */
    CardSuitType getCardSuit() const;

    /**
     * @brief 当前节点位置
     */
    cocos2d::Vec2 getCurrentPosition() const;

    /**
     * @brief 播放移动动画
     */
    void playMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback = nullptr);

    /**
     * @brief 播放反向移动动画（目前直接同 playMoveAnimation）
     */
    void playReverseMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback = nullptr);

    /**
     * @brief 设置点击回调（传出 cardId）
     */
    void setClickCallback(std::function<void(int)> callback);

private:
    // 使用 weak_ptr 持有模型，避免视图持有强引用或悬挂裸指针
    std::weak_ptr<CardModel> _cardModel; ///< 指向模型的弱引用
    cocos2d::Sprite* _frontSprite = nullptr; ///< 正面图片
    cocos2d::Sprite* _backSprite = nullptr; ///< 背面图片
    std::function<void(int)> _clickCallback; ///< 点击回调

    // 辅助函数
    cocos2d::Sprite* loadCardSprite(CardFaceType faceType, CardSuitType suitType);

    // 触摸回调
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);

    // 节点退出回调（用于移除监听器）
    virtual void onExit() override;
};

#endif /* CardView_h */