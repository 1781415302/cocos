#pragma once

#include "cocos2d.h"
#include "models/GameModel.h"
#include "models/UndoModel.h"
#include "views/CardView.h"
#include "configs/LevelConfig.h"
#include "views/UndoView.h"
#include "managers/SaveManager.h"
#include <unordered_map>
#include <memory>
#include <string>

/**
 * GameController
 * - 协调 model 与 view
 * - 处理用户操作的业务逻辑
 * - 持有 managers（如 SaveManager）作为成员或通过注入使用
 *
 * 说明：保持对外 API 向后兼容（构造函数参数不变）。
 */
    class GameController
{
public:
    // 可选注入外部 SaveManager；若传空则内部创建自有实例
    explicit GameController(cocos2d::Node* parentNode, SaveManager* saveManager = nullptr);
    ~GameController();

    // Start a specific level; optional savePath to load from
    void startGame(const std::string& levelId, const std::string& optionalSavePath = "");

    // Load a previously saved file into current controller (override current model)
    bool loadFromSave(const std::string& savePath);

    // Click handlers...
    void handlePlayfieldCardClick(int cardId);
    void handleReserveClick();
    void handleUndo();
    void reset();

    // expose save trigger
    void saveToActive() const;

private:
    // view creation helpers
    void createViewsFromModel();
    void createPlayfieldViews();
    void createReserveViews();
    void createHandViews();

    // single-card view creation helpers (reduce duplication)
    CardView* createCardViewForPlayfield(const std::shared_ptr<CardModel>& cardPtr);
    CardView* createCardViewForReserve(const std::shared_ptr<CardModel>& cardPtr);
    CardView* createCardViewForHand(const std::shared_ptr<CardModel>& cardPtr);

    // small helpers
    void ensureUndoView();
    void saveActiveIfSet() const;

    int findPlayfieldIndexByCardId(int cardId) const;
    int findReserveIndexByCardId(int cardId) const;
    int findHandIndexByCardId(int cardId) const;
    bool facesAreAdjacent(CardFaceType a, CardFaceType b) const;
    void animatePlayfieldCardToHand(int playfieldIndex, int cardId);
    void animateReserveTopToHand();
    void updatePlayfieldCoverage(float overlapAreaThreshold = 0.0f);
    void drawInitialReserveTopToHand(bool animate = false);
    void repositionUndoToRightOfHand(float spacing = 16.0f);
    void reparentView(CardView* v, cocos2d::Node* newParent);

private:
    cocos2d::Node* _parentNode = nullptr;
    cocos2d::Node* _playfieldNode = nullptr;
    cocos2d::Node* _reserveNode = nullptr;
    cocos2d::Node* _handNode = nullptr;

    GameModel _gameModel;
    UndoModel _undoModel;

    // map cardId -> CardView* (cocos 的节点由 engine 管理)
    std::unordered_map<int, CardView*> _cardViews;

    bool _busy = false;

    float _moveDuration = 0.28f;

    cocos2d::Vec2 _defaultHandPosition = cocos2d::Vec2(540.0f, 200.0f);

    UndoView* _undoView = nullptr;

    // SaveManager 持有方式：优先使用外部注入，否则内部自有
    SaveManager* _saveManager = nullptr;
    std::unique_ptr<SaveManager> _ownedSaveManager;
};