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
 * - 负责把 level 配置创建成 view + model
 * - Playfield / Reserve / Hand 的管理
 */
class GameController
{
public:
    explicit GameController(cocos2d::Node* parentNode);
    ~GameController();

    // 开始指定关卡
    void startGame(const std::string& levelId);

    // CardView 在 playfield 的点击处理
    void handlePlayfieldCardClick(int cardId);

    // Reserve 区点击（抽牌）
    void handleReserveClick();

    // 撤销（未实现）
    void handleUndo();

    // 重置 controller（删除 view、清 model）
    void reset();

private:
    // 把 model -> view
    void createViewsFromModel();

    // 更新 playfield/hand/reserve index 查找
    int findPlayfieldIndexByCardId(int cardId) const;
    int findReserveIndexByCardId(int cardId) const;
    int findHandIndexByCardId(int cardId) const;

    // 点数相邻的判断（用于匹配）
    bool facesAreAdjacent(CardFaceType a, CardFaceType b) const;

    // 动画移动 helpers
    void animatePlayfieldCardToHand(int playfieldIndex, int cardId);
    void animateReserveTopToHand();

    // 新增：覆盖检测并同步 model/view
    //  - 按 playfield 配置顺序（数组顺序）判断：若存在任意 index > i 的牌与 i 相交，
    //    则 i 被覆盖（covered）。否则 i 翻开（exposed）。
    //  - overlapAreaThreshold: 可选阈值（以像素面积计），用于忽略极小重叠
    void updatePlayfieldCoverage(float overlapAreaThreshold = 0.0f);

    // 新增：在开局时自动从 reserve 翻一张牌到 hand（animate=false 表示无动画、立即生效）
    void drawInitialReserveTopToHand(bool animate = false);

private:
    cocos2d::Node* _parentNode = nullptr;

    cocos2d::Node* _playfieldNode = nullptr;
    cocos2d::Node* _reserveNode = nullptr;
    cocos2d::Node* _handNode = nullptr;

    GameModel _gameModel;

    std::unordered_map<int, CardView*> _cardViews;

    bool _busy = false;

    float _moveDuration = 0.28f;

    cocos2d::Vec2 _defaultHandPosition = cocos2d::Vec2(540.0f, 200.0f);
};

#endif // GameController_h