#pragma once
// Classes/HelloWorldScene.h
#ifndef __HELLO_WORLD_SCENE_H__
#define __HELLO_WORLD_SCENE_H__

#include "cocos2d.h"

namespace cocos2d { class Node; }

class GameController;

class HelloWorld : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();

    virtual bool init() override;
    CREATE_FUNC(HelloWorld);

    virtual void onExit() override;

private:
    GameController* _gameController = nullptr;
    cocos2d::Node* _gameParentNode = nullptr;
};

#endif // __HELLO_WORLD_SCENE_H__