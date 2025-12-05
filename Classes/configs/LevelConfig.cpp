// test/Classes/configs/LevelConfig.cpp
#include "LevelConfig.h"

const CardPileConfig& LevelConfig::getPlayfieldConfig() const
{
    return _playfieldConfig;
}

const CardPileConfig& LevelConfig::getStackConfig() const
{
    return _stackConfig;
}

void LevelConfig::setPlayfieldConfig(const CardPileConfig& config)
{
    _playfieldConfig = config;
}

void LevelConfig::setStackConfig(const CardPileConfig& config)
{
    _stackConfig = config;
}