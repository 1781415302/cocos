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

    // helpers
    static Action makeDrawReserveToHand(const CardModel& cardBefore);
    static Action makeMovePlayfieldToHand(const CardModel& cardBefore, int playfieldIndex);
    static Action makeMoveHandToPlayfield(const CardModel& cardBefore, int playfieldIndex);
    static Action makeMoveHandToReserve(const CardModel& cardBefore);
    static Action makeFlipHandTop(const CardModel& cardBefore);

private:
    std::vector<Action> _stack;
};

#endif // UndoModel_h