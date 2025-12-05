#ifndef  _APP_DELEGATE_H_
#define  _APP_DELEGATE_H_

#include "cocos2d.h"

/**
@brief cocos2d 应用程序。

这里使用私有继承以从 Director 隐藏部分接口。
*/
class  AppDelegate : private cocos2d::Application
{
public:
    AppDelegate();
    virtual ~AppDelegate();

    virtual void initGLContextAttrs();

    /**
    @brief 在此实现 Director 和 Scene 的初始化代码。
    @return true    初始化成功，应用继续运行。
    @return false   初始化失败，应用终止。
    */
    virtual bool applicationDidFinishLaunching();

    /**
    @brief 应用进入后台时调用
    @param 应用程序指针
    */
    virtual void applicationDidEnterBackground();

    /**
    @brief 应用重新进入前台时调用
    @param 应用程序指针
    */
    virtual void applicationWillEnterForeground();
};

#endif // _APP_DELEGATE_H_

