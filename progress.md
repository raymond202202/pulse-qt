# pulse-qt 迭代进度

> 目标：把 Electron 版 Pulse（网络专家 API 客户端）重写为 Qt 版（轻量本地工具，自己用）
> 参考源码：`~/hermes-projects/pulse`（Electron 版，功能对齐源）
> 规则：每轮实现后 cmake 构建验证 + git commit（本地，**禁止 git push**）；每轮结束更新本文件

## 阶段清单（按序迭代）

- [x] **阶段 0 骨架**：CMake + 主窗口 + RequestPanel（GET/POST/URL/Body）+ ResponsePanel（状态/耗时/文本响应）+ jsonview 库接入（构建通过，二进制 86KB）
- [x] **阶段 1 响应 JSON 树**：ResponsePanel 嵌入 JsonTreeModel/Delegate（复用 jsonview），JSON 响应树形展示（而非纯文本），树形/文本双视图 + 状态着色 + 复制 + 浅色紫配主题
- [x] **阶段 2 集合/环境**：左栏集合树（集合→文件夹→请求，右键管理、点击回填）+ 环境变量管理（多环境 + `{{var}}` 占位符替换 URL/Body）+ 保存到集合
- [x] **阶段 3 历史记录**：SQLite 存请求历史（方法/URL/时间/状态），发送自动记录，左栏"历史"页签点击回填请求区
- [x] **阶段 4 Headers 编辑**：请求 Headers 表编辑（启用/Key/Value 行）+ Params 表（自动拼到 URL），载荷持久化到集合/历史
- [x] **阶段 5 AI 面板**：接 flare server（spawn 子进程 + JSON Lines 协议），网络专家（解释报错/生成请求/工具调用），key 由 flare 从环境变量/~/.flare/.env 读取（不硬编码）
- [x] **阶段 6 打磨**：大响应懒加载（树按需展开 + 文本 >512KB 截断提示）/主题（浅色白底紫配 #6d4aff）/状态栏（版本·环境·flare·最近响应）/快捷键（Ctrl+Enter 发送、Ctrl+S 保存）
- [x] **全部阶段完成** 🎉 7 个阶段（0-6）全部构建通过并 commit

## 迭代记录

| 轮次 | 时间 | 完成 | 构建 | 备注 |
|------|------|------|------|------|
| 0 | 22:05 | 骨架 | ✅ | 初始化 |
| 1 | 本轮 | 阶段1 JSON 树 | ✅ | ResponsePanel 嵌入 jsonview 懒加载树；树形/文本切换；状态着色；浅色紫配主题 QSS；冒烟测试通过 |
| 2 | 本轮 | 阶段2 集合/环境 | ✅ | CollectionStore/EnvironmentStore 单例（QSettings）；左栏集合树+右键管理；环境对话框+{{var}}解析；保存到集合；三栏布局 v0.3.0 |
| 3 | 本轮 | 阶段3 历史记录 | ✅ | HistoryStore（QSQLITE，500条上限）；发送自动记录；历史页签点击回填；SQLite 写入/查询实测通过 v0.4.0 |
| 4 | 本轮 | 阶段4 Headers/Params | ✅ | Body/Headers/Params 页签；表编辑（启用/Key/Value）；Params 自动拼 URL；Headers 设请求头；载荷持久化+旧库迁移实测 v0.5.0 |
| 5 | 本轮 | 阶段5 AI 面板 | ✅ | FlareServer spawn flare server（JSON Lines）；AI 助手页签+流式聊天+12 个宿主工具（http_request 异步/pulse_*）；上下文快照；端到端实测 v0.6.0 |
| 6 | 本轮 | 阶段6 打磨 | ✅ | 快捷键（Ctrl+Enter/Ctrl+S）；状态栏（环境/flare/最近响应）；大响应文本截断+树懒加载；全部 7 阶段完成 v0.7.0 |
| 7 | 本轮 | 无（全部完成） | ✅ | 夜间验证轮：无未完成阶段；重新构建 0 错误；offscreen 冒烟启动正常；工作区干净 |
| 8 | 本轮 | 无（全部完成） | ✅ | 夜间验证轮：无未完成阶段；cmake 重新构建 0 错误（二进制 1.0MB）；offscreen 启动事件循环存活正常无报错；工作区干净 |
| 9 | 本轮 | 无（全部完成） | ✅ | 夜间验证轮：无未完成阶段；git 工作区干净（master 10 个提交）；重新构建 0 错误；offscreen 冒烟启动存活 6s 无任何报错输出；构建产物 1.0MB |
| 10 | 本轮 | 无（全部完成） | ✅ | 夜间验证轮：无未完成阶段；git 工作区干净（master 12 个提交）；cmake 重新构建 0 错误；offscreen 冒烟启动存活 6s 无任何报错输出；构建产物 1.0MB |
| 11 | 本轮 | 无（全部完成） | ✅ | 夜间验证轮：无未完成阶段；git 工作区干净（master 12 个提交）；cmake 重新构建 0 错误；offscreen 冒烟启动存活 6s 零输出；构建产物 1.0MB |
| 12 | 本轮 | 无（全部完成） | ✅ | 夜间验证轮：无未完成阶段；git 工作区干净（master 13 个提交）；cmake 重新构建 0 错误；offscreen 冒烟启动存活 8s 零输出；构建产物 1.0MB |
| 13 | 本轮 | 无（全部完成） | ✅ | 夜间验证轮：无未完成阶段；git 工作区干净（master 13 个提交）；cmake 重新构建 0 错误；offscreen 冒烟启动存活 8s 零输出；构建产物 1.0MB |
| 14 | 本轮 | 无（全部完成） | ✅ | 夜间验证轮：无未完成阶段；git 工作区干净（master 15 个提交）；cmake 重新构建 0 错误；offscreen 冒烟启动存活 8s 零输出；构建产物 1.0MB |
| 15 | 本轮 | 无（全部完成） | ✅ | 夜间验证轮：无未完成阶段；git 工作区干净（master 15 个提交）；cmake 重新构建 0 错误；offscreen 冒烟启动存活 8s 零输出；构建产物 1.0MB |

## 构建命令

```bash
cd ~/hermes-projects/pulse-qt
cmake -B build && cmake --build build -j$(nproc)
./build/pulse-qt
```

## 铁律

- **禁止 git push**（用户明早验收后才决定是否推 GitHub）
- 不动其他仓库（pulse/storyspire/flare/json-viewer 只读参考）
- 每轮必须构建通过才 commit；构建失败要修复后继续
- UI 浅色主题白底紫配 #6d4aff
