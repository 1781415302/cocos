#include "LevelConfigLoader.h"
#include "cocos2d.h"
#include <cmath>
#include <limits>
#include <sstream>

// rapidjson 头文件（cocos2d-x 自带）
#include "json/document.h"
#include "json/error/en.h"

using namespace cocos2d;

// 前置声明（递归转换相关）
static Value rapidjsonValueToCocosValue(const rapidjson::Value& rv);
static ValueMap rapidjsonObjectToValueMap(const rapidjson::Value& obj);
static ValueVector rapidjsonArrayToValueVector(const rapidjson::Value& arr);

// 将 rapidjson 对象转换为 cocos2d::ValueMap（递归）
static ValueMap rapidjsonObjectToValueMap(const rapidjson::Value& obj) {
    ValueMap map;
    for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
        std::string key = it->name.GetString();
        map.emplace(key, rapidjsonValueToCocosValue(it->value));
    }
    return map;
}

// 将 rapidjson 数组转换为 cocos2d::ValueVector（递归）
static ValueVector rapidjsonArrayToValueVector(const rapidjson::Value& arr) {
    ValueVector vec;
    for (rapidjson::SizeType i = 0; i < arr.Size(); ++i) {
        vec.emplace_back(rapidjsonValueToCocosValue(arr[i]));
    }
    return vec;
}

// 将单个 rapidjson::Value 转为 cocos2d::Value（保留整数/浮点/字符串/布尔/空/对象/数组）
static Value rapidjsonValueToCocosValue(const rapidjson::Value& rv) {
    if (rv.IsObject()) {
        return Value(rapidjsonObjectToValueMap(rv));
    }
    else if (rv.IsArray()) {
        return Value(rapidjsonArrayToValueVector(rv));
    }
    else if (rv.IsString()) {
        return Value(std::string(rv.GetString()));
    }
    else if (rv.IsBool()) {
        return Value(rv.GetBool());
    }
    else if (rv.IsInt()) {
        return Value(rv.GetInt());
    }
    else if (rv.IsUint()) {
        unsigned int u = rv.GetUint();
        if (u <= static_cast<unsigned int>(std::numeric_limits<int>::max())) {
            return Value(static_cast<int>(u));
        }
        else {
            // 超出 int 范围则转为字符串以避免截断
            return Value(std::to_string(u));
        }
    }
    else if (rv.IsInt64()) {
        int64_t v = rv.GetInt64();
        if (v <= static_cast<int64_t>(std::numeric_limits<int>::max()) &&
            v >= static_cast<int64_t>(std::numeric_limits<int>::min())) {
            return Value(static_cast<int>(v));
        }
        else {
            return Value(std::to_string(v));
        }
    }
    else if (rv.IsUint64()) {
        uint64_t v = rv.GetUint64();
        if (v <= static_cast<uint64_t>(std::numeric_limits<int>::max())) {
            return Value(static_cast<int>(v));
        }
        else {
            return Value(std::to_string(v));
        }
    }
    else if (rv.IsDouble()) {
        double d = rv.GetDouble();
        double intpart;
        // 如果是整数且在 int 范围内，则保存为 int，否则保存为 float（cocos2d::Value 使用 float 存储浮点）
        if (std::modf(d, &intpart) == 0.0 &&
            intpart <= static_cast<double>(std::numeric_limits<int>::max()) &&
            intpart >= static_cast<double>(std::numeric_limits<int>::min())) {
            return Value(static_cast<int>(intpart));
        }
        return Value(static_cast<float>(d));
    }
    else if (rv.IsNull()) {
        return Value(); // 默认构造的 Value 表示 null/空
    }

    // 兜底（理论上不会到这里）
    if (rv.IsString()) {
        return Value(std::string(rv.GetString()));
    }
    return Value();
}

// 直接使用 rapidjson 解析并构建 LevelConfig，不输出诊断日志
LevelConfig LevelConfigLoader::loadLevelConfig(const std::string& levelId)
{
    LevelConfig config;
    std::string filePath = "levels/" + levelId + ".json";

    auto fileUtils = FileUtils::getInstance();

    // 如果文件不存在或读取为空，则返回空的 config
    if (!fileUtils->isFileExist(filePath)) {
        return config;
    }

    std::string content = fileUtils->getStringFromFile(filePath);
    if (content.empty()) {
        return config;
    }

    // rapidjson 解析
    rapidjson::Document d;
    d.Parse(content.c_str());
    if (d.HasParseError()) {
        // 解析失败则返回默认 config（不打印诊断信息）
        return config;
    }

    // 将 Document 转换为 ValueMap；如果根是数组则按兼容性放到 Playfield
    ValueMap root;
    if (d.IsObject()) {
        Value vroot = rapidjsonValueToCocosValue(d);
        if (vroot.getType() == Value::Type::MAP) {
            root = vroot.asValueMap();
        }
        else {
            return config;
        }
    }
    else if (d.IsArray()) {
        ValueVector vv = rapidjsonArrayToValueVector(d);
        root["Playfield"] = Value(vv);
    }
    else {
        return config;
    }

    if (root.empty()) {
        return config;
    }

    // 解析 Playfield（存在且为数组时）
    auto it = root.find("Playfield");
    if (it != root.end() && it->second.getType() == Value::Type::VECTOR) {
        config.setPlayfieldConfig(parseCardPile(it->second.asValueVector()));
    }

    // 解析 Stack（存在且为数组时）
    it = root.find("Stack");
    if (it != root.end() && it->second.getType() == Value::Type::VECTOR) {
        config.setStackConfig(parseCardPile(it->second.asValueVector()));
    }

    return config;
}

// 将 ValueVector 转为 CardPileConfig（保持原有逻辑）
CardPileConfig LevelConfigLoader::parseCardPile(const cocos2d::ValueVector& pileArray)
{
    CardPileConfig pileConfig;
    for (const auto& cardValue : pileArray) {
        if (cardValue.getType() == cocos2d::Value::Type::MAP) {
            CardConfig card = parseCard(cardValue.asValueMap());
            pileConfig.cards.push_back(card);
        }
        else {
            // 跳过非对象项
        }
    }
    return pileConfig;
}

// 将单张卡的 ValueMap 转为 CardConfig（保持原有逻辑）
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

    // CardFace
    it = cardMap.find("CardFace");
    if (it != cardMap.end() && it->second.getType() == cocos2d::Value::Type::INTEGER) {
        card.face = static_cast<CardFaceType>(it->second.asInt());
    }

    // Position { x, y }
    it = cardMap.find("Position");
    if (it != cardMap.end() && it->second.getType() == cocos2d::Value::Type::MAP) {
        cocos2d::ValueMap posMap = it->second.asValueMap();
        auto pit = posMap.find("x");
        if (pit != posMap.end() && (pit->second.getType() == cocos2d::Value::Type::FLOAT || pit->second.getType() == cocos2d::Value::Type::INTEGER)) {
            card.position.x = pit->second.asFloat();
        }
        pit = posMap.find("y");
        if (pit != posMap.end() && (pit->second.getType() == cocos2d::Value::Type::FLOAT || pit->second.getType() == cocos2d::Value::Type::INTEGER)) {
            card.position.y = pit->second.asFloat();
        }
    }

    return card;
}