#pragma once
#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#include <string>
#include <vector>

class GameModel;
class UndoModel;

class SaveManager
{
public:
    static SaveManager& getInstance();

    // 获取 saves 目录路径（writablePath + "saves/"）
    std::string getSavesDirectory() const;

    // 确保 saves 目录存在（若无则创建）
    void ensureSavesDirectoryExists() const;

    // 列出 saves 目录下文件（返回完整路径列表）
    std::vector<std::string> listSaveFiles() const;

    // 设置/获取 pending 加载路径（UI 打开文件后写入）
    void setPendingLoadPath(const std::string& path);
    std::string getPendingLoadPath() const;

    // Active save path (当前游戏会话正在使用的存档文件)
    void setActiveSavePath(const std::string& path);
    std::string getActiveSavePath() const;

    // 创建新的存档（按时间命名），并返回新文件路径（不会写入初始内容）
    std::string createNewSaveFileForLevel(const std::string& levelId) const;

    // 保存/加载 GameModel + UndoModel 到/从指定文件（JSON）
    bool saveGameToFile(const std::string& filepath, const GameModel& gameModel, const class UndoModel& undoModel) const;
    bool loadGameFromFile(const std::string& filepath, GameModel& outGameModel, class UndoModel& outUndoModel) const;

private:
    SaveManager() = default;
    std::string _pendingPath;
    std::string _activePath;
};

#endif // SAVE_MANAGER_H