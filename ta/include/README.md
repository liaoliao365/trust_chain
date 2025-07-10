# Include 目录说明

这个目录包含了 trust_chain 项目的头文件，用于避免循环依赖问题。

## 文件结构

```
include/
├── trust_chain_ta.h         # 主要的数据结构定义
├── trust_chain_constants.h  # 常量定义（避免循环依赖）
└── README.md               # 说明文档
```

## 依赖关系设计

### 问题描述
在重构过程中，我们发现了一个循环依赖问题：
- `utils.c` 需要 `trust_chain_ta.h` 中的常量定义（如 `ROLE_ADMIN`）
- `trust_chain_ta.c` 需要 `utils.h` 中的函数声明
- 这造成了循环依赖

### 解决方案
创建了独立的 `trust_chain_constants.h` 文件来存放所有常量定义：

#### trust_chain_constants.h
- 包含所有操作类型定义（`OP_ADD`, `OP_DELETE`, `OP_PUSH`）
- 包含所有角色类型定义（`ROLE_ADMIN`, `ROLE_WRITER`）
- 包含所有最大长度定义（`MAX_REPO_ID`, `MAX_KEY_LENGTH` 等）

#### 新的依赖关系
```
trust_chain_constants.h  (基础常量)
    ↓
trust_chain_ta.h        (数据结构)
    ↓
utils.h                 (工具函数声明)
    ↓
utils.c                 (工具函数实现)
    ↓
trust_chain_ta.c        (主要业务逻辑)
```

### 依赖层次
1. **trust_chain_constants.h** - 最底层，只包含常量定义
2. **trust_chain_ta.h** - 包含数据结构定义，依赖常量
3. **utils.h** - 包含工具函数声明，依赖常量和数据结构
4. **utils.c** - 实现工具函数，依赖常量和数据结构
5. **trust_chain_ta.c** - 主要业务逻辑，依赖所有其他模块

## 使用示例

### 在 utils.h 中
```c
#include "trust_chain_constants.h"  // 获取常量定义
```

### 在 trust_chain_ta.h 中
```c
#include "trust_chain_constants.h"  // 获取常量定义
```

### 在 utils.c 中
```c
#include "utils.h"                  // 获取函数声明
#include "key_list/key_list.h"      // 获取密钥链表功能
// 不需要包含 trust_chain_ta.h，因为常量已在 utils.h 中获取
```

## 优势

1. **避免循环依赖**：通过分层设计，避免了循环包含
2. **清晰的依赖关系**：每个文件的依赖关系都很明确
3. **易于维护**：常量定义集中管理，便于修改
4. **模块化**：每个模块职责明确，便于测试和重用

## 注意事项

1. 新增常量时，应该放在 `trust_chain_constants.h` 中
2. 新增数据结构时，应该放在 `trust_chain_ta.h` 中
3. 新增工具函数时，应该放在 `utils.h` 和 `utils.c` 中
4. 避免在高层模块中直接包含低层模块的实现文件 