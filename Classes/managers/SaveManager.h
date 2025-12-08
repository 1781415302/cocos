#pragma once
#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#include <string>
#include <vector>

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

private:
    SaveManager() = default;
    std::string _pendingPath;
};

#endif // SAVE_MANAGER_H