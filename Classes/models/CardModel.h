// Classes/models/CardModel.h
#ifndef CardModel_h
#define CardModel_h

#include "cocos2d.h" // Include cocos2d headers if using Vec2, etc.
#include "utils/CardEnums.h" // Include CardEnums header

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
     * @brief 构造函数
     * @param id 卡牌的唯一标识符
     * @param faceType 卡牌的点数
     * @param suitType 卡牌的花色
     * @param position 卡牌在主牌区的位置
     * @param status 卡牌的初始状态（覆盖/翻开）
     */
    CardModel(int id, CardFaceType faceType, CardSuitType suitType, cocos2d::Vec2 position, CardStatus status = CardStatus::COVERED);

    // --- Getter 方法 (Getters) ---
    /**
     * @brief 获取卡牌ID
     * @return 卡牌的唯一标识符
     */
    int getId() const;

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

    // --- Setter 方法 (Setters) ---
    /**
     * @brief 设置卡牌位置
     * @param pos 新的坐标
     */
    void setPosition(cocos2d::Vec2 pos);

    /**
     * @brief 设置卡牌状态
     * @param status 新的状态
     */
    void setStatus(CardStatus status);

    // --- 其他可能需要的方法 ---
    // 例如：isExposed(), isCovered() 等，根据需要添加

private:
    // --- 私有成员变量 ---
    int _id;                ///< 卡牌的唯一标识符
    CardFaceType _cardFace; ///< 卡牌的点数
    CardSuitType _cardSuit; ///< 卡牌的花色
    cocos2d::Vec2 _position;///< 卡牌在主牌区的坐标
    CardStatus _status;     ///< 卡牌的状态 (覆盖/翻开)
};

#endif /* CardModel_h */