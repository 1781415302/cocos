#pragma once
// Classes/managers/UndoManager.h
#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H

#include <memory>

#include "models/GameModel.h"
#include "models/UndoModel.h"

/**
 * UndoManager
 * - 非单例，持有对 GameModel 和 UndoModel 的引用
 * - 负责将 UndoModel 中的 Action 应用到 GameModel（执行撤销）
 * - 不依赖 controller
 */
class UndoManager {
public:
    explicit UndoManager(GameModel& model, UndoModel& undoModel);

    // 尝试撤销最近一次 action（true = 成功并已从 undo stack 中移除）
    bool undoLast();

    // 对外工具
    void clear() { _undoModel.clear(); }
    size_t size() const { return _undoModel.size(); }

    UndoModel& getUndoModel() { return _undoModel; }

private:
    GameModel& _model;
    UndoModel& _undoModel;

    // 应用单个 action（返回是否成功）
    bool applyAction(const UndoModel::Action& action);
};

#endif // UNDOMANAGER_H