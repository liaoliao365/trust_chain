# Utils 工具类模块

这个模块包含了 trust_chain 项目中的各种工具类函数，提供签名验证、时间获取、哈希计算、仓库管理等功能。

## 文件结构

```
utils/
├── utils.h      # 头文件，包含所有工具函数的声明
├── utils.c      # 实现文件，包含所有工具函数的实现
└── README.md    # 说明文档
```

## 功能分类

### 1. 签名验证函数
- `verify_signature()` - 验证数字签名
  - 参数：数据、数据长度、签名密钥、签名
  - 返回：TEE_Result 类型的结果

### 2. 时间相关函数
- `get_trust_time()` - 获取可信时间戳
  - 返回：TEE_Time 类型的时间

### 3. 哈希计算函数
- `hash_data()` - 计算数据的 SHA256 哈希值
  - 参数：数据、数据长度、哈希输出缓冲区
  - 返回：TEE_Result 类型的结果

### 4. 仓库管理函数
- `repo_exists()` - 检查仓库是否存在
  - 参数：仓库ID
  - 返回：TEE_Result 类型的结果

### 5. TEE签名生成函数
- `generate_tee_signature()` - 生成TEE签名
  - 参数：哈希值、签名输出缓冲区
  - 功能：为区块生成TEE签名

### 6. 区块生成函数
- `get_access_block()` - 生成访问控制区块
  - 参数：仓库ID、操作类型、角色、公钥、签名密钥、签名、区块输出
  - 功能：生成访问控制相关的区块

- `get_contribution_block()` - 生成贡献区块
  - 参数：仓库ID、操作类型、分支、提交哈希、签名密钥、签名、区块输出
  - 功能：生成贡献相关的区块

## 使用示例

```c
#include "utils/utils.h"

// 验证签名
TEE_Result res = verify_signature(data, data_len, sigkey, signature);
if (res != TEE_SUCCESS) {
    // 处理错误
}

// 获取可信时间
TEE_Time time = get_trust_time();

// 计算哈希
char hash[65];
res = hash_data(data, data_len, hash);
if (res == TEE_SUCCESS) {
    // 使用哈希值
}

// 检查仓库是否存在
res = repo_exists(repo_id);
if (res == TEE_SUCCESS) {
    // 仓库存在
}

// 生成访问控制区块
struct block access_block;
res = get_access_block(repo_id, op, role, pubkey, sigkey, signature, &access_block);

// 生成贡献区块
struct block contrib_block;
res = get_contribution_block(repo_id, op, branch, commit_hash, sigkey, signature, &contrib_block);
```

## 依赖关系

### 内部依赖
- `key_list/key_list.h` - 密钥链表功能
- `trust_chain_ta.h` - 主要数据结构定义

### 外部依赖
- OP-TEE API (`tee_api_types.h`, `tee_internal_api.h`)
- 标准库 (`stdbool.h`, `stdint.h`, `stddef.h`)

## 编译

在 Makefile 中添加：
```makefile
SRCS += utils/utils.c
```

## 注意事项

1. 所有函数都遵循 OP-TEE 的编程规范
2. 内存管理使用 OP-TEE 的 TEE_Malloc/TEE_Free
3. 字符串操作使用 OP-TEE 的安全函数
4. 错误处理使用 TEE_Result 类型
5. 时间操作使用 OP-TEE 的 TEE_GetSystemTime
6. 哈希计算使用 OP-TEE 的 TEE_ALG_SHA256 算法

## 安全考虑

1. 签名验证函数目前是简化实现，实际使用时需要完整的签名验证逻辑
2. TEE签名生成函数目前是占位符实现，需要根据实际TEE环境实现
3. 所有字符串操作都使用OP-TEE的安全函数，避免缓冲区溢出
4. 哈希计算使用标准的SHA256算法，确保安全性 