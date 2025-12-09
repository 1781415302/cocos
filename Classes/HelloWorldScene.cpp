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
    // 默认进入关卡 "1"，新建一个 SaveManager 实例
    return HelloWorld::createSceneWithLevel("1", std::make_shared<SaveManager>());
}

Scene* HelloWorld::createSceneWithLevel(const std::string& levelId, std::shared_ptr<SaveManager> saveManager)
{
    auto scene = HelloWorld::createWithLevel(levelId, saveManager);
    return scene;
}

HelloWorld* HelloWorld::createWithLevel(const std::string& levelId, std::shared_ptr<SaveManager> saveManager)
{
    HelloWorld* ret = new (std::nothrow) HelloWorld();
    if (ret && ret->initWithLevel(levelId, saveManager))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool HelloWorld::init()
{
    return initWithLevel("1", std::make_shared<SaveManager>());
}

bool HelloWorld::initWithLevel(const std::string& levelId, std::shared_ptr<SaveManager> saveManager)
{
    CCLOG("HelloWorld::initWithLevel - begin levelId=%s", levelId.c_str());

    if (!Scene::init()) {
        CCLOG("HelloWorld::initWithLevel - Scene::init failed");
        return false;
    }

    _levelId = levelId;
    // 若未传入，则新建一个 SaveManager
    _saveManager = saveManager ? saveManager : std::make_shared<SaveManager>();

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    auto bgSprite = Sprite::create("background.png");
    if (bgSprite) {
        Size bgSize = bgSprite->getContentSize();
        float scaleX = visibleSize.width / bgSize.width;
        float scaleY = visibleSize.height / bgSize.height;
        float scale = std::max(scaleX, scaleY);
        bgSprite->setScale(scale);
        bgSprite->setPosition(origin + Vec2(visibleSize.width * 0.5f, visibleSize.height * 0.5f));
        this->addChild(bgSprite, -2);
    }
    else {
        CCLOG("HelloWorld::initWithLevel - background.png not found");
    }

    auto layerColor = LayerColor::create(Color4B(25, 100, 25, 0));
    this->addChild(layerColor, -1);

    const std::string chineseFont = "fonts/FLjiangdouti-Regular-2.ttf";

    std::string title = std::string(u8"关卡 ") + levelId;
    auto titleLabel = Label::createWithTTF(title, chineseFont, 50);
    if (titleLabel) {
        titleLabel->setPosition(origin + Vec2(visibleSize.width * 0.5f, visibleSize.height - 30));
        this->addChild(titleLabel, 1000);
    }
    else {
        CCLOG("HelloWorld::initWithLevel - failed to create title label");
    }

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

    _gameParentNode = Node::create();
    _gameParentNode->setPosition(origin + Vec2(10, 10));
    this->addChild(_gameParentNode, 0);

    // 将 SaveManager 注入 GameController
    _gameController = new GameController(_gameParentNode, _saveManager.get());

    // 处理 pending load
    std::string pending = _saveManager->getPendingLoadPath();
    CCLOG("HelloWorld::initWithLevel - pending save path = '%s'", pending.c_str());

    if (!pending.empty()) {
        CCLOG("HelloWorld::initWithLevel - attempting to start game from save: %s", pending.c_str());
        _gameController->startGame(_levelId, pending);
        _saveManager->setActiveSavePath(pending);
        _saveManager->setPendingLoadPath("");
        CCLOG("HelloWorld::initWithLevel - started game from save %s", pending.c_str());
    }
    else {
        _gameController->startGame(_levelId);
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