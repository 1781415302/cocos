#pragma once
#ifndef UndoView_h
#define UndoView_h

#include "cocos2d.h"
#include <functional>

class UndoView : public cocos2d::Node
{
public:
    static UndoView* create(const std::string& imageFile);
    bool init(const std::string& imageFile);
    void setClickCallback(std::function<void()> callback);
    cocos2d::Size getButtonSize() const;

private:
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);
    virtual void onExit() override;

private:
    cocos2d::Sprite* _sprite = nullptr;
    std::function<void()> _clickCallback;
};

#endif // UndoView_h