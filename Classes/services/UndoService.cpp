#include "services/UndoService.h"
#include "services/GameModelService.h"

bool UndoService::applyAction(GameModel& model, const UndoModel::Action& action)
{
    switch (action.type)
    {
    case UndoModel::ActionType::DrawReserveToHand:
        return GameModelService::moveTopHandCardBackToReserve(
            model, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);

    case UndoModel::ActionType::MovePlayfieldToHand:
        return GameModelService::moveTopHandCardToPlayfieldAt(
            model, action.playfieldIndex, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);

    case UndoModel::ActionType::MoveHandToPlayfield:
        // 将牌从牌面移回手牌（与 GameModelService::movePlayfieldCardToHand 相同语义）
        return GameModelService::movePlayfieldCardToHand(model, action.playfieldIndex);

    case UndoModel::ActionType::MoveHandToReserve:
        // 撤销 hand->reserve，相当于 draw reserve -> hand 的反向
        return GameModelService::drawReserveToHand(model);

    case UndoModel::ActionType::FlipHandTopFaceUp:
    {
        auto& hand = model.getHandCards();
        if (hand.empty()) return false;
        hand.back()->setFaceUp(action.prevFaceUp);
        return true;
    }
    default:
        return false;
    }
}