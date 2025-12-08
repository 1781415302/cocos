// Classes/HelloWorldScene.cpp
#include "HelloWorldScene.h"
#include "controllers/GameController.h"
#include "LevelSelectScene.h"
#include "managers/SaveManager.h"
#include "ui/CocosGUI.h"
#include <algorithm>
#include <string>

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    // 默认进入关卡 "1"
    return HelloWorld::createSceneWithLevel("1");
}

Scene* HelloWorld::createSceneWithLevel(const std::string& levelId)
{
    auto scene = HelloWorld::createWithLevel(levelId);
    return scene;
}

HelloWorld* HelloWorld::createWithLevel(const std::string& levelId)
{
    HelloWorld* ret = new (std::nothrow) HelloWorld();
    if (ret && ret->initWithLevel(levelId))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool HelloWorld::init()
{
    return initWithLevel("1");
}

bool HelloWorld::initWithLevel(const std::string& levelId)
{
    CCLOG("HelloWorld::initWithLevel - begin levelId=%s", levelId.c_str());

    if (!Scene::init()) {
        CCLOG("HelloWorld::initWithLevel - Scene::init failed");
        return false;
    }

    _levelId = levelId;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 背景图片（居中并按最大的比例缩放以填充屏幕）
    auto bgSprite = Sprite::create("background.png");
    if (bgSprite) {
        Size bgSize = bgSprite->getContentSize();
        float scaleX = visibleSize.width / bgSize.width;
        float scaleY = visibleSize.height / bgSize.height;
        float scale = std::max(scaleX, scaleY);
        bgSprite->setScale(scale);
        bgSprite->setPosition(origin + Vec2(visibleSize.width * 0.5f, visibleSize.height * 0.5f));
        this->addChild(bgSprite, -2); // 放在最底层
    }
    else {
        CCLOG("HelloWorld::initWithLevel - background.png not found");
    }

    // 半透明底色层（在背景和游戏区之间）
    auto layerColor = LayerColor::create(Color4B(25, 100, 25, 0));
    this->addChild(layerColor, -1);

    // 字体（确保 fonts 文件夹中存在）
    const std::string chineseFont = "fonts/FLjiangdouti-Regular-2.ttf";

    // 关卡标题（显示当前关卡）
    std::string title = std::string(u8"关卡 ") + levelId;
    auto titleLabel = Label::createWithTTF(title, chineseFont, 50);
    if (titleLabel) {
        titleLabel->setPosition(origin + Vec2(visibleSize.width * 0.5f, visibleSize.height - 30));
        this->addChild(titleLabel, 1000);
    }
    else {
        CCLOG("HelloWorld::initWithLevel - failed to create title label");
    }

    // 退出/返回按钮（右上）
    auto closeItem = ui::Button::create();
    closeItem->setTitleFontName(chineseFont);
    closeItem->setTitleFontSize(40);
    closeItem->setTitleText(u8"退出");
    closeItem->setAnchorPoint(Vec2(1.0f, 1.0f));
    closeItem->setPosition(origin + Vec2(visibleSize.width - 10, visibleSize.height - 10));
    closeItem->addClickEventListener([this](Ref*) {
        auto scene = LevelSelectScene::createScene();
        Director::getInstance()->replaceScene(TransitionFade::create(0.3f, scene));
        });
    this->addChild(closeItem, 1000);

    // 游戏区域父节点（GameController 会把 playfield / reserve / hand 放在这个节点下）
    _gameParentNode = Node::create();
    _gameParentNode->setPosition(origin + Vec2(10, 10)); // 留边距
    this->addChild(_gameParentNode, 0);

    // 创建 GameController（不在 init 中直接 startGame，因为可能需要先从 SaveManager 加载 pending）
    _gameController = new GameController(_gameParentNode);

    // 检查是否有 pending 存档路径（从 LevelSelect 点击打开存档时设置）
    std::string pending = SaveManager::getInstance().getPendingLoadPath();
    CCLOG("HelloWorld::initWithLevel - pending save path = '%s'", pending.c_str());

    if (!pending.empty()) {
        // 如果存在 pending 存档，优先从存档加载
        bool ok = false;
        CCLOG("HelloWorld::initWithLevel - attempting to start game from save: %s", pending.c_str());
        _gameController->startGame(_levelId, pending);
        // 将该路径设为 active，这样后续自动保存会写回同一文件
        SaveManager::getInstance().setActiveSavePath(pending);
        // 清空 pending 避免重复加载
        SaveManager::getInstance().setPendingLoadPath("");
        CCLOG("HelloWorld::initWithLevel - started game from save %s", pending.c_str());
    }
    else {
        // 正常新建关卡
        _gameController->startGame(_levelId);
        // SaveManager 会在 GameController::startGame 内创建新的活动存档并保存初始状态
        CCLOG("HelloWorld::initWithLevel - started new game level %s", _levelId.c_str());
    }

    CCLOG("HelloWorld::initWithLevel - end");
    return true;
}

void HelloWorld::onExit()
{
    if (_gameController) {
        delete _gameController;
        _gameController = nullptr;
    }
    Scene::onExit();
}