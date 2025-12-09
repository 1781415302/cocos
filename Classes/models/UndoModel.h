#pragma once
// Classes/models/UndoModel.h
#ifndef UndoModel_h
#define UndoModel_h

#include "cocos2d.h"
#include "CardModel.h"
#include <vector>
#include <optional>
#include <memory>
#include <utils/json.hpp>

using json = nlohmann::json;

class GameModel;

/**
 * UndoModel 仅负责保存 action stack（用于序列化/恢复）。
 * 不包含“如何将 action 应用到 GameModel”这样的业务逻辑。
 * 具体的 apply 操作应由 UndoManager（managers 层）或 Controller 执行。
 */
class UndoModel
{
public:
    enum class ActionType
    {
        DrawReserveToHand,
        MovePlayfieldToHand,
        MoveHandToPlayfield,
        MoveHandToReserve,
        FlipHandTopFaceUp
    };

    struct Action
    {
        ActionType type;

        int cardId = -1;
        int playfieldIndex = -1;
        cocos2d::Vec2 prevPosition;
        CardStatus prevStatus = CardStatus::COVERED;
        bool prevVisible = true;
        bool prevFaceUp = true;
        bool faceUpAfter = true;

        json toJson() const;
        static Action fromJson(const json& j);
    };

    void push(const Action& action);
    bool pop(Action& outAction);
    void clear();
    size_t size() const;

    // Serialization
    json toJson() const;
    static UndoModel fromJson(const json& j);

    // helpers: 工厂函数仅用于构造 action（仅捕获数据快照）
    static Action makeDrawReserveToHand(const CardModel& cardBefore);
    static Action makeMovePlayfieldToHand(const CardModel& cardBefore, int playfieldIndex);
    static Action makeMoveHandToPlayfield(const CardModel& cardBefore, int playfieldIndex);
    static Action makeMoveHandToReserve(const CardModel& cardBefore);
    static Action makeFlipHandTop(const CardModel& cardBefore);

private:
    std::vector<Action> _stack;
};

#endif // UndoModel_h