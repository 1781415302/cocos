// test/Classes/utils/CardEnums.h
#ifndef CardEnums_h
#define CardEnums_h

#include <cocos2d.h>

// 花色类型
enum class CardSuitType : int
{
    CST_NONE = -1,
    CST_CLUBS,      // 梅花
    CST_DIAMONDS,   // 方块
    CST_HEARTS,     // 红桃
    CST_SPADES,     // 黑桃
    CST_NUM_CARD_SUIT_TYPES
};

// 正面类型 (点数)
enum class CardFaceType : int
{
    CFT_NONE = -1,
    CFT_ACE,    // 1
    CFT_TWO,    // 2
    CFT_THREE,  // 3
    CFT_FOUR,   // 4
    CFT_FIVE,   // 5
    CFT_SIX,    // 6
    CFT_SEVEN,  // 7
    CFT_EIGHT,  // 8
    CFT_NINE,   // 9
    CFT_TEN,    // 10
    CFT_JACK,   // 11
    CFT_QUEEN,  // 12
    CFT_KING,   // 13
    CFT_NUM_CARD_FACE_TYPES
};

#endif /* CardEnums_h */