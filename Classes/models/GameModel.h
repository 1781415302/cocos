// test/Classes/models/GameModel.h
#ifndef GameModel_h
#define GameModel_h

#include "cocos2d.h"
#include "CardModel.h"
#include <vector>

// @brief 存储整个游戏运行时状态的数据模型
class GameModel
{
public:
    // @brief 构造函数
    GameModel();

    // @brief 获取主牌区 (Playfield) 的牌列表 (可修改)
    std::vector<CardModel>& getPlayfieldCards();
    // @brief 获取主牌区 (Playfield) 的牌列表 (只读)
    const std::vector<CardModel>& getPlayfieldCards() const;
    // @brief 获取备用牌堆 (Stack) 的牌列表 (可修改)
    std::vector<CardModel>& getStackCards();
    // @brief 获取备用牌堆 (Stack) 的牌列表 (只读)
    const std::vector<CardModel>& getStackCards() const;

    // @brief 将主牌区的牌移动到备用牌堆顶部
    // @param playfieldIndex 主牌区中牌的索引
    // @return 操作是否成功
    bool movePlayfieldCardToStack(int playfieldIndex);

    // @brief 翻开备用牌堆顶部的牌
    // @return 操作是否成功
    bool flipTopStackCard();

    // @brief 检查主牌区是否有翻开的、可移动的牌
    bool hasMovablePlayfieldCard() const;

    // @brief 检查备用牌堆顶部的牌是否可以被匹配 (即是否有翻开的牌)
    bool canMatchWithTopStackCard() const;

private:
    std::vector<CardModel> _playfieldCards; // 主牌区的牌
    std::vector<CardModel> _stackCards;     // 备用牌堆的牌
};

#endif // GameModel_h#pragma once
