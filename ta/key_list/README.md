# Key List 模块

这个模块包含了 trust_chain 项目中与密钥链表相关的所有功能。

## 文件结构

```
key_list/
├── key_list.h      # 头文件，包含数据结构和函数声明
├── key_list.c      # 实现文件，包含所有函数实现
└── README.md       # 说明文档
```

## 数据结构

### key_node
```c
struct key_node {
    char *key;                    /* 动态分配的密钥字符串 */
    struct key_node *next;        /* 指向下一个节点的指针 */
};
```

### key_list
```c
struct key_list {
    struct key_node *head;        /* 链表头指针 */
    uint32_t count;              /* 节点数量 */
};
```

## 函数接口

### 内存管理函数
- `init_key_list(struct key_list *key_list)` - 初始化密钥链表
- `cleanup_key_list(struct key_list *key_list)` - 清理密钥链表
- `copy_key_string(const char *src, char **dst)` - 复制密钥字符串
- `create_key_node(const char *key)` - 创建密钥节点
- `free_key_node(struct key_node *node)` - 释放密钥节点

### 链表操作函数
- `key_exists_in_set(const struct key_list *key_list, const char *key)` - 检查密钥是否存在
- `add_key_to_set(struct key_list *key_list, const char *key)` - 添加密钥到集合
- `remove_key_from_set(struct key_list *key_list, const char *key)` - 从集合中移除密钥

## 使用示例

```c
#include "key_list/key_list.h"

// 创建并初始化密钥链表
struct key_list admin_keys;
init_key_list(&admin_keys);

// 添加密钥
add_key_to_set(&admin_keys, "admin_key_1");
add_key_to_set(&admin_keys, "admin_key_2");

// 检查密钥是否存在
if (key_exists_in_set(&admin_keys, "admin_key_1")) {
    // 密钥存在
}

// 移除密钥
remove_key_from_set(&admin_keys, "admin_key_1");

// 清理链表
cleanup_key_list(&admin_keys);
```

## 编译

在 Makefile 中添加：
```makefile
SRCS += key_list/key_list.c
```

## 注意事项

1. 所有内存分配使用 OP-TEE 的 TEE_Malloc/TEE_Free
2. 字符串操作使用 OP-TEE 的 TEE_StrCmp/TEE_StrLen 等函数
3. 函数返回 TEE_Result 类型，遵循 OP-TEE 的错误码规范 