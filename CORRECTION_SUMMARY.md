# 项目更正总结

根据依赖分析，对 trust_chain 项目进行了全面更正，解决了循环依赖和外部变量问题。

## 更正内容

### 1. 创建了独立的常量定义文件

**文件**: `optee_examples/trust_chain/ta/include/trust_chain_constants.h`

包含所有常量定义：
```c
#define ROLE_ADMIN  1
#define ROLE_WRITER 2
#define MAX_REPO_ID 1000
#define MAX_KEY_LENGTH 256
#define MAX_HASH_LENGTH 64
#define MAX_SIGNATURE_LENGTH 512
#define MAX_BRANCH_LENGTH 128
#define OP_ADD     0
#define OP_DELETE  1
#define OP_PUSH    2
```

### 2. 修改了依赖关系

**修改前**:
```
utils.c → trust_chain_ta.h
trust_chain_ta.c → utils.h
```

**修改后**:
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

### 3. 添加了仓库管理函数

**在 utils.h 中添加**:
```c
TEE_Result get_repo_metadata(uint32_t rep_id, struct repo_metadata **repo);
TEE_Result update_repo_block_height(uint32_t rep_id, uint32_t block_height);
TEE_Result update_repo_latest_hash(uint32_t rep_id, const char *hash);
```

**在 utils.c 中实现**:
```c
TEE_Result get_repo_metadata(uint32_t rep_id, struct repo_metadata **repo) {
    if (rep_id >= MAX_REPO_ID || repositories[rep_id] == NULL) {
        return TEE_ERROR_ITEM_NOT_FOUND;
    }
    *repo = repositories[rep_id];
    return TEE_SUCCESS;
}
```

### 4. 修改了变量作用域

**修改前**:
```c
static struct repo_metadata *repositories[MAX_REPO_ID];
```

**修改后**:
```c
struct repo_metadata *repositories[MAX_REPO_ID];  // 移除 static
```

### 5. 更新了所有直接访问

#### 5.1 在 trust_chain_ta.c 中

**修改前**:
```c
repositories[rep_id]->block_height = block.block_height;
TEE_StrCpy(repositories[rep_id]->latest_hash, block.parent_hash);
```

**修改后**:
```c
update_repo_block_height(rep_id, block.block_height);
update_repo_latest_hash(rep_id, block.parent_hash);
```

#### 5.2 在 get_latest_hash 函数中

**修改前**:
```c
TEE_StrCpy(hash_out, repositories[rep_id]->latest_hash);
```

**修改后**:
```c
struct repo_metadata *repo;
res = get_repo_metadata(rep_id, &repo);
if (res == TEE_SUCCESS) {
    TEE_StrCpy(hash_out, repo->latest_hash);
}
```

#### 5.3 在权限检查中

**修改前**:
```c
if (!key_exists_in_set(repositories[rep_id]->admin_keys, pubkey)) {
    return TEE_ERROR_ACCESS_DENIED;
}
```

**修改后**:
```c
struct repo_metadata *repo;
res = get_repo_metadata(rep_id, &repo);
if (res != TEE_SUCCESS) {
    return res;
}
if (!key_exists_in_set(repo->admin_keys, pubkey)) {
    return TEE_ERROR_ACCESS_DENIED;
}
```

### 6. 更新了工具函数

#### 6.1 repo_exists 函数

**修改前**:
```c
TEE_Result repo_exists(uint32_t rep_id) {
    if (rep_id >= MAX_REPO_ID || repositories[rep_id] == NULL) {
        return TEE_ERROR_ITEM_NOT_FOUND;
    }
    return TEE_SUCCESS;
}
```

**修改后**:
```c
TEE_Result repo_exists(uint32_t rep_id) {
    struct repo_metadata *repo;
    return get_repo_metadata(rep_id, &repo);
}
```

#### 6.2 get_access_block 和 get_contribution_block 函数

现在通过 `get_repo_metadata()` 函数获取仓库元数据，而不是直接访问 `repositories` 数组。

## 更正后的优势

1. **避免循环依赖**: 通过分层设计解决
2. **保持封装性**: 通过函数接口访问全局变量
3. **清晰的依赖关系**: 每个模块的依赖都很明确
4. **易于维护**: 常量集中管理，函数职责明确
5. **类型安全**: 通过函数接口提供类型检查

## 验证方法

1. **编译测试**: 确保没有循环依赖错误
2. **链接测试**: 确保没有未定义的外部变量错误
3. **功能测试**: 确保所有功能正常工作

## 文件结构

```
optee_examples/trust_chain/ta/
├── include/
│   ├── trust_chain_ta.h         # 主要数据结构
│   ├── trust_chain_constants.h  # 常量定义
│   └── README.md               # 说明文档
├── key_list/
│   ├── key_list.h              # 密钥链表模块
│   ├── key_list.c
│   └── README.md
├── utils/
│   ├── utils.h                 # 工具类模块
│   ├── utils.c
│   └── README.md
├── trust_chain_ta.c            # 主要业务逻辑
├── Makefile
└── ...
```

所有更正已完成，项目现在具有清晰的依赖关系和良好的模块化结构。 