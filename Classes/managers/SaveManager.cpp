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
    // ����ƽ̨�����˵���ǰ����Ŀ¼
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        exePath = cwd;
    }
#endif

    if (exePath.empty()) {
        // ��Ϊ�����Ļ���ʹ�� writablePath��ͨ�����ǿ�д��
        exePath = FileUtils::getInstance()->getWritablePath();
        if (!exePath.empty() && (exePath.back() == '/' || exePath.back() == '\\')) {
            exePath.pop_back();
        }
    }

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

    FileUtils::getInstance()->createDirectory(dir);

    if (!FileUtils::getInstance()->isDirectoryExist(dir)) {
        std::string fallback = FileUtils::getInstance()->getWritablePath();
        fallback = ensureTrailingSlash(fallback) + "saves/";
        FileUtils::getInstance()->createDirectory(fallback);

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
    std::string filename = timestampFilename(levelId);
    std::string full = getSavesDirectory() + filename;

    json j;
    j["version"] = 1;
    j["levelId"] = levelId;
    std::string tmp = full + ".tmp";
    auto s = j.dump(2);
    FileUtils::getInstance()->writeStringToFile(s, tmp);
    remove(full.c_str());
    rename(tmp.c_str(), full.c_str());

    return full;
}

bool SaveManager::saveGameToFile(const std::string& filepath, const GameModel& gameModel, const UndoModel& undoModel) const
{
    try {
        json j = gameModel.toJson();
        j["undo"] = undoModel.toJson();

        std::string tmp = filepath + ".tmp";
        std::string content = j.dump(2);

        bool ok = FileUtils::getInstance()->writeStringToFile(content, tmp);
        if (!ok) return false;

        remove(filepath.c_str());
        if (rename(tmp.c_str(), filepath.c_str()) != 0) {
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