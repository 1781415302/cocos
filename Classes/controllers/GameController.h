#pragma once
#ifndef GameController_h
#define GameController_h

#include "cocos2d.h"
#include "models/GameModel.h"
#include "models/UndoModel.h"
#include "views/CardView.h"
#include "configs/LevelConfig.h"
#include "views/UndoView.h" // <- 确保包含 UndoView 的头
#include <unordered_map>
#include <memory>
#include <string>

/**
 * GameController (with Undo)
 *
 * Responsibilities:
 * - Load level config, build view + model
 * - Manage playfield / reserve / hand
 */
class GameController
{
public:
    explicit GameController(cocos2d::Node* parentNode);
    ~GameController();

    // Start a specific level
    void startGame(const std::string& levelId);

    // Click handler for playfield card
    void handlePlayfieldCardClick(int cardId);

    // Click handler for reserve stack
    void handleReserveClick();

    // Undo last action
    void handleUndo();

    // Cleanup controller, remove views and reset model
    void reset();

private:
    // Build views from model
    void createViewsFromModel();

    // Index helpers
    int findPlayfieldIndexByCardId(int cardId) const;
    int findReserveIndexByCardId(int cardId) const;
    int findHandIndexByCardId(int cardId) const;

    // Adjacent face rule (±1)
    bool facesAreAdjacent(CardFaceType a, CardFaceType b) const;

    // Animations
    void animatePlayfieldCardToHand(int playfieldIndex, int cardId);
    void animateReserveTopToHand();

    // Recompute playfield coverage and sync face-up state
    void updatePlayfieldCoverage(float overlapAreaThreshold = 0.0f);

    // Auto draw the first reserve card to hand (animate=false for initial)
    void drawInitialReserveTopToHand(bool animate = false);

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

    // Undo button view (指针类型正确声明)
    UndoView* _undoView = nullptr;
};

#endif // GameController_h