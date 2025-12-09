// Classes/LevelSelectScene.cpp
#include "LevelSelectScene.h"
#include "HelloWorldScene.h"
#include "ui/CocosGUI.h"
#include "managers/SaveManager.h" // 草稿 SaveManager 单例（见下方新增文件）

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

    // 新增：在关卡选择页添加“读取存档”按钮（草稿）
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

        // 适配屏幕
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
            // NOTE (草稿)：如果 SaveManager 中有 pending save path，则在进入场景后
            // HelloWorld / GameController 需要从该路径加载存档（后续实现）。
            Director::getInstance()->replaceScene(TransitionFade::create(0.3f, HelloWorld::createSceneWithLevel(levelId)));
            }),
        nullptr
    ));
}

// 新增：展示游戏内存档浏览器（草稿实现）
// 点击列表项会直接打开该存档（立刻进入关卡并设置 SaveManager pending 路径）
void LevelSelectScene::showSaveBrowser()
{
    // 弹出半透明遮罩
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto overlay = LayerColor::create(Color4B(0, 0, 0, 160));
    overlay->setPosition(origin);
    overlay->setName("save_browser_overlay");
    this->addChild(overlay, 2000);

    // 中央面板：使用半透明深色背景以减少视觉干扰
    auto panel = LayerColor::create(Color4B(36, 36, 36, 230), 720, 600);
    panel->setAnchorPoint(Vec2(0.5f, 0.5f));
    panel->setPosition(origin + visibleSize * 0.25f);
    panel->setName("save_browser_panel");
    overlay->addChild(panel);

    // 可选的边框线（白色半透明）
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

    // 标题
    auto title = Label::createWithTTF(u8"选择存档文件", "fonts/FLjiangdouti-Regular-2.ttf", 28);
    title->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 36));
    panel->addChild(title, 2);

    // 列表（使用 ui::ListView）
    auto listView = ui::ListView::create();
    listView->setContentSize(Size(panel->getContentSize().width - 40, panel->getContentSize().height - 140));
    listView->setAnchorPoint(Vec2(0.5f, 1.0f));
    listView->setPosition(Vec2(panel->getContentSize().width * 0.5f, panel->getContentSize().height - 64));
    listView->setItemsMargin(8.0f);
    listView->setScrollBarEnabled(true);
    panel->addChild(listView, 2);

    // 获取 saves 目录下文件
    SaveManager::getInstance().ensureSavesDirectoryExists();
    std::vector<std::string> files = SaveManager::getInstance().listSaveFiles();
    if (files.empty()) {
        auto noLabel = Label::createWithSystemFont(u8"没有发现存档文件", "Arial", 22);
        noLabel->setPosition(listView->getContentSize() * 0.5f);
        listView->addChild(noLabel);
    }
    else {
        // 每个文件做成一个 button item；点击 item 就直接打开存档
        for (const auto& fullPath : files) {
            std::string filename = fullPath;
            // 尝试只保留文件名（去掉目录）
            auto pos = fullPath.find_last_of("/\\");
            if (pos != std::string::npos) filename = fullPath.substr(pos + 1);

            auto item = ui::Button::create();
            item->setContentSize(Size(listView->getContentSize().width, 48));
            item->setTitleText(filename);
            item->setTitleFontSize(20);
            item->setZoomScale(0.02f);
            item->setUserData(nullptr); // placeholder
            // 存储完整路径到 button 的名字字段以便回调使用
            item->setName(fullPath);

            // 关键：直接为 item 添加点击事件（按值捕获 fullPath 和 overlay）
            item->addClickEventListener([this, fullPath, overlay](Ref*) {
                CCLOG("Open save (item click): %s", fullPath.c_str());
                SaveManager::getInstance().setPendingLoadPath(fullPath);
                // 立即关闭面板并进入默认关卡（草稿：将来应解析 levelId 并进入对应关卡）
                overlay->removeFromParent();
                auto scene = HelloWorld::createSceneWithLevel("1");
                Director::getInstance()->replaceScene(TransitionFade::create(0.3f, scene));
                });

            listView->pushBackCustomItem(item);
        }
    }

    // 底部保留一个取消按钮以便关闭面板
    auto cancelBtn = ui::Button::create();
    cancelBtn->setTitleText(u8"取消");
    cancelBtn->setTitleFontSize(22);
    cancelBtn->setPosition(Vec2(panel->getContentSize().width * 0.5f, 40));
    panel->addChild(cancelBtn, 2);

    cancelBtn->addClickEventListener([overlay](Ref*) {
        overlay->removeFromParent();
        });

    // 屏蔽底层点击，但允许 panel 内部控件接收事件：
    auto swallowListener = EventListenerTouchOneByOne::create();
    swallowListener->setSwallowTouches(true);
    swallowListener->onTouchBegan = [panel](Touch* touch, Event* event) -> bool {
        Vec2 touchInPanel = panel->convertToNodeSpace(touch->getLocation());
        Rect panelRect(0, 0, panel->getContentSize().width, panel->getContentSize().height);
        if (panelRect.containsPoint(touchInPanel)) {
            // 在 panel 内，返回 false，事件继续传递到 panel 子节点（例如 listView item）
            return false;
        }
        // 在 panel 外，吞掉触摸，阻止底下的场景接收
        return true;
        };
    overlay->getEventDispatcher()->addEventListenerWithSceneGraphPriority(swallowListener, overlay);
}