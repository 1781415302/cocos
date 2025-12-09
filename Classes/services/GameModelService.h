#pragma once
// Services: 处理与 GameModel 相关的业务逻辑，不持有数据

#include "cocos2d.h"
#include "models/GameModel.h"

class GameModelService
{
public:
    // Reserve -> Hand
    static bool drawReserveToHand(GameModel& model);

    // Playfield -> Hand
    static bool movePlayfieldCardToHand(GameModel& model, int playfieldIndex);

    // Hand -> Playfield（保持原有三个重载）
    static bool moveTopHandCardToPlayfieldAt(GameModel& model, int playfieldIndex, cocos2d::Vec2 position, CardStatus status);
    static bool moveTopHandCardToPlayfieldAt(GameModel& model, int playfieldIndex, cocos2d::Vec2 position, CardStatus status, bool visible);
    static bool moveTopHandCardToPlayfieldAt(GameModel& model, int playfieldIndex, cocos2d::Vec2 position, CardStatus status, bool visible, bool faceUp);

    // Hand 翻面
    static bool flipTopHandCard(GameModel& model);

    // Hand -> Reserve
    static bool moveTopHandCardBackToReserve(GameModel& model, cocos2d::Vec2 position, CardStatus status, bool visible);
    static bool moveTopHandCardBackToReserve(GameModel& model, cocos2d::Vec2 position, CardStatus status, bool visible, bool faceUp);

    // 查询
    static bool hasMovablePlayfieldCard(const GameModel& model);
    static bool canMatchWithHandTop(const GameModel& model);
};