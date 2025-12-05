#pragma once
// test/Classes/services/GameModelFromLevelGenerator.h
#ifndef GameModelFromLevelGenerator_h
#define GameModelFromLevelGenerator_h

#include "models/GameModel.h"
#include "configs/LevelConfig.h"

// @brief 将静态的LevelConfig转换为动态的GameModel
class GameModelFromLevelGenerator
{
public:
    // @brief 根据LevelConfig生成GameModel
    // @param config 静态关卡配置
    // @return 生成的GameModel对象
    static GameModel generateGameModel(const LevelConfig& config);
};

#endif // GameModelFromLevelGenerator_h