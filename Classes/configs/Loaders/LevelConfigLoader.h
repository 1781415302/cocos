// test/Classes/configs/loaders/LevelConfigLoader.h
#ifndef LevelConfigLoader_h
#define LevelConfigLoader_h

#include "../LevelConfig.h"

// @brief 负责加载和解析关卡配置文件
class LevelConfigLoader
{
public:
    // @brief 从指定的关卡ID加载配置
    // @param levelId 关卡ID (例如 "level1")
    // @return 加载成功的LevelConfig对象，失败则返回空对象
    static LevelConfig loadLevelConfig(const std::string& levelId);

private:
    // @brief 解析ValueMap为CardPileConfig
    static CardPileConfig parseCardPile(const cocos2d::ValueVector& pileArray);
    // @brief 解析ValueMap为CardConfig
    static CardConfig parseCard(const cocos2d::ValueMap& cardMap);
};

#endif 
