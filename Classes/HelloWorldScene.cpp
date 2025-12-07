// Classes/HelloWorldScene.cpp
#include "HelloWorldScene.h"
#include "controllers/GameController.h"
#include "LevelSelectScene.h"
#include "ui/CocosGUI.h"
#include <algorithm>
#include<string>

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

    // 背景图（优先显示），按屏幕等比缩放以覆盖可视区域
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

    // 背景色层（保留但设为透明，以便在背景图不存在时作为备用颜色）
    auto layerColor = LayerColor::create(Color4B(25, 100, 25, 0));
    this->addChild(layerColor, -1);

    // 注意：请确保把支持中文的 TTF（例如 simhei.ttf）放到 resources/fonts/ 下
    const std::string chineseFont = "fonts/FLjiangdouti-Regular-2.ttf";

    // 标题：使用 UTF-8 字面量并显式构造 std::string 以避免指针运算错误
    std::string title = std::string(u8"关卡") + levelId;
    auto label = Label::createWithTTF(title, chineseFont, 50);
    label->setPosition(origin + Vec2(visibleSize.width * 0.5f, visibleSize.height - 30));
    this->addChild(label, 1000);

    // 退出/返回按钮：改为返回关卡选择页面，确保按钮标题使用中文字体
    auto closeItem = ui::Button::create();
    closeItem->setTitleFontName(chineseFont);
    closeItem->setTitleFontSize(40);
    closeItem->setTitleText(u8"退出");
    closeItem->setPosition(Vec2(origin.x + visibleSize.width - 60, origin.y + visibleSize.height - 30));
    closeItem->addClickEventListener([this](Ref*) {
        auto scene = LevelSelectScene::createScene();
        Director::getInstance()->replaceScene(TransitionFade::create(0.3f, scene));
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