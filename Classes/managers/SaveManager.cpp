#include "SaveManager.h"
#include "cocos2d.h"

using namespace cocos2d;

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
    // FileUtils::isDirectoryExist may not exist on older versions; try createDirectory which is safe
    FileUtils::getInstance()->createDirectory(dir);
}

std::vector<std::string> SaveManager::listSaveFiles() const
{
    std::vector<std::string> out;
    std::string dir = getSavesDirectory();

    // Try to use FileUtils::listFiles (available in many cocos2d-x versions)
    std::vector<std::string> files;
    try {
        files = FileUtils::getInstance()->listFiles(dir);
    }
    catch (...) {
        files.clear();
    }

    for (const auto& f : files) {
        // Only include regular files (skip directories) and simple filter by extension
        if (f.size() > 5) {
            // naive filter for ".json" or ".save"
            if (f.find(".json") != std::string::npos || f.find(".save") != std::string::npos) {
                out.push_back(f);
            }
        }
    }

    // If listFiles returned empty, attempt to probe common save file naming
    if (out.empty()) {
        // Try scanning using getValueMapFromFile? (skip for now)
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