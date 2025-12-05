#pragma once
// Classes/views/CardView.h
#ifndef CardView_h
#define CardView_h

#include "cocos2d.h"
#include "utils/CardEnums.h"
#include <functional> // For std::function

// 前向声明，避免循环依赖
class CardModel;

/**
 * @brief 卡牌视图组件
 * @details 负责显示卡牌图片，处理触摸事件，并提供动画接口。
 *          不包含业务逻辑，仅负责UI展示和用户输入捕获。
 */
class CardView : public cocos2d::Node
{
public:
    /**
     * @brief 创建卡牌视图
     * @param cardModel 指向关联的卡牌数据模型的常量指针
     * @return 成功则返回实例指针，失败返回 nullptr
     */
    static CardView* create(const CardModel* cardModel);

    /**
     * @brief 初始化卡牌视图
     * @param cardModel 指向关联的卡牌数据模型的常量指针
     * @return 成功则返回 true，失败返回 false
     */
    bool init(const CardModel* cardModel);

    /**
     * @brief 设置卡牌是否可见（正面朝上）
     * @param visible true 显示正面图片，false 显示背面图片
     */
    void setCardVisible(bool visible);

    /**
     * @brief 获取卡牌的 ID
     * @return 卡牌的唯一标识符
     */
    int getCardId() const;

    /**
     * @brief 获取卡牌的点数
     * @return 卡牌的点数（CardFaceType）
     */
    CardFaceType getCardFace() const;

    /**
     * @brief 获取卡牌的花色
     * @return 卡牌的花色（CardSuitType）
     */
    CardSuitType getCardSuit() const;

    /**
     * @brief 获取卡牌的当前位置
     * @return 卡牌当前的坐标
     */
    cocos2d::Vec2 getCurrentPosition() const;

    /**
     * @brief 播放移动动画
     * @param targetPos 动画的目标位置
     * @param duration 动画持续时间
     * @param completionCallback 动画完成后的回调函数
     */
    void playMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback = nullptr);

    /**
     * @brief 播放反向移动动画（用于回退）
     * @param targetPos 动画的目标位置（通常是原始位置）
     * @param duration 动画持续时间
     * @param completionCallback 动画完成后的回调函数
     */
    void playReverseMoveAnimation(cocos2d::Vec2 targetPos, float duration, std::function<void()> completionCallback = nullptr);

    /**
     * @brief 设置点击回调函数
     * @param callback 当卡牌被点击时调用的函数
     */
    void setClickCallback(std::function<void(int)> callback);

private:
    // 私有成员变量
    const CardModel* _cardModel; ///< 指向关联的卡牌数据模型的常量指针
    cocos2d::Sprite* _frontSprite; ///< 正面图片精灵
    cocos2d::Sprite* _backSprite; ///< 背面图片精灵
    std::function<void(int)> _clickCallback; ///< 点击回调函数

    // 私有方法
    /**
     * @brief 加载卡牌图片
     * @param faceType 卡牌点数
     * @param suitType 卡牌花色
     * @return 成功则返回精灵指针，失败返回 nullptr
     */
    cocos2d::Sprite* loadCardSprite(CardFaceType faceType, CardSuitType suitType);

    /**
     * @brief 触摸事件处理函数
     * @param touch 触摸对象
     * @param event 事件对象
     * @return 是否消费了触摸事件
     */
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
};

#endif /* CardView_h */