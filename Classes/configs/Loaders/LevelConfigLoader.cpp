#include "LevelConfigLoader.h"
#include "cocos2d.h"

LevelConfig LevelConfigLoader::loadLevelConfig(const std::string& levelId)
{
    LevelConfig config;
    std::string filePath = "levels/" + levelId + ".json"; // resources/levels/<levelId>.json

    auto fileUtils = cocos2d::FileUtils::getInstance();
    if (!fileUtils->isFileExist(filePath)) {
        CCLOG("LevelConfigLoader::loadLevelConfig - file not found: %s", filePath.c_str());
        return config;
    }

    // 使用 cocos2d 的 ValueMap 来解析 JSON 文件
    cocos2d::ValueMap root = fileUtils->getValueMapFromFile(filePath);
    if (root.empty()) {
        CCLOG("LevelConfigLoader::loadLevelConfig - parsed ValueMap is empty or parse failed: %s", filePath.c_str());
        return config;
    }

    // 解析 Playfield（如果存在且为数组）
    auto it = root.find("Playfield");
    if (it != root.end() && it->second.getType() == cocos2d::Value::Type::VECTOR) {
        config.setPlayfieldConfig(parseCardPile(it->second.asValueVector()));
    }
    else {
        CCLOG("LevelConfigLoader::loadLevelConfig - Playfield missing or not an array in %s", filePath.c_str());
    }

    // 解析 Stack（如果存在且为数组）
    it = root.find("Stack");
    if (it != root.end() && it->second.getType() == cocos2d::Value::Type::VECTOR) {
        config.setStackConfig(parseCardPile(it->second.asValueVector()));
    }
    else {
        CCLOG("LevelConfigLoader::loadLevelConfig - Stack missing or not an array in %s", filePath.c_str());
    }

    return config;
}

CardPileConfig LevelConfigLoader::parseCardPile(const cocos2d::ValueVector& pileArray)
{
    CardPileConfig pileConfig;
    for (const auto& cardValue : pileArray) {
        if (cardValue.getType() == cocos2d::Value::Type::MAP) {
            CardConfig card = parseCard(cardValue.asValueMap());
            pileConfig.cards.push_back(card);
        }
        else {
            CCLOG("LevelConfigLoader::parseCardPile - skipping non-object entry in pile array");
        }
    }
    return pileConfig;
}

CardConfig LevelConfigLoader::parseCard(const cocos2d::ValueMap& cardMap)
{
    CardConfig card;
    // 默认值
    card.suit = CardSuitType::CST_NONE;
    card.face = CardFaceType::CFT_NONE;
    card.position = cocos2d::Vec2::ZERO;

    // CardSuit
    auto it = cardMap.find("CardSuit");
    if (it != cardMap.end() && it->second.getType() == cocos2d::Value::Type::INTEGER) {
        card.suit = static_cast<CardSuitType>(it->second.asInt());
    }
    else {
        CCLOG("LevelConfigLoader::parseCard - CardSuit missing or wrong type; using CST_NONE");
    }

    // CardFace
    it = cardMap.find("CardFace");
    if (it != cardMap.end() && it->second.getType() == cocos2d::Value::Type::INTEGER) {
        card.face = static_cast<CardFaceType>(it->second.asInt());
    }
    else {
        CCLOG("LevelConfigLoader::parseCard - CardFace missing or wrong type; using CFT_NONE");
    }

    // Position { x, y }
    it = cardMap.find("Position");
    if (it != cardMap.end() && it->second.getType() == cocos2d::Value::Type::MAP) {
        cocos2d::ValueMap posMap = it->second.asValueMap();
        auto pit = posMap.find("x");
        if (pit != posMap.end() && (pit->second.getType() == cocos2d::Value::Type::FLOAT || pit->second.getType() == cocos2d::Value::Type::INTEGER)) {
            card.position.x = pit->second.asFloat();
        }
        else {
            CCLOG("LevelConfigLoader::parseCard - Position.x missing or wrong type; using 0");
        }
        pit = posMap.find("y");
        if (pit != posMap.end() && (pit->second.getType() == cocos2d::Value::Type::FLOAT || pit->second.getType() == cocos2d::Value::Type::INTEGER)) {
            card.position.y = pit->second.asFloat();
        }
        else {
            CCLOG("LevelConfigLoader::parseCard - Position.y missing or wrong type; using 0");
        }
    } // 否则保持默认 Vec2::ZERO

    return card;
}