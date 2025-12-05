#pragma once
// Classes/controllers/GameController.h
#ifndef GameController_h
#define GameController_h

#include "cocos2d.h"
#include "models/GameModel.h"
#include "views/CardView.h"
#include "configs/LevelConfig.h"
#include <unordered_map>
#include <memory>
#include <string>

/**
 * GameController (without Undo)
 *
 * 说明：
 * - 该类负责初始化关卡、创建视图、处理三大区域的交互：
 *     - Playfield (主牌区)
 *     - Reserve  (备用牌堆，可点击抽牌)
 *     - Hand     (底牌堆，用于匹配)
 * - 动画期间会将 _busy 设为 true，避免重复交互。
 */
    class GameController
{
public:
    explicit GameController(cocos2d::Node* parentNode);
    ~GameController();

    // 启动关卡（resources/levels/<levelId>.json）
    void startGame(const std::string& levelId);

    // CardView 的 playfield 点击回调
    void handlePlayfieldCardClick(int cardId);

    // Reserve 区点击（通常是点备用牌堆的顶部卡）
    void handleReserveClick();

    // 占位：撤销（后续实现）
    void handleUndo();

    // 清理/重置
    void reset();

private:
    // 根据 model 创建三个区域的 CardView
    void createViewsFromModel();

    // 查找 playfield/hand/reserve 中卡牌 index
    int findPlayfieldIndexByCardId(int cardId) const;
    int findReserveIndexByCardId(int cardId) const;
    int findHandIndexByCardId(int cardId) const;

    // 判定两张牌点数是否相邻（只看点数）
    bool facesAreAdjacent(CardFaceType a, CardFaceType b) const;

    // 执行动画并在回调中更新模型
    void animatePlayfieldCardToHand(int playfieldIndex, int cardId);
    void animateReserveTopToHand(); // 点击备用牌堆时调用

private:
    cocos2d::Node* _parentNode = nullptr;

    // 三个子区域节点，便于管理 ZOrder 与布局
    cocos2d::Node* _playfieldNode = nullptr;
    cocos2d::Node* _reserveNode = nullptr;
    cocos2d::Node* _handNode = nullptr;

    GameModel _gameModel;

    // cardId -> CardView* 映射（CardView 由 Cocos 引用计数管理）
    std::unordered_map<int, CardView*> _cardViews;

    // 动画中锁，避免并发操作
    bool _busy = false;

    // 动画时间
    float _moveDuration = 0.28f;

    // 当 hand 为空时的默认位置（可从 LevelConfig 中读取）
    cocos2d::Vec2 _defaultHandPosition = cocos2d::Vec2(540.0f, 200.0f);
};

#endif // GameController_h
