#include "SaveManager.h"
#include "cocos2d.h"
#include "models/GameModel.h"
#include "models/UndoModel.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <utils/json.hpp>

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <windows.h>
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
#include <mach-o/dyld.h>
#include <limits.h>
#include <stdlib.h>
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#else
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#endif

using namespace cocos2d;
using json = nlohmann::json;

SaveManager& SaveManager::getInstance()
{
    static SaveManager inst;
    return inst;
}

static std::string ensureTrailingSlash(const std::string& p) {
    if (p.empty()) return p;
    char last = p.back();
    if (last == '/' || last == '\\') return p;
    return p + "/";
}

static std::string getExecutableDirectory()
{
    std::string exePath;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
    char buf[MAX_PATH] = { 0 };
    DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        exePath.assign(buf, len);
    }
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
    char pathBuf[PATH_MAX];
    uint32_t size = PATH_MAX;
    if (_NSGetExecutablePath(pathBuf, &size) == 0) {
        char realBuf[PATH_MAX];
        if (realpath(pathBuf, realBuf)) {
            exePath = realBuf;
        }
        else {
            exePath = pathBuf;
        }
    }
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
    char pathBuf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", pathBuf, sizeof(pathBuf) - 1);
    if (len != -1) {
        pathBuf[len] = '\0';
        char realBuf[PATH_MAX];
        if (realpath(pathBuf, realBuf)) {
            exePath = realBuf;
        }
        else {
            exePath = pathBuf;
        }
    }
#else
    // 其他平台：回退到当前工作目录
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        exePath = cwd;
    }
#endif

    if (exePath.empty()) {
        // 作为最后的回退使用 writablePath（通常总是可写）
        exePath = FileUtils::getInstance()->getWritablePath();
        // remove trailing slash if present, will be normalized below
        if (!exePath.empty() && (exePath.back() == '/' || exePath.back() == '\\')) {
            exePath.pop_back();
        }
    }

    // 抽取目录部分
    size_t pos = exePath.find_last_of("/\\");
    if (pos != std::string::npos) {
        return exePath.substr(0, pos);
    }
    return exePath;
}

std::string SaveManager::getSavesDirectory() const
{
    if (!_savesDirOverride.empty()) {
        return ensureTrailingSlash(_savesDirOverride);
    }

    std::string exeDir = getExecutableDirectory();
    std::string dir = ensureTrailingSlash(exeDir) + "saves/";
    return dir;
}

void SaveManager::setSavesDirectory(const std::string& path)
{
    _savesDirOverride = path;
}

void SaveManager::ensureSavesDirectoryExists()
{
    std::string dir = getSavesDirectory();

    // 尝试创建目标目录
    FileUtils::getInstance()->createDirectory(dir);

    // 如果创建后仍然不存在，则说明目标目录可能只读（常见于移动平台的可执行目录）。
    if (!FileUtils::getInstance()->isDirectoryExist(dir)) {
        // 回退到 writablePath + "saves/"
        std::string fallback = FileUtils::getInstance()->getWritablePath();
        fallback = ensureTrailingSlash(fallback) + "saves/";
        FileUtils::getInstance()->createDirectory(fallback);

        // 记录并使用回退目录（设置 override 让后续调用使用回退）
        _savesDirOverride = fallback;
        CCLOG("SaveManager: executable-dir 'saves' not writable, falling back to: %s", _savesDirOverride.c_str());
    }
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
        // 过滤合理扩展名
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
    // 这里假设 ensureSavesDirectoryExists 已在调用点被调用；
    // 如果没有，请在调用前显式调用 ensureSavesDirectoryExists()
    std::string filename = timestampFilename(levelId);
    std::string full = getSavesDirectory() + filename;

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