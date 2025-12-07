#pragma once
// Classes/LevelSelectScene.h
#ifndef LEVEL_SELECT_SCENE_H
#define LEVEL_SELECT_SCENE_H

#include "cocos2d.h"

class LevelSelectScene : public cocos2d::Scene
{
public:
    static cocos2d::Scene* createScene();

    virtual bool init() override;
    CREATE_FUNC(LevelSelectScene);

private:
    void createBackground();
    void createLevelButtons();
    void runSelectAnimationAndEnter(const std::string& levelId, cocos2d::Node* targetButton);
};

#endif // LEVEL_SELECT_SCENE_H