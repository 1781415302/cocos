#pragma once
// Classes/services/UndoService.h
#ifndef UndoService_h
#define UndoService_h

#include "models/GameModel.h"
#include "models/UndoModel.h"

/**
 * @brief 撤销业务逻辑的集中处理（不管理数据生命周期）
 */
class UndoService
{
public:
    // 按 ActionType 对 GameModel 执行撤销操作，返回是否成功
    static bool applyAction(GameModel& model, const UndoModel::Action& action);
};

#endif // UndoService_h