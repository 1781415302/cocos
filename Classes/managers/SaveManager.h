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

    // 获取 saves 目录：默认是可执行文件所在目录下的 "saves/"。
    // 如果调用了 setSavesDirectory() 则返回被覆盖的路径（覆盖路径应为绝对路径，末尾可有或无斜杠）。
    std::string getSavesDirectory() const;

    // 设置自定义存档目录（传入绝对路径）；传空字符串以恢复默认（可执行程序目录下的 saves）
    void setSavesDirectory(const std::string& path);

    // 确保 saves 目录存在，必要时创建；如果默认目录不可写，会回退到 writablePath + "saves/"
    void ensureSavesDirectoryExists();

    // 列出 saves 目录下的存档文件
    std::vector<std::string> listSaveFiles() const;

    // pending / active path 管理（UI 使用）
    void setPendingLoadPath(const std::string& path);
    std::string getPendingLoadPath() const;

    void setActiveSavePath(const std::string& path);
    std::string getActiveSavePath() const;

    // 创建新存档文件用于 levelId（会在目录中创建以时间戳命名的文件）
    std::string createNewSaveFileForLevel(const std::string& levelId) const;

    // 保存/读取 GameModel + UndoModel 到/从 指定文件（JSON）
    bool saveGameToFile(const std::string& filepath, const GameModel& gameModel, const class UndoModel& undoModel) const;
    bool loadGameFromFile(const std::string& filepath, GameModel& outGameModel, UndoModel& outUndoModel) const;

private:
    SaveManager() = default;
    std::string _pendingPath;
    std::string _activePath;

    // 如果非空，覆盖默认的 saves 目录（必须是绝对路径）
    std::string _savesDirOverride;
};

#endif // SAVE_MANAGER_H