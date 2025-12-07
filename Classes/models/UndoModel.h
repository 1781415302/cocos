#pragma once
// Classes/models/UndoModel.h
#ifndef UndoModel_h
#define UndoModel_h

#include "cocos2d.h"
#include "CardModel.h"
#include <vector>
#include <optional>
#include <memory>

class GameModel;

/**
 * @brief 记录并回滚模型层操作的 Undo 数据结构（不涉及控制器动画）
 *
 * 只做数据回滚：控制器可在外部完成动画，再调用 applyLast 进行模型状态回滚。
 */
class UndoModel
{
public:
    enum class ActionType
    {
        DrawReserveToHand,       // reserve -> hand
        MovePlayfieldToHand,     // playfield[i] -> hand
        MoveHandToPlayfield,     // hand top -> playfield[i]
        MoveHandToReserve,       // hand top -> reserve
        FlipHandTopFaceUp        // hand top 翻面
    };

    struct Action
    {
        ActionType type;

        int cardId = -1;               // 参与的牌 id（用于调试/校验）
        int playfieldIndex = -1;       // 目标或源的 playfield 索引
        cocos2d::Vec2 prevPosition;    // 回滚时要恢复的 position
        CardStatus prevStatus = CardStatus::COVERED;
        bool prevVisible = true;
        bool prevFaceUp = true;

        // 仅在需要时使用的后置状态（例如翻面后回滚需要知道当前面朝）
        bool faceUpAfter = true;
    };

    void push(const Action& action);
    bool pop(Action& outAction);
    void clear();
    size_t size() const;
    bool applyLast(GameModel& model);

    // 构造便捷函数
    static Action makeDrawReserveToHand(const CardModel& cardBefore);
    static Action makeMovePlayfieldToHand(const CardModel& cardBefore, int playfieldIndex);
    static Action makeMoveHandToPlayfield(const CardModel& cardBefore, int playfieldIndex);
    static Action makeMoveHandToReserve(const CardModel& cardBefore);
    static Action makeFlipHandTop(const CardModel& cardBefore);

private:
    std::vector<Action> _stack;
};

#endif // UndoModel_h