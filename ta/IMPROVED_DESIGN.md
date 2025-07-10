# 改进后的模块设计

## 问题分析

你提出的问题非常正确！原来的设计确实存在问题：

1. **职责不清**：`utils` 模块声明了不属于它的变量 `repositories`
2. **依赖混乱**：`utils` 模块依赖 `trust_chain_ta.c` 中的全局变量
3. **模块边界模糊**：仓库管理应该是核心业务逻辑，不是工具函数

## 改进后的设计

### 1. 清晰的模块职责

```
trust_chain_ta.c (核心模块)
├── 全局变量定义 (repositories[])
├── 仓库管理函数 (repo_exists)
├── 主要业务逻辑
└── TA接口实现

utils/ (工具模块)
├── 纯工具函数 (hash_data, verify_signature)
├── 区块生成函数 (get_access_block, get_contribution_block)
└── 无全局变量依赖

key_list/ (密钥管理模块)
├── 密钥链表操作
└── 独立的密钥管理功能
```

### 2. 改进的依赖关系

**之前的问题**：
```
utils.h 声明 extern repositories[]
utils.c 调用 get_repo_metadata()
trust_chain_ta.c 定义 repositories[]
trust_chain_ta.c 调用 utils 函数
```

**现在的设计**：
```
trust_chain_ta.c 定义 repositories[]
trust_chain_ta.c 实现 repo_exists()
trust_chain_ta.c 调用 utils 函数，传递仓库参数
utils 函数通过参数接收仓库数据，无全局变量依赖
```

### 3. 函数签名改进

**之前**：
```c
TEE_Result get_access_block(uint32_t rep_id, ...);
TEE_Result get_contribution_block(uint32_t rep_id, ...);
```

**现在**：
```c
TEE_Result get_access_block(const struct repo_metadata *repo, ...);
TEE_Result get_contribution_block(const struct repo_metadata *repo, ...);
```

### 4. 优势

1. **清晰的职责分离**：
   - 核心模块管理全局状态
   - 工具模块提供纯函数
   - 无循环依赖

2. **更好的封装性**：
   - 工具函数不依赖全局变量
   - 通过参数传递数据
   - 便于单元测试

3. **符合C语言最佳实践**：
   - 声明在头文件，定义在实现文件
   - 避免重复定义
   - 清晰的模块边界

## 总结

你的质疑完全正确！原来的设计确实存在问题。现在的设计：

- ✅ 核心模块管理全局状态
- ✅ 工具模块提供纯函数
- ✅ 无循环依赖
- ✅ 清晰的模块边界
- ✅ 符合C语言最佳实践

这种设计更加合理，感谢你的提醒！ 