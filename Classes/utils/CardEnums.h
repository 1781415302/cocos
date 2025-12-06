// test/Classes/utils/CardEnums.h
#ifndef CardEnums_h
#define CardEnums_h

#include <cocos2d.h>

// 花色类型
enum class CardSuitType : int
{
    CST_NONE = -1,
    CST_CLUBS,      // 梅花=0
    CST_DIAMONDS,   // 方块=1
    CST_HEARTS,     // 红桃=2
    CST_SPADES,     // 黑桃=3
    CST_NUM_CARD_SUIT_TYPES
};

// 正面类型 (点数)
enum class CardFaceType : int
{
    CFT_NONE = -1,
	CFT_ACE,    // A=0
    CFT_TWO,    // 2=1
    CFT_THREE,  // 3=2
    CFT_FOUR,   // 4=3
    CFT_FIVE,   // 5=4
    CFT_SIX,    // 6=5
	CFT_SEVEN,  // 7=6
    CFT_EIGHT,  // 8=7
    CFT_NINE,   // 9=8
    CFT_TEN,    // 10=9
	CFT_JACK,   // J=10
    CFT_QUEEN,  // Q=11
    CFT_KING,   // K=12
    CFT_NUM_CARD_FACE_TYPES
};

#endif /* CardEnums_h */