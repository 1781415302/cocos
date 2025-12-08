#include "SaveManager.h"
#include "cocos2d.h"
#include "models/GameModel.h"
#include "models/UndoModel.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <utils/json.hpp>

using namespace cocos2d;
using json = nlohmann::json;

SaveManager& SaveManager::getInstance()
{
    static SaveManager inst;
    return inst;
}

std::string SaveManager::getSavesDirectory() const
{
    std::string dir = FileUtils::getInstance()->getWritablePath() + "saves";
    return dir;
}

void SaveManager::ensureSavesDirectoryExists() const
{
    std::string dir = getSavesDirectory();
    FileUtils::getInstance()->createDirectory(dir);
}

std::vector<std::string> SaveManager::listSaveFiles() const
{
    std::vector<std::string> out;
    std::string dir = getSavesDirectory();

    std::vector<std::string> files;
    try {
        files = FileUtils::getInstance()->listFiles(dir);
    }
    catch (...) {
        files.clear();
    }

    for (const auto& f : files) {
        // filter for reasonable extensions
        if (f.size() > 5) {
            if (f.find(".json") != std::string::npos || f.find(".save") != std::string::npos) {
                out.push_back(f);
            }
        }
    }
    return out;
}

void SaveManager::setPendingLoadPath(const std::string& path)
{
    _pendingPath = path;
}

std::string SaveManager::getPendingLoadPath() const
{
    return _pendingPath;
}

void SaveManager::setActiveSavePath(const std::string& path)
{
    _activePath = path;
}

std::string SaveManager::getActiveSavePath() const
{
    return _activePath;
}

static std::string timestampFilename(const std::string& levelId)
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H-%M-%S");
    ss << "_level" << levelId << ".json";
    return ss.str();
}

std::string SaveManager::createNewSaveFileForLevel(const std::string& levelId) const
{
    ensureSavesDirectoryExists();
    std::string filename = timestampFilename(levelId);
    std::string full = getSavesDirectory() + "/" + filename;

    // create an empty JSON file to reserve the name (atomic create)
    json j;
    j["version"] = 1;
    j["levelId"] = levelId;
    // write initial empty structure
    std::string tmp = full + ".tmp";
    auto s = j.dump(2);
    FileUtils::getInstance()->writeStringToFile(s, tmp);
    // rename tmp -> full
    remove(full.c_str());
    rename(tmp.c_str(), full.c_str());

    return full;
}

bool SaveManager::saveGameToFile(const std::string& filepath, const GameModel& gameModel, const UndoModel& undoModel) const
{
    // Build JSON via GameModel / UndoModel -> json
    try {
        json j = gameModel.toJson();
        j["undo"] = undoModel.toJson();

        std::string tmp = filepath + ".tmp";
        std::string content = j.dump(2);

        // write tmp
        bool ok = FileUtils::getInstance()->writeStringToFile(content, tmp);
        if (!ok) return false;

        // atomically replace
        remove(filepath.c_str());
        if (rename(tmp.c_str(), filepath.c_str()) != 0) {
            // fallback: try to copy
            FileUtils::getInstance()->writeStringToFile(content, filepath);
        }
        return true;
    }
    catch (const std::exception& ex) {
        CCLOG("SaveManager::saveGameToFile exception: %s", ex.what());
        return false;
    }
}

bool SaveManager::loadGameFromFile(const std::string& filepath, GameModel& outGameModel, UndoModel& outUndoModel) const
{
    try {
        std::string content = FileUtils::getInstance()->getStringFromFile(filepath);
        auto j = json::parse(content);

        outGameModel = GameModel::fromJson(j);
        if (j.contains("undo")) {
            outUndoModel = UndoModel::fromJson(j["undo"]);
        }
        return true;
    }
    catch (const std::exception& ex) {
        CCLOG("SaveManager::loadGameFromFile exception: %s", ex.what());
        return false;
    }
}