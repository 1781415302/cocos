#pragma once
// Classes/models/CardModel.h
#ifndef CardModel_h
#define CardModel_h

#include "cocos2d.h" // Include cocos2d headers if using Vec2, etc.
#include "utils/CardEnums.h" // Include CardEnums header
#include <memory>

/**
 * @brief 卡牌状态枚举
 */
enum class CardStatus {
    COVERED,    // 覆盖状态
    EXPOSED     // 翻开状态
};

/**
 * @brief 卡牌数据模型
 * @details 存储卡牌的ID、花色、点数、位置和状态等运行时信息。
 */
class CardModel
{
public:
    // --- 构造函数 ---
    /**
     * @brief 构造函数（带 id）
     * @param id 卡牌的唯一标识符
     * @param faceType 卡牌的点数
     * @param suitType 卡牌的花色
     * @param position 卡牌在主牌区的位置
     * @param status 卡牌的初始状态（覆盖/翻开）
     */
    CardModel(int id, CardFaceType faceType, CardSuitType suitType, cocos2d::Vec2 position, CardStatus status = CardStatus::COVERED);

    /**
     * @brief 兼容构造函数（便捷重载，符合已有生成器使用顺序）
     * @param suitType 卡牌的花色
     * @param faceType 卡牌的点数
     * @param position 卡牌位置
     * @param isFaceUp 是否为正面（true 表示翻开）
     *
     * 注意：此构造函数会把 id 置为 -1，调用方可在插入 GameModel 时由 GameModel 分配最终 id。
     */
    CardModel(CardSuitType suitType, CardFaceType faceType, cocos2d::Vec2 position, bool isFaceUp = false);

    // --- Getter 方法 (Getters) ---
    /**
     * @brief 获取卡牌ID
     * @return 卡牌的唯一标识符
     */
    int getId() const;

    /**
     * @brief 设置卡牌ID
     */
    void setId(int id);

    /**
     * @brief 获取卡牌点数
     * @return 卡牌的点数 (CardFaceType)
     */
    CardFaceType getCardFace() const;

    /**
     * @brief 获取卡牌花色
     * @return 卡牌的花色 (CardSuitType)
     */
    CardSuitType getCardSuit() const;

    /**
     * @brief 获取卡牌位置
     * @return 卡牌在主牌区的坐标 (cocos2d::Vec2)
     */
    cocos2d::Vec2 getPosition() const;

    /**
     * @brief 获取卡牌状态
     * @return 卡牌的状态 (CardStatus)
     */
    CardStatus getStatus() const;

    // --- 状态查询/设置 ---
    /**
     * @brief 卡牌是否为正面（翻开）
     */
    bool isFaceUp() const;

    /**
     * @brief 将卡牌设置为正面或背面
     */
    void setFaceUp(bool faceUp);

    /**
     * @brief 卡牌是否可见（用于表示是否在场景中显示）
     */
    bool isVisible() const;

    /**
     * @brief 设置可见性
     */
    void setVisible(bool visible);

    // --- Setter 方法 (Setters) ---
    /**
     * @brief 设置卡牌位置
     * @param pos 新的坐标
     */
    void setPosition(cocos2d::Vec2 pos);

    /**
     * @brief 设置卡牌状态枚举
     * @param status 新的状态
     */
    void setStatus(CardStatus status);

private:
    // --- 私有成员变量 ---
    int _id = -1;                ///< 卡牌的唯一标识符（默认 -1，表示未分配）
    CardFaceType _cardFace; ///< 卡牌的点数
    CardSuitType _cardSuit; ///< 卡牌的花色
    cocos2d::Vec2 _position;///< 卡牌在主牌区的坐标
    CardStatus _status;     ///< 卡牌的状态 (覆盖/翻开)
    bool _visible = true;   ///< 是否可见（用于显示控制）
};

#endif /* CardModel_h */