#include "UndoModel.h"
#include "GameModel.h"
#include "services/GameModelService.h"
#include <utils/json.hpp>

using json = nlohmann::json;
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
        ok = GameModelService::moveTopHandCardBackToReserve(model, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
        break;
    case ActionType::MovePlayfieldToHand:
        ok = GameModelService::moveTopHandCardToPlayfieldAt(model, action.playfieldIndex, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
        break;
    case ActionType::MoveHandToPlayfield:
        ok = GameModelService::movePlayfieldCardToHand(model, action.playfieldIndex);
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
        ok = GameModelService::moveTopHandCardBackToReserve(model, action.prevPosition, action.prevStatus, action.prevVisible, action.prevFaceUp);
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

// Serialization for Action (unchanged)
json UndoModel::Action::toJson() const
{
    json j;
    j["type"] = static_cast<int>(type);
    j["cardId"] = cardId;
    j["playfieldIndex"] = playfieldIndex;
    j["prevPosition"] = { {"x", prevPosition.x}, {"y", prevPosition.y} };
    j["prevStatus"] = static_cast<int>(prevStatus);
    j["prevVisible"] = prevVisible;
    j["prevFaceUp"] = prevFaceUp;
    j["faceUpAfter"] = faceUpAfter;
    return j;
}

UndoModel::Action UndoModel::Action::fromJson(const json& j)
{
    Action a;
    a.type = static_cast<ActionType>(j.value("type", 0));
    a.cardId = j.value("cardId", -1);
    a.playfieldIndex = j.value("playfieldIndex", -1);
    if (j.contains("prevPosition")) {
        a.prevPosition.x = j["prevPosition"].value("x", 0.0f);
        a.prevPosition.y = j["prevPosition"].value("y", 0.0f);
    }
    a.prevStatus = static_cast<CardStatus>(j.value("prevStatus", static_cast<int>(CardStatus::COVERED)));
    a.prevVisible = j.value("prevVisible", true);
    a.prevFaceUp = j.value("prevFaceUp", true);
    a.faceUpAfter = j.value("faceUpAfter", true);
    return a;
}

// Action factory functions (unchanged)
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
    a.prevFaceUp = cardBefore.isFaceUp();   // previous state
    a.faceUpAfter = true;
    return a;
}

// Serialize entire UndoModel
json UndoModel::toJson() const
{
    json arr = json::array();
    for (const auto& a : _stack) {
        arr.push_back(a.toJson());
    }
    return arr;
}

UndoModel UndoModel::fromJson(const json& j)
{
    UndoModel um;
    if (!j.is_array()) return um;
    for (const auto& el : j) {
        um._stack.push_back(Action::fromJson(el));
    }
    return um;
}