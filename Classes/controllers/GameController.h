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
    // ... existing private methods unchanged ...
    void createViewsFromModel();

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

    std::unordered_map<int, CardView*> _cardViews;

    bool _busy = false;

    float _moveDuration = 0.28f;

    cocos2d::Vec2 _defaultHandPosition = cocos2d::Vec2(540.0f, 200.0f);

    UndoView* _undoView = nullptr;

    // SaveManager 持有方式：优先使用外部注入，否则内部自有
    SaveManager* _saveManager = nullptr;
    std::unique_ptr<SaveManager> _ownedSaveManager;
};