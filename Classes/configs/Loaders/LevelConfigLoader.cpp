#include "LevelConfigLoader.h"
#include "cocos2d.h"
#include <iomanip>
#include <sstream>
#include <cmath> // 为了 std::modf

// rapidjson headers (cocos2d-x 自带 rapidjson)
#include "json/document.h"
#include "json/error/en.h"

using namespace cocos2d;
// 注意：不使用 `using namespace rapidjson;`，改为显式使用 rapidjson:: 前缀

static Value rapidjsonValueToCocosValue(const rapidjson::Value& rv); // forward

static ValueMap rapidjsonObjectToValueMap(const rapidjson::Value& obj) {
    ValueMap map;
    for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
        std::string key = it->name.GetString();
        map.emplace(key, rapidjsonValueToCocosValue(it->value));
    }
    return map;
}

static ValueVector rapidjsonArrayToValueVector(const rapidjson::Value& arr) {
    ValueVector vec;
    for (rapidjson::SizeType i = 0; i < arr.Size(); ++i) {
        vec.emplace_back(rapidjsonValueToCocosValue(arr[i]));
    }
    return vec;
}

static Value rapidjsonValueToCocosValue(const rapidjson::Value& rv) {
    // Convert rapidjson::Value -> cocos2d::Value
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
        // cocos2d::Value doesn't have unsigned int variant; store as int (beware overflow)
        return Value(static_cast<int>(rv.GetUint()));
    }
    else if (rv.IsInt64()) {
        return Value(static_cast<int>(rv.GetInt64()));
    }
    else if (rv.IsUint64()) {
        return Value(static_cast<int>(rv.GetUint64()));
    }
    else if (rv.IsDouble()) {
        double d = rv.GetDouble();
        double intpart;
        if (std::modf(d, &intpart) == 0.0 && intpart <= static_cast<double>(INT_MAX) && intpart >= static_cast<double>(INT_MIN)) {
            return Value(static_cast<int>(intpart));
        }
        return Value(static_cast<float>(d));
    }
    else if (rv.IsNull()) {
        return Value(); // null/default
    }
    // fallback: stringify
    if (rv.IsString()) {
        return Value(std::string(rv.GetString()));
    }
    return Value();
}

