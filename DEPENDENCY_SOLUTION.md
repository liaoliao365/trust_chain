# 依赖问题解决方案

## 问题描述

在重构过程中，我们发现了一个依赖问题：

1. **循环依赖问题**：
   - `utils.c` 需要 `trust_chain_ta.h` 中的常量定义（如 `ROLE_ADMIN`）
   - `trust_chain_ta.c` 需要 `utils.h` 中的函数声明
   - 这造成了循环依赖

2. **外部变量问题**：
   - `utils.c` 中声明了 `extern struct repo_metadata *repositories[];`
   - 但 `repositories` 在 `trust_chain_ta.c` 中定义为 `static`
   - 这导致链接错误

## 解决方案

### 1. 循环依赖解决方案

创建了独立的常量定义文件 `trust_chain_constants.h`：

```c
// trust_chain_constants.h
#define ROLE_ADMIN  1
#define ROLE_WRITER 2
#define MAX_REPO_ID 1000
// ... 其他常量
```

**新的依赖层次**：
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

### 2. 外部变量解决方案

#### 方案选择
考虑了几种方案：

1. **保持 static 变量**：通过函数参数传递（复杂）
2. **改为全局变量**：通过函数接口访问（推荐）
3. **创建仓库管理模块**：过度设计

选择了方案2，因为：
- 简单有效
- 保持了封装性（通过函数接口访问）
- 避免了过度设计

#### 具体实现

1. **修改变量声明**：
   ```c
   // trust_chain_ta.c
   struct repo_metadata *repositories[MAX_REPO_ID];  // 移除 static
   ```

2. **添加仓库管理函数**：
   ```c
   // utils.h
   TEE_Result get_repo_metadata(uint32_t rep_id, struct repo_metadata **repo);
   TEE_Result update_repo_block_height(uint32_t rep_id, uint32_t block_height);
   TEE_Result update_repo_latest_hash(uint32_t rep_id, const char *hash);
   ```

3. **实现仓库管理函数**：
   ```c
   // utils.c
   TEE_Result get_repo_metadata(uint32_t rep_id, struct repo_metadata **repo) {
       if (rep_id >= MAX_REPO_ID || repositories[rep_id] == NULL) {
           return TEE_ERROR_ITEM_NOT_FOUND;
       }
       *repo = repositories[rep_id];
       return TEE_SUCCESS;
   }
   ```

4. **修改工具函数**：
   ```c
   // utils.c 中的函数现在通过 get_repo_metadata() 访问仓库
   TEE_Result get_access_block(...) {
       struct repo_metadata *repo;
       res = get_repo_metadata(rep_id, &repo);
       // 使用 repo 而不是 repositories[rep_id]
   }
   ```

## 最终架构

```
trust_chain_constants.h  (常量定义)
    ↓
trust_chain_ta.h        (数据结构定义)
    ↓
utils.h                 (工具函数声明 + 仓库管理函数声明)
    ↓
utils.c                 (工具函数实现 + 仓库管理函数实现)
    ↓
trust_chain_ta.c        (主要业务逻辑 + repositories 数组)
```

## 优势

1. **避免循环依赖**：通过分层设计解决
2. **保持封装性**：通过函数接口访问全局变量
3. **清晰的依赖关系**：每个模块的依赖都很明确
4. **易于维护**：常量集中管理，函数职责明确

## 注意事项

1. `repositories` 数组现在是全局变量，但只能通过函数接口访问
2. 所有对 `repositories` 的直接访问都应该通过仓库管理函数
3. 新增仓库相关操作时，应该添加到仓库管理函数中
4. 常量定义应该放在 `trust_chain_constants.h` 中

## 验证

可以通过以下方式验证解决方案：

1. **编译测试**：确保没有循环依赖错误
2. **链接测试**：确保没有未定义的外部变量错误
3. **功能测试**：确保所有功能正常工作

这个解决方案既解决了技术问题，又保持了代码的清晰性和可维护性。 