#pragma once
// Classes/managers/GameManager.h
#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <memory>
#include <vector>
#include <optional>

#include "models/GameModel.h"
#include "models/UndoModel.h"

/**
 * GameManager
 * - 非单例，持有对 GameModel 的引用
 * - 负责原子性地修改 GameModel（draw/move/flip/add 等）
 * - 可选地将操作记录到 UndoModel（由 controller/外层传入）
 */
class GameManager {
public:
    // 构造：传入要管理的 model，undoModel 可选（用于记录操作）
    explicit GameManager(GameModel& model, UndoModel* undoModel = nullptr);

    // --- ID 管理 ---
    int allocateCardId();

    // --- add helpers ---
    void addPlayfieldCard(const std::shared_ptr<CardModel>& card);
    void addReserveCard(const std::shared_ptr<CardModel>& card);
    void addHandCard(const std::shared_ptr<CardModel>& card);

    // --- 游戏行为（这些从原 GameModel 中抽离出来） ---
    bool drawReserveToHand(); // 从 reserve 抽到 hand 并翻面（记录 action）
    bool movePlayfieldCardToHand(int playfieldIndex); // 从 playfield 指定下标移动到 hand
    bool moveTopHandCardToPlayfieldAt(int playfieldIndex, cocos2d::Vec2 position, CardStatus status, bool visible = true, bool faceUp = true);
    bool flipTopHandCard(); // 将 hand 顶部翻为 faceUp（记录 action）
    bool moveTopHandCardBackToReserve(cocos2d::Vec2 position, CardStatus status, bool visible, bool faceUp); // 将 hand 顶部返回 reserve

    // --- 查询/校验方法 ---
    bool hasMovablePlayfieldCard() const;
    bool canMatchWithHandTop() const;

    int findPlayfieldIndexById(int cardId) const;
    int findReserveIndexById(int cardId) const;
    int findHandIndexById(int cardId) const;

    // 直接访问 GameModel（只读或用于测试）
    GameModel& getModel() { return _model; }
    const GameModel& getModel() const { return _model; }

    // 设置/替换 UndoModel
    void setUndoModel(UndoModel* undoModel) { _undoModel = undoModel; }

private:
    GameModel& _model;
    UndoModel* _undoModel = nullptr;

    // 内部辅助：将对 single card 的快照生成 action（如果有 _undoModel）
    void recordAction(const UndoModel::Action& a);
};

#endif // GAMEMANAGER_H