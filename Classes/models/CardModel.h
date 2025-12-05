// test/Classes/models/CardModel.h
#ifndef CardModel_h
#define CardModel_h

#include "cocos2d.h"
#include "utils/CardEnums.h"

// @brief 表示游戏中一张牌的数据模型
class CardModel
{
public:
    // @brief 构造函数
    CardModel(CardSuitType suit, CardFaceType face, cocos2d::Vec2 position, bool isFaceUp);

    // @brief 获取花色
    CardSuitType getSuit() const;
    // @brief 获取点数
    CardFaceType getFace() const;
    // @brief 获取位置
    cocos2d::Vec2 getPosition() const;
    // @brief 设置位置
    void setPosition(const cocos2d::Vec2& position);
    // @brief 获取是否翻开
    bool isFaceUp() const;
    // @brief 设置是否翻开
    void setFaceUp(bool isFaceUp);
    // @brief 获取是否可见 (用于处理被覆盖的牌)
    bool isVisible() const;
    // @brief 设置是否可见
    void setVisible(bool visible);

private:
    CardSuitType _suit;       // 花色
    CardFaceType _face;       // 点数
    cocos2d::Vec2 _position;  // 位置 (在主牌区)
    bool _isFaceUp;           // 是否翻开
    bool _isVisible;          // 是否可见 (例如，被上方的牌覆盖则不可见)
};

#endif // CardModel_h