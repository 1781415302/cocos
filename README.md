# Cocos 卡牌示例游戏 (card2)

作者：1781415302  
分支：card2

简要说明
---
这是一个基于 Cocos2d-x 的小型卡牌/消除类游戏原型。工程采用常见的分层架构（models / views / controllers / managers / services / utils / configs），本 README 旨在帮助维护者与新贡献者快速理解代码结构、开发约定，并提供如何新增卡牌或扩展“回退（undo）”功能的具体步骤与示例。

目录
---
- [概览](#概览)
- [关键文件与目录结构](#关键文件与目录结构)
- [快速运行（本地）](#快速运行本地)
- [开发约定（摘要）](#开发约定摘要)
- [如何添加一张新卡牌](#如何添加一张新卡牌)
  - [在关卡 JSON 中添加（静态）](#在关卡-json-中添加静态)
  - [运行时创建并加入 GameModel（动态）](#运行时创建并加入-gamemodel动态)
- [如何新增一种 Undo 操作（完整步骤）](#如何新增一种-undo-操作完整步骤)
- [模式与注意事项（工程规范）](#模式与注意事项工程规范)
- [示例代码片段：新增卡并 push Undo](#示例代码片段新增卡并-push-undo)
- [建议与下一步](#建议与下一步)

概览
---
这是一个原型级别的卡牌小游戏，目标是作为教学/实验用途，便于扩展。主要职责分层如下：

- models — 纯数据模型（支持 JSON 序列化/反序列化）
- views — UI 层（只读持有 model 指针或通过回调与 controller 交互）
- controllers — 协调 model 与 view，处理用户交互与业务流程
- managers — 辅助的管理或全局服务（非单例）
- services — 无状态的业务函数
- utils — 独立工具（枚举、json 头等）
- configs — 关卡/静态数据加载器（例如 `levels/*.json`）

关键文件与目录结构（相对于 Classes/）
---
- AppDelegate.{cpp,h}
- HelloWorldScene.*, LevelSelectScene.*
- configs/
  - LevelConfig.{h,cpp}
  - Loaders/LevelConfigLoader.{h,cpp}
- controllers/
  - GameController.{h,cpp}
- managers/
  - SaveManager.{h,cpp} （注：UndoManager.h 存在但未实现）
- models/
  - CardModel.{h,cpp}
  - GameModel.{h,cpp}
  - UndoModel.{h,cpp}
- services/
  - GameModelFromLevelGenerator.{h,cpp}
  - GameModelService.{h,cpp}
  - UndoService.{h,cpp}
- utils/
  - CardEnums.h
  - json.hpp (nlohmann::json)
- views/
  - CardView.{h,cpp}
  - UndoView.{h,cpp}
- levels/ （资源目录，存放关卡 JSON，`LevelConfigLoader` 读取）

快速运行（本地）
---
1. 安装并配置与项目对应的 Cocos2d-x 版本（与项目最初编译版本一致）。  
2. 使用你常用的 IDE 或通过 CMake 构建项目。  
3. 运行模拟器或平台程序，查看 Level Select 场景并开始游戏。  
（平台与引擎版本依赖请参考本地 Cocos2d-x 文档）

开发约定（摘要）
---
- models 仅负责数据与序列化，不包含业务逻辑。
- views 负责渲染与事件转发。视图内应持有 `std::weak_ptr<const Model>`（只读）。
- controllers 协调模型与视图，处理用户操作并调用 services/managers。
- managers 不应为单例，可作为 controller 的成员或通过注入提供。
- services 为无状态函数或静态方法，输入/输出明确，不持有数据。
- 所有模型实现 JSON 序列化（使用 nlohmann::json）。

如何添加一张新卡牌
---
下面给出两种方式：在关卡 JSON 中添加（静态）或在运行时通过代码创建（动态）。

在关卡 JSON 中添加（静态）
1. 打开 `levels/<levelId>.json`（例如 `levels/1.json`），找到 Playfield/Stack 节点。  
2. 在对应数组添加一项，示例结构：
```json
{
  "CardSuit": 2,
  "CardFace": 0,
  "Position": { "x": 200, "y": 300 }
}
```
- CardSuit/ CardFace 使用 `utils/CardEnums.h` 中定义的整数值。  
3. 加入资源（若新增花色/点数图片），并确保 `CardView::createFrontNode` 能按命名规则加载对应文件。  
4. `LevelConfigLoader::loadLevelConfig` 会读取 JSON，`GameModelFromLevelGenerator::generateGameModel` 将其转换为 `CardModel` 并加入 `GameModel`。

运行时创建并加入 GameModel（动态）
1. 在 controller 中创建 CardModel：
```cpp
auto card = std::make_shared<CardModel>(CardSuitType::CST_HEARTS, CardFaceType::CFT_ACE, cocos2d::Vec2(200, 300), /*isFaceUp=*/false);
```
2. 将卡牌添加到目标容器：
```cpp
gameModel.addPlayfieldCard(card); // 或 addReserveCard / addHandCard
```
3. 为该 card 创建视图并加入场景：
```cpp
CardView* v = CardView::create(card);
_playfieldNode->addChild(v);
_cardViews[card->getId()] = v;
v->setClickCallback([this](int id){ this->handlePlayfieldCardClick(id); });
```
4. 若新增字段请同时更新 `CardModel::toJson()` 与 `CardModel::fromJson()`，以保持可序列化。

如何新增一种 Undo 操作（完整步骤）
---
项目撤销系统由 `models/UndoModel`（Action + ActionType）、factory 函数、`services/UndoService::applyAction` 以及 `controllers/GameController::handleUndo` 协同实现。下面以新增 "SwapTwoPlayfieldCards" 为例说明全流程：

1) 在 UndoModel 中新增 ActionType、字段与 factory
- 在 `UndoModel::ActionType` 中添加新枚举项，例如 `SwapPlayfieldCards`。
- 在 `UndoModel::Action` 中添加所需字段（如 cardIdA、cardIdB、indexA、indexB）。
- 添加静态工厂函数方便构建：
```cpp
static Action makeSwapPlayfieldCards(int cardIdA, int indexA, int cardIdB, int indexB) {
  Action a;
  a.type = ActionType::SwapPlayfieldCards;
  a.cardIdA = cardIdA;
  a.cardIdB = cardIdB;
  a.indexA = indexA;
  a.indexB = indexB;
  return a;
}
```
- 更新 `Action::toJson()` / `Action::fromJson()`，序列化/反序列化新字段：
```cpp
j["cardIdA"] = cardIdA;
j["cardIdB"] = cardIdB;
j["indexA"] = indexA;
j["indexB"] = indexB;
```

2) 在 UndoService 中实现 applyAction 的新分支
- 在 `UndoService::applyAction(...)` 的 switch 中增加 case：
```cpp
case UndoModel::ActionType::SwapPlayfieldCards:
  return GameModelService::swapPlayfieldCards(model, action.indexA, action.indexB);
```
- 若 `GameModelService` 无对应函数，则添加 `swapPlayfieldCards` 并实现对 `model.getPlayfieldCards()` 的交换与位置更新（更新 CardModel::position 或由 controller 更新视图位置）。

3) 在 Controller 中 push Undo 并做动画
- 在执行交换操作时构造并 push action：
```cpp
UndoModel::Action action = UndoModel::Action::makeSwapPlayfieldCards(cardAId, idxA, cardBId, idxB);
_undoModel.push(action);
```
- 动画：执行正向动画后在回调中修改模型，或先修改模型再由 controller 发起视图的补偿动画。`handleUndo()` 中加入回放新 ActionType 的视图恢复逻辑。

4) 序列化
- 由于你已在 Action 的 toJson/fromJson 中添加字段，`SaveManager` 会在保存时包含新的 undo 条目；通常无需修改 `SaveManager`。

5) 测试
- 运行并验证：触发操作 → 执行 Undo → 检查模型、视图与保存数据的一致性。建议编写单元测试验证 `UndoService::applyAction` 的行为。

模式与注意事项（工程规范）
---
- View 与 Model 的所有权
  - Model 由 Controller/Manager 持有（例如 `GameModel` 作为 `GameController` 成员）。
  - View 只持有 `std::weak_ptr<const CardModel>`，通过 callback 把事件传给 controller。
- Managers 不做单例
  - 例如 `SaveManager` 应注入到使用它的场景或 controller，而不是在全局使用单例。
- Services 为无状态
  - `GameModelService` / `UndoService` 应仅暴露纯函数或静态函数，输入为 `GameModel&`，无内部状态。
- 可测试性
  - 抽象文件系统或平台相关依赖，便于对 `SaveManager` 等进行单元测试与 Mock。

示例：添加新卡并 push Undo（代码片段）
---
```cpp
// 创建并加入 playfield
auto newCard = std::make_shared<CardModel>(CardSuitType::CST_HEARTS, CardFaceType::CFT_FIVE, Vec2(100,200), true);
_gameModel.addPlayfieldCard(newCard);

// 创建视图
CardView* view = CardView::create(newCard);
_playfieldNode->addChild(view);
_cardViews[newCard->getId()] = view;
view->setClickCallback([this](int id){ this->handlePlayfieldCardClick(id); });

// 构造并 push 可撤销动作
auto action = UndoModel::Action::makeMoveHandToPlayfield(/* args */);
_undoModel.push(action);

// 可选：保存当前进度
saveActiveIfSet();
```

建议与下一步
---
当前仓库分层清晰，便于扩展。为提高可维护性与可测试性，推荐按优先级执行：

1. 将 CardView 持有的 model 类型改为 `std::weak_ptr<const CardModel>`（增加只读保护）。  
2. 抽象 `SaveManager` 的文件系统/平台依赖，以便注入 mock。  
3. 清理未实现的 `UndoManager.h`（删除或实现占位）。  
4. 在 `UndoModel` / `UndoService` 中加入注释示例，指导新增 undo 类型的贡献者。

如果你需要，我可以：
- A) 将本 README 保存到仓库并发起 PR（需要你授权在仓库上创建分支/提交）。  
- B) 把 CardView 的 const 改动与 GameController 的小幅重构打成补丁 / PR。  
- C) 把一个具体的 Undo 类型（例如 SwapPlayfieldCards）的完整实现做成补丁并包含单元测试示例。

感谢你提供详细说明！请告诉我你希望我接下来做哪一步（例如生成 PR / 直接把 README 写入仓库 / 继续实现某个补丁）。