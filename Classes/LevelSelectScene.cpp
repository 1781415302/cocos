// Classes/LevelSelectScene.cpp
#include "LevelSelectScene.h"
#include "HelloWorldScene.h"
#include "ui/CocosGUI.h"

USING_NS_CC;

Scene* LevelSelectScene::createScene()
{
    return LevelSelectScene::create();
}

bool LevelSelectScene::init()
{
    if (!Scene::init()) return false;

    createBackground();
    createLevelButtons();

    return true;
}

void LevelSelectScene::createBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto bg = Sprite::create("level_background.png");
    if (bg)
    {
        bg->setAnchorPoint(Vec2(0.5f, 0.5f));
        bg->setPosition(origin + visibleSize * 0.5f);

        // ×ÔÊÊÓ¦Ëõ·Å
        auto bgSize = bg->getContentSize();
        if (bgSize.width > 0 && bgSize.height > 0)
        {
            float scaleX = visibleSize.width / bgSize.width;
            float scaleY = visibleSize.height / bgSize.height;
            bg->setScale(std::max(scaleX, scaleY));
        }

        addChild(bg, -1);
    }
}

void LevelSelectScene::createLevelButtons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    Vec2 center = origin + visibleSize * 0.5f;

    float spacing = 500.0f;

    struct BtnInfo { std::string img; std::string level; Vec2 offset; };
    std::vector<BtnInfo> btns = {
        { "level_2.png", "2", Vec2(spacing * 0.5f, 0.0f) },
        { "level_1.png", "1", Vec2(-spacing * 0.5f, 0.0f) },
    };

    for (const auto& info : btns)
    {
        auto btn = ui::Button::create(info.img, info.img);
        if (!btn) continue;

        btn->setScale9Enabled(false);
        btn->setAnchorPoint(Vec2(0.5f, 0.5f));
        btn->setPosition(center + info.offset);
        btn->setZoomScale(0.02f);

        btn->addClickEventListener([this, info, btn](Ref*) {
            runSelectAnimationAndEnter(info.level, btn);
            });

        addChild(btn, 1);
    }
}

void LevelSelectScene::runSelectAnimationAndEnter(const std::string& levelId, Node* targetButton)
{
    if (!targetButton) return;

    targetButton->stopAllActions();
    targetButton->runAction(Sequence::create(
        ScaleTo::create(0.08f, 0.92f),
        ScaleTo::create(0.08f, 1.05f),
        ScaleTo::create(0.06f, 1.0f),
        CallFunc::create([levelId]() {
            auto scene = HelloWorld::createSceneWithLevel(levelId);
            Director::getInstance()->replaceScene(TransitionFade::create(0.3f, scene));
            }),
        nullptr
    ));
}