// Classes/HelloWorldScene.cpp
#include "HelloWorldScene.h"
#include "controllers/GameController.h"
#include "ui/CocosGUI.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    // 默认进入 1 关
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
    // 兼容原有入口，默认加载 1 关
    return initWithLevel("1");
}

bool HelloWorld::initWithLevel(const std::string& levelId)
{
    if (!Scene::init()) {
        return false;
    }

    _levelId = levelId;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 背景色层
    auto layerColor = LayerColor::create(Color4B(25, 100, 25, 255));
    this->addChild(layerColor, -1);

    // 标题
    auto label = Label::createWithSystemFont("Card Game Demo", "Arial", 28);
    label->setPosition(origin + Vec2(visibleSize.width * 0.5f, visibleSize.height - 30));
    this->addChild(label, 1000);

    // 退出按钮（PC 平台可用）
    auto closeItem = ui::Button::create();
    closeItem->setTitleText("Quit");
    closeItem->setTitleFontSize(20);
    closeItem->setPosition(Vec2(origin.x + visibleSize.width - 60, origin.y + visibleSize.height - 30));
    closeItem->addClickEventListener([this](Ref*) {
        Director::getInstance()->end();
        });
    this->addChild(closeItem, 1000);

    // GameController 的父节点
    _gameParentNode = Node::create();
    _gameParentNode->setPosition(origin + Vec2(10, 10)); // 简单边距
    this->addChild(_gameParentNode, 0);

    // 创建 GameController
    _gameController = new GameController(_gameParentNode);

    // 按传入的关卡 ID 启动游戏
    _gameController->startGame(_levelId);

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