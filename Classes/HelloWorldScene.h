#pragma once
// Classes/HelloWorldScene.h
#ifndef __HELLO_WORLD_SCENE_H__
#define __HELLO_WORLD_SCENE_H__

#include "cocos2d.h"
#include <string>
#include <memory>

namespace cocos2d { class Node; }

class GameController;
class SaveManager;

class HelloWorld : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    // 传入关卡 ID 和共享的 SaveManager
    static cocos2d::Scene* createSceneWithLevel(const std::string& levelId, std::shared_ptr<SaveManager> saveManager = nullptr);
    static HelloWorld* createWithLevel(const std::string& levelId, std::shared_ptr<SaveManager> saveManager = nullptr);

    virtual bool init() override;
    bool initWithLevel(const std::string& levelId, std::shared_ptr<SaveManager> saveManager);
    CREATE_FUNC(HelloWorld);

    virtual void onExit() override;

private:
    GameController* _gameController = nullptr;
    cocos2d::Node* _gameParentNode = nullptr;
    std::string _levelId = "1";
    std::shared_ptr<SaveManager> _saveManager;
};

#endif // __HELLO_WORLD_SCENE_H__