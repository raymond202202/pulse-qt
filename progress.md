# pulse-qt 迭代进度

> 目标：把 Electron 版 Pulse（网络专家 API 客户端）重写为 Qt 版（轻量本地工具，自己用）
> 参考源码：`~/hermes-projects/pulse`（Electron 版，功能对齐源）
> 规则：每轮实现后 cmake 构建验证 + git commit（本地，**禁止 git push**）；每轮结束更新本文件

## 阶段清单（按序迭代）

- [x] **阶段 0 骨架**：CMake + 主窗口 + RequestPanel（GET/POST/URL/Body）+ ResponsePanel（状态/耗时/文本响应）+ jsonview 库接入（构建通过，二进制 86KB）
- [ ] **阶段 1 响应 JSON 树**：ResponsePanel 嵌入 JsonTreeModel/Delegate（复用 jsonview），JSON 响应树形展示（而非纯文本）
- [ ] **阶段 2 集合/环境**：左栏集合树（集合→请求）+ 环境变量管理（多环境 + `{{var}}` 占位符替换 URL/Headers/Body）
- [ ] **阶段 3 历史记录**：SQLite 存请求历史（方法/URL/时间/状态），点击回填请求区
- [ ] **阶段 4 Headers 编辑**：请求 Headers 表编辑（Key/Value 行）+ Params 表（自动拼到 URL）
- [ ] **阶段 5 AI 面板**：接 flare server（spawn 子进程 + JSON Lines 协议），网络专家（解释报错/生成请求），参考 Pulse 的 pulseTools 语义
- [ ] **阶段 6 打磨**：大响应懒加载/主题（浅色白底紫配 #6d4aff）/状态栏细节/快捷键

## 迭代记录

| 轮次 | 时间 | 完成 | 构建 | 备注 |
|------|------|------|------|------|
| 0 | 22:05 | 骨架 | ✅ | 初始化 |

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