LevelConfig LevelConfigLoader::loadLevelConfig(const std::string& levelId)
{
    LevelConfig config;
    std::string filePath = "levels/" + levelId + ".json"; // resources/levels/<levelId>.json

    auto fileUtils = FileUtils::getInstance();

    // 1) 打印文件存在性与 full path，用于诊断
    bool exists = fileUtils->isFileExist(filePath);
    CCLOG("LevelConfigLoader::loadLevelConfig - isFileExist('%s') = %d", filePath.c_str(), exists ? 1 : 0);
    std::string fullPath = fileUtils->fullPathForFilename(filePath);
    CCLOG("LevelConfigLoader::loadLevelConfig - fullPathForFilename('%s') = '%s'", filePath.c_str(), fullPath.c_str());

    if (!exists) {
        CCLOG("LevelConfigLoader::loadLevelConfig - file not found: %s", filePath.c_str());
        return config;
    }

    // 2) 读取原始文本并打印前 200 字符和前几个字节的十六进制（用于发现 BOM 或奇怪字符）
    std::string content = fileUtils->getStringFromFile(filePath);
    CCLOG("LevelConfigLoader::loadLevelConfig - file size = %zu bytes", content.size());
    if (!content.empty()) {
        CCLOG("LevelConfigLoader::loadLevelConfig - preview = %.200s", content.c_str());

        // 打印前 16 字节的十六进制，方便看 BOM（EF BB BF）或其他不可见字符
        std::ostringstream oss;
        size_t n = std::min<size_t>(16, content.size());
        for (size_t i = 0; i < n; ++i) {
            oss << std::hex << std::setfill('0') << std::setw(2) << (static_cast<unsigned int>(static_cast<unsigned char>(content[i]))) << " ";
        }
        CCLOG("LevelConfigLoader::loadLevelConfig - first %zu bytes hex: %s", n, oss.str().c_str());
    }
    else {
        CCLOG("LevelConfigLoader::loadLevelConfig - file content is empty: %s", filePath.c_str());
        return config;
    }

    // 3) 先尝试使用 cocos2d 的 ValueMap 解析（原有方式）
    ValueMap root = fileUtils->getValueMapFromFile(filePath);
    if (root.empty()) {
        CCLOG("LevelConfigLoader::loadLevelConfig - parsed ValueMap is empty or parse failed: %s", filePath.c_str());

        // 3a) 再尝试 ValueVector，看是否文件顶层是数组（防止格式误差）
        ValueVector vv = fileUtils->getValueVectorFromFile(filePath);
        if (!vv.empty()) {
            CCLOG("LevelConfigLoader::loadLevelConfig - file parsed as ValueVector (size=%zu). Possibly JSON top-level is an array.", vv.size());
            // 如果你的 JSON 真的是数组形式，这里可以调整后续解析逻辑
        }
        else {
            CCLOG("LevelConfigLoader::loadLevelConfig - ValueVector also empty. Will try rapidjson parse to get detailed error info.");

            // 3b) 用 rapidjson 直接解析字符串，并在成功时把 Document 转换为 cocos2d::ValueMap
            rapidjson::Document d;
            d.Parse(content.c_str());
            if (d.HasParseError()) {
                auto err = d.GetParseError();
                size_t offset = static_cast<size_t>(d.GetErrorOffset());
                CCLOG("LevelConfigLoader::loadLevelConfig - rapidjson parse error: %s at offset %zu", rapidjson::GetParseError_En(err), offset);
                size_t start = (offset > 40) ? (offset - 40) : 0;
                size_t end = std::min(content.size(), offset + 40);
                std::string contextSnippet = content.substr(start, end - start);
                CCLOG("LevelConfigLoader::loadLevelConfig - context around error (offset %zu): %.200s", offset, contextSnippet.c_str());
            }
            else {
                CCLOG("LevelConfigLoader::loadLevelConfig - rapidjson parse succeeded (but ValueMapFromFile earlier failed). Document type = %d", d.GetType());

                if (d.IsObject()) {
                    try {
                        Value vroot = rapidjsonValueToCocosValue(d);
                        if (vroot.getType() == Value::Type::MAP) {
                            root = vroot.asValueMap();
                            CCLOG("LevelConfigLoader::loadLevelConfig - converted rapidjson Document to ValueMap successfully.");
                        }
                        else {
                            CCLOG("LevelConfigLoader::loadLevelConfig - rapidjson root is object but conversion produced non-map Value (type=%d)", vroot.getType());
                        }
                    }
                    catch (const std::exception& ex) {
                        CCLOG("LevelConfigLoader::loadLevelConfig - exception during rapidjson->Value conversion: %s", ex.what());
                    }
                }
                else {
                    CCLOG("LevelConfigLoader::loadLevelConfig - rapidjson root is not an object (type=%d).", d.GetType());
                }
            }
        }
    }

    if (root.empty()) {
        CCLOG("LevelConfigLoader::loadLevelConfig - final root is empty, aborting parse for: %s", filePath.c_str());
        return config;
    }

    // 解析 Playfield（如果存在且为数组）
    auto it = root.find("Playfield");
    if (it != root.end() && it->second.getType() == Value::Type::VECTOR) {
        config.setPlayfieldConfig(parseCardPile(it->second.asValueVector()));
    }
    else {
        CCLOG("LevelConfigLoader::loadLevelConfig - Playfield missing or not an array in %s", filePath.c_str());
    }

    // 解析 Stack（如果存在且为数组）
    it = root.find("Stack");
    if (it != root.end() && it->second.getType() == Value::Type::VECTOR) {
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