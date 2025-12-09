// Classes/LevelSelectScene.cpp
#include "LevelSelectScene.h"
#include "HelloWorldScene.h"
#include "ui/CocosGUI.h"
#include "managers/SaveManager.h" // SaveManager 已非单例
#include <memory>

USING_NS_CC;

Scene* LevelSelectScene::createScene()
{
    return LevelSelectScene::create();
}

bool LevelSelectScene::init()
{
    if (!Scene::init()) return false;

    // 创建 SaveManager 实例（共享给后续场景）
    _saveManager = std::make_shared<SaveManager>();

    createBackground();
    createLevelButtons();

    // 载入存档按钮
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    auto loadBtn = ui::Button::create();
    loadBtn->setTitleText(u8"读取存档");
    loadBtn->setTitleFontName("fonts/FLjiangdouti-Regular-2.ttf");
    loadBtn->setTitleFontSize(35);
    loadBtn->setAnchorPoint(Vec2(0.0f, 1.0f));
    loadBtn->setPosition(origin + Vec2(10.0f, visibleSize.height - 10.0f));
    loadBtn->addClickEventListener([this](Ref*) {
        this->showSaveBrowser();
        });
    this->addChild(loadBtn, 1000);

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
        CallFunc::create([levelId, sm = _saveManager]() {
            // 进入游戏场景，传递 SaveManager 共享实例
            Director::getInstance()->replaceScene(
                TransitionFade::create(0.3f, HelloWorld::createSceneWithLevel(levelId, sm)));
            }),
        nullptr
    ));
}

void LevelSelectScene::showSaveBrowser()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto overlay = LayerColor::create(Color4B(0, 0, 0, 160));
    overlay->setPosition(origin);
    overlay->setName("save_browser_overlay");
    this->addChild(overlay, 2000);

    auto panel = LayerColor::create(Color4B(36, 36, 36, 230), 720, 600);
    panel->setAnchorPoint(Vec2(0.5f, 0.5f));
    panel->setPosition(origin + visibleSize * 0.25f);
    panel->setName("save_browser_panel");
    overlay->addChild(panel);

    {
        auto draw = DrawNode::create();
        Vec2 rect[4];
        rect[0] = Vec2(0, 0);
        rect[1] = Vec2(panel->getContentSize().width, 0);
        rect[2] = Vec2(panel->getContentSize().width, panel->getContentSize().height);
        rect[3] = Vec2(0, panel->getContentSize().height);
        draw->drawPolygon(rect, 4, Color4F(0, 0, 0, 0), 1.0f, Color4F(1.0f, 1.0f, 1.0f, 0.08f));
        draw->setPosition(Vec2::ZERO);
        panel->addChild(draw, 1);
    }

    auto title = Label::createWithTTF(u8"选择存档文件", "fonts/FLjiangdouti-Regular-2.ttf", 28);
    title->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 36));
    panel->addChild(title, 2);

    auto listView = ui::ListView::create();
    listView->setContentSize(Size(panel->getContentSize().width - 40, panel->getContentSize().height - 140));
    listView->setAnchorPoint(Vec2(0.5f, 1.0f));
    listView->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 64));
    listView->setItemsMargin(8.0f);
    listView->setScrollBarEnabled(true);
    panel->addChild(listView, 2);

    _saveManager->ensureSavesDirectoryExists();
    std::vector<std::string> files = _saveManager->listSaveFiles();
    if (files.empty()) {
        auto noLabel = Label::createWithSystemFont(u8"没有找到存档文件", "Arial", 22);
        noLabel->setPosition(listView->getContentSize() * 0.5f);
        listView->addChild(noLabel);
    }
    else {
        for (const auto& fullPath : files) {
            std::string filename = fullPath;
            auto pos = fullPath.find_last_of("/\\");
            if (pos != std::string::npos) filename = fullPath.substr(pos + 1);

            auto item = ui::Button::create();
            item->setContentSize(Size(listView->getContentSize().width, 48));
            item->setTitleText(filename);
            item->setTitleFontSize(20);
            item->setZoomScale(0.02f);
            item->setName(fullPath);

            item->addClickEventListener([this, fullPath, overlay](Ref*) {
                CCLOG("Open save (item click): %s", fullPath.c_str());
                _saveManager->setPendingLoadPath(fullPath);
                overlay->removeFromParent();
                auto scene = HelloWorld::createSceneWithLevel("1", _saveManager);
                Director::getInstance()->replaceScene(TransitionFade::create(0.3f, scene));
                });

            listView->pushBackCustomItem(item);
        }
    }

    auto cancelBtn = ui::Button::create();
    cancelBtn->setTitleText(u8"取消");
    cancelBtn->setTitleFontSize(22);
    cancelBtn->setPosition(Vec2(panel->getContentSize().width * 0.5f, 40));
    panel->addChild(cancelBtn, 2);

    cancelBtn->addClickEventListener([overlay](Ref*) {
        overlay->removeFromParent();
        });

    auto swallowListener = EventListenerTouchOneByOne::create();
    swallowListener->setSwallowTouches(true);
    swallowListener->onTouchBegan = [panel](Touch* touch, Event* event) -> bool {
        Vec2 touchInPanel = panel->convertToNodeSpace(touch->getLocation());
        Rect panelRect(0, 0, panel->getContentSize().width, panel->getContentSize().height);
        if (panelRect.containsPoint(touchInPanel)) {
            return false;
        }
        return true;
        };
    overlay->getEventDispatcher()->addEventListenerWithSceneGraphPriority(swallowListener, overlay);
}