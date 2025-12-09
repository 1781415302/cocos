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
    // 不再是单例，允许外部构造/注入
    SaveManager() = default;

    // ��ȡ saves Ŀ¼��Ĭ���ǿ�ִ���ļ�����Ŀ¼�µ� "saves/"��
    // ���������� setSavesDirectory() �򷵻ر����ǵ�·��������·��ӦΪ����·����ĩβ���л���б�ܣ���
    std::string getSavesDirectory() const;

    // �����Զ����浵Ŀ¼����������·�����������ַ����Իָ�Ĭ�ϣ���ִ�г���Ŀ¼�µ� saves��
    void setSavesDirectory(const std::string& path);

    // ȷ�� saves Ŀ¼���ڣ���Ҫʱ����������Ĭ��Ŀ¼����д�������˵� writablePath + "saves/"
    void ensureSavesDirectoryExists();

    // �г� saves Ŀ¼�µĴ浵�ļ�
    std::vector<std::string> listSaveFiles() const;

    // pending / active path ������UI ʹ�ã�
    void setPendingLoadPath(const std::string& path);
    std::string getPendingLoadPath() const;

    void setActiveSavePath(const std::string& path);
    std::string getActiveSavePath() const;

    // �����´浵�ļ����� levelId������Ŀ¼�д�����ʱ�����������ļ���
    std::string createNewSaveFileForLevel(const std::string& levelId) const;

    // ����/��ȡ GameModel + UndoModel ��/�� ָ���ļ���JSON��
    bool saveGameToFile(const std::string& filepath, const GameModel& gameModel, const class UndoModel& undoModel) const;
    bool loadGameFromFile(const std::string& filepath, GameModel& outGameModel, UndoModel& outUndoModel) const;

private:
    std::string _pendingPath;
    std::string _activePath;

    // �����ǿգ�����Ĭ�ϵ� saves Ŀ¼�������Ǿ���·����
    std::string _savesDirOverride;
};

#endif // SAVE_MANAGER_H