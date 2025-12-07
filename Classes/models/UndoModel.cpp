#include "UndoModel.h"
#include "GameModel.h"

using namespace cocos2d;

void UndoModel::push(const Action& action)
{
    _stack.push_back(action);
}

bool UndoModel::pop(Action& outAction)
{
    if (_stack.empty()) return false;
    outAction = _stack.back();
    _stack.pop_back();
    return true;
}

void UndoModel::clear()
{
    _stack.clear();
}

size_t UndoModel::size() const
{
    return _stack.size();
}

bool UndoModel::applyLast(GameModel& model)
{
    if (_stack.empty()) return false;

    Action action = _stack.back();

    bool ok = false;
    switch (action.type)
    {
    case ActionType::DrawReserveToHand:
        ok = model.moveTopHandCardBackToReserve(action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
        break;
    case ActionType::MovePlayfieldToHand:
        ok = model.moveTopHandCardToPlayfieldAt(action.playfieldIndex, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
        break;
    case ActionType::MoveHandToPlayfield:
        ok = model.movePlayfieldCardToHand(action.playfieldIndex);
        if (ok) {
            auto& hand = model.getHandCards();
            if (!hand.empty()) {
                hand.back()->setPosition(action.prevPosition);
                hand.back()->setStatus(action.prevStatus);
                hand.back()->setVisible(action.prevVisible);
                hand.back()->setFaceUp(action.prevFaceUp);
            }
        }
        break;
    case ActionType::MoveHandToReserve:
        ok = model.moveTopHandCardBackToReserve(action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
        break;
    case ActionType::FlipHandTopFaceUp:
    {
        auto& hand = model.getHandCards();
        if (!hand.empty()) {
            hand.back()->setFaceUp(action.prevFaceUp);
            ok = true;
        }
        break;
    }
    default:
        ok = false;
    }

    if (ok) {
        _stack.pop_back();
    }
    return ok;
}

// -------- 便捷构造器 --------

UndoModel::Action UndoModel::makeDrawReserveToHand(const CardModel& cardBefore)
{
    Action a;
    a.type = ActionType::DrawReserveToHand;
    a.cardId = cardBefore.getId();
    a.prevPosition = cardBefore.getPosition();
    a.prevStatus = cardBefore.getStatus();
    a.prevVisible = cardBefore.isVisible();
    a.prevFaceUp = cardBefore.isFaceUp();
    return a;
}

UndoModel::Action UndoModel::makeMovePlayfieldToHand(const CardModel& cardBefore, int playfieldIndex)
{
    Action a;
    a.type = ActionType::MovePlayfieldToHand;
    a.cardId = cardBefore.getId();
    a.playfieldIndex = playfieldIndex;
    a.prevPosition = cardBefore.getPosition();
    a.prevStatus = cardBefore.getStatus();
    a.prevVisible = cardBefore.isVisible();
    a.prevFaceUp = cardBefore.isFaceUp();
    return a;
}

UndoModel::Action UndoModel::makeMoveHandToPlayfield(const CardModel& cardBefore, int playfieldIndex)
{
    Action a;
    a.type = ActionType::MoveHandToPlayfield;
    a.cardId = cardBefore.getId();
    a.playfieldIndex = playfieldIndex;
    a.prevPosition = cardBefore.getPosition();
    a.prevStatus = cardBefore.getStatus();
    a.prevVisible = cardBefore.isVisible();
    a.prevFaceUp = cardBefore.isFaceUp();
    return a;
}

UndoModel::Action UndoModel::makeMoveHandToReserve(const CardModel& cardBefore)
{
    Action a;
    a.type = ActionType::MoveHandToReserve;
    a.cardId = cardBefore.getId();
    a.prevPosition = cardBefore.getPosition();
    a.prevStatus = cardBefore.getStatus();
    a.prevVisible = cardBefore.isVisible();
    a.prevFaceUp = cardBefore.isFaceUp();
    return a;
}

UndoModel::Action UndoModel::makeFlipHandTop(const CardModel& cardBefore)
{
    Action a;
    a.type = ActionType::FlipHandTopFaceUp;
    a.cardId = cardBefore.getId();
    a.prevPosition = cardBefore.getPosition();
    a.prevStatus = cardBefore.getStatus();
    a.prevVisible = cardBefore.isVisible();
    a.prevFaceUp = cardBefore.isFaceUp();   // 翻面前的状态
    a.faceUpAfter = true;
    return a;
}