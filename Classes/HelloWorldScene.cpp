// Classes/HelloWorldScene.cpp
#include "HelloWorldScene.h"
#include "controllers/GameController.h"
#include "ui/CocosGUI.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
}

bool HelloWorld::init()
{
    if (!Scene::init()) {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 背景色（可选）
    auto layerColor = LayerColor::create(Color4B(25, 100, 25, 255));
    this->addChild(layerColor, -1);

    // 顶部标题，便于调试
    auto label = Label::createWithSystemFont("Card Game Demo", "Arial", 28);
    label->setPosition(origin + Vec2(visibleSize.width * 0.5f, visibleSize.height - 30));
    this->addChild(label, 1000);

    // 退出按钮（方便 PC 平台测试）
    auto closeItem = ui::Button::create();
    closeItem->setTitleText("Quit");
    closeItem->setTitleFontSize(20);
    closeItem->setPosition(Vec2(origin.x + visibleSize.width - 60, origin.y + visibleSize.height - 30));
    closeItem->addClickEventListener([this](Ref*) {
        Director::getInstance()->end();
        });
    this->addChild(closeItem, 1000);

    // 创建一个用于放置游戏节点的 parent node（传入 GameController）
    _gameParentNode = Node::create();
    // 你可以调整位置 / 缩放 / anchor 来匹配你的布局
    _gameParentNode->setPosition(origin + Vec2(10, 10)); // 留点边距
    this->addChild(_gameParentNode, 0);

    // 创建 GameController，传入 parent node
    _gameController = new GameController(_gameParentNode);

    // 启动关卡（示例使用 "1"）
    // 注意：确保 Resources/levels/1.json 存在且路径正确
    _gameController->startGame("1");

    return true;
}

void HelloWorld::onExit()
{
    // 删除 GameController（如果有必要）
    if (_gameController) {
        delete _gameController;
        _gameController = nullptr;
    }
    Scene::onExit();
}