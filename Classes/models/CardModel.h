#pragma once
// Classes/models/CardModel.h
#ifndef CardModel_h
#define CardModel_h

#include "cocos2d.h" // Include cocos2d headers if using Vec2, etc.
#include "utils/CardEnums.h" // Include CardEnums header
#include <memory>
#include <utils/json.hpp>

using json = nlohmann::json;

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
    CardModel(int id, CardFaceType faceType, CardSuitType suitType, cocos2d::Vec2 position, CardStatus status = CardStatus::COVERED);
    CardModel(CardSuitType suitType, CardFaceType faceType, cocos2d::Vec2 position, bool isFaceUp = false);

    // getters / setters...
    int getId() const;
    void setId(int id);

    CardFaceType getCardFace() const;
    CardSuitType getCardSuit() const;
    cocos2d::Vec2 getPosition() const;
    CardStatus getStatus() const;

    bool isFaceUp() const;
    void setFaceUp(bool faceUp);

    bool isVisible() const;
    void setVisible(bool visible);

    void setPosition(cocos2d::Vec2 pos);
    void setStatus(CardStatus status);

    // 序列化 / 反序列化
    json toJson() const;
    static CardModel fromJson(const json& j);

private:
    int _id = -1;
    CardFaceType _cardFace;
    CardSuitType _cardSuit;
    cocos2d::Vec2 _position;
    CardStatus _status;
    bool _visible = true;
};

#endif /* CardModel_h */