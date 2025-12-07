#pragma once
// Classes/HelloWorldScene.h
#ifndef __HELLO_WORLD_SCENE_H__
#define __HELLO_WORLD_SCENE_H__

#include "cocos2d.h"
#include <string>

namespace cocos2d { class Node; }

class GameController;

class HelloWorld : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();
    // 根据关卡 ID 创建场景
    static cocos2d::Scene* createSceneWithLevel(const std::string& levelId);
    static HelloWorld* createWithLevel(const std::string& levelId);

    virtual bool init() override;
    // 可指定关卡 ID 的初始化
    bool initWithLevel(const std::string& levelId);
    CREATE_FUNC(HelloWorld);

    virtual void onExit() override;

private:
    GameController* _gameController = nullptr;
    cocos2d::Node* _gameParentNode = nullptr;
    std::string _levelId = "1";
};

#endif // __HELLO_WORLD_SCENE_H__