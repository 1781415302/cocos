// Classes/managers/UndoManager.cpp
#include "UndoManager.h"
#include "models/CardModel.h"

using namespace cocos2d;

UndoManager::UndoManager(GameModel& model, UndoModel& undoModel)
    : _model(model), _undoModel(undoModel)
{
}

bool UndoManager::undoLast()
{
    UndoModel::Action action;
    if (!_undoModel.pop(action)) return false;

    bool ok = applyAction(action);
    if (!ok) {
        // 如果应用失败，将 action 重新推入 stack，保持一致性
        _undoModel.push(action);
        return false;
    }
    return true;
}

bool UndoManager::applyAction(const UndoModel::Action& action)
{
    switch (action.type) {
    case UndoModel::ActionType::DrawReserveToHand:
    {
        // 撤销：将 hand 顶部移动回 reserve（并恢复属性）
        auto& hand = _model.getHandCards();
        if (hand.empty()) return false;
        auto cardPtr = hand.back();
        hand.pop_back();

        cardPtr->setPosition(action.prevPosition);
        cardPtr->setStatus(action.prevStatus);
        cardPtr->setVisible(action.prevVisible);
        cardPtr->setFaceUp(action.prevFaceUp);

        _model.getReserveCards().push_back(cardPtr);
        return true;
    }
    case UndoModel::ActionType::MovePlayfieldToHand:
    {
        // 撤销：将 hand 顶部移回 playfield 指定索引，并恢复属性
        auto& hand = _model.getHandCards();
        if (hand.empty()) return false;
        auto cardPtr = hand.back();
        hand.pop_back();

        cardPtr->setPosition(action.prevPosition);
        cardPtr->setStatus(action.prevStatus);
        cardPtr->setVisible(action.prevVisible);
        cardPtr->setFaceUp(action.prevFaceUp);

        auto& pf = _model.getPlayfieldCards();
        int idx = action.playfieldIndex;
        if (idx < 0 || idx > static_cast<int>(pf.size())) {
            pf.push_back(cardPtr);
        }
        else {
            pf.insert(pf.begin() + idx, cardPtr);
        }
        return true;
    }
    case UndoModel::ActionType::MoveHandToPlayfield:
    {
        // 撤销：将 playfield 指定索引上的卡移回 hand，并恢复属性
        int idx = action.playfieldIndex;
        auto& pf = _model.getPlayfieldCards();
        if (idx < 0 || idx >= static_cast<int>(pf.size())) return false;
        auto cardPtr = pf[idx];
        pf.erase(pf.begin() + idx);

        // 放回 hand，并恢复 prev 状态到手中（前一次位置是 prevPosition）
        cardPtr->setPosition(action.prevPosition);
        cardPtr->setStatus(action.prevStatus);
        cardPtr->setVisible(action.prevVisible);
        cardPtr->setFaceUp(action.prevFaceUp);

        _model.getHandCards().push_back(cardPtr);
        return true;
    }
    case UndoModel::ActionType::MoveHandToReserve:
    {
        // 撤销：将 hand 顶部移回 reserve（与 DrawReserveToHand 相似）
        auto& hand = _model.getHandCards();
        if (hand.empty()) return false;
        auto cardPtr = hand.back();
        hand.pop_back();

        cardPtr->setPosition(action.prevPosition);
        cardPtr->setStatus(action.prevStatus);
        cardPtr->setVisible(action.prevVisible);
        cardPtr->setFaceUp(action.prevFaceUp);

        _model.getReserveCards().push_back(cardPtr);
        return true;
    }
    case UndoModel::ActionType::FlipHandTopFaceUp:
    {
        auto& hand = _model.getHandCards();
        if (hand.empty()) return false;
        hand.back()->setFaceUp(action.prevFaceUp);
        return true;
    }
    default:
        return false;
    }
}