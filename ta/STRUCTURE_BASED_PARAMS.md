# 基于结构体的参数设计

## 设计理念

为了解决OP-TEE TA接口只有4个参数槽位的限制，同时保持参数语义清晰，我们采用了基于结构体的参数设计。

## 结构体定义

### 1. access_control_message 结构体

```c
struct access_control_message {
    uint32_t rep_id;      // 仓库ID
    uint32_t op;          // 操作类型 (OP_ADD/OP_DELETE)
    uint32_t role;        // 角色 (ROLE_ADMIN/ROLE_WRITER)
    char pubkey[MAX_KEY_LENGTH];  // 被处理的用户公钥
};
```

**对应需求文档**：
```
func AccessControl (<RepID, Op, Role, Pubkey>, SigKey, Signature)
```

### 2. commit_message 结构体

```c
struct commit_message {
    uint32_t rep_id;      // 仓库ID
    uint32_t op;          // 操作类型 (OP_PUSH)
    char commit_hash[MAX_HASH_LENGTH];  // 提交哈希
};
```

**对应需求文档**：
```
Func Commit (Enc(Key)TEE.pk, <RepID, Op, CommitHash>, SigKey, Signature)
```

## 参数分配方案

### access_control 函数

```c
static TEE_Result access_control(uint32_t param_types, TEE_Param params[4]) {
    // params[0].memref.buffer: access_control_message 结构体
    // params[1].memref.buffer: sigkey (授权人的公钥)
    // params[2].memref.buffer: signature (对消息的签名)
    // params[3]: 未使用
}
```

**参数解析**：
- `params[0]`: 包含 `<RepID, Op, Role, Pubkey>` 的结构体
- `params[1]`: `sigkey` (授权人的公钥)
- `params[2]`: `signature` (sigkey对消息的签名)

### commit 函数

```c
static TEE_Result commit(uint32_t param_types, TEE_Param params[4]) {
    // params[0].memref.buffer: commit_message 结构体
    // params[1].memref.buffer: encrypted_key
    // params[2].memref.buffer: sigkey (授权人的公钥)
    // params[3].memref.buffer: signature (对消息的签名)
}
```

**参数解析**：
- `params[0]`: 包含 `<RepID, Op, CommitHash>` 的结构体
- `params[1]`: `Enc(Key)TEE.pk` 加密密钥
- `params[2]`: `sigkey` (授权人的公钥)
- `params[3]`: `signature` (sigkey对消息的签名)

## 签名验证

### access_control 签名验证

```c
// 构造验证数据: <RepID, Op, Role, Pubkey>
char data_to_verify[1024];
TEE_Snprintf(data_to_verify, sizeof(data_to_verify), 
             "%u:%u:%u:%s", ac_msg->rep_id, ac_msg->op, ac_msg->role, ac_msg->pubkey);

// 验证签名
res = verify_signature(data_to_verify, TEE_StrLen(data_to_verify), sigkey, signature);
```

### commit 签名验证

```c
// 构造验证数据: <RepID, Op, CommitHash>
char data_to_verify[1024];
TEE_Snprintf(data_to_verify, sizeof(data_to_verify), 
             "%u:%u:%s", cm_msg->rep_id, cm_msg->op, cm_msg->commit_hash);

// 验证签名
res = verify_signature(data_to_verify, TEE_StrLen(data_to_verify), sigkey, signature);
```

## 优势

1. **语义清晰**：结构体字段名明确表达了参数含义
2. **类型安全**：编译时检查参数类型
3. **易于扩展**：可以轻松添加新字段而不影响接口
4. **符合OP-TEE限制**：在4个参数槽位内传递所有必要信息
5. **便于调试**：结构体便于调试和日志记录

## 客户端调用示例

### access_control 调用

```c
struct access_control_message ac_msg = {
    .rep_id = 1,
    .op = OP_ADD,
    .role = ROLE_ADMIN,
    .pubkey = "admin_public_key"
};

char sigkey[] = "authorizer_public_key";
char signature[] = "signature_data";

TEE_Param params[4] = {
    {.memref = {.buffer = &ac_msg, .size = sizeof(ac_msg)}},
    {.memref = {.buffer = sigkey, .size = strlen(sigkey)}},
    {.memref = {.buffer = signature, .size = strlen(signature)}},
    {.a = 0}
};
```

### commit 调用

```c
struct commit_message cm_msg = {
    .rep_id = 1,
    .op = OP_PUSH,
    .commit_hash = "commit_hash_value"
};

char encrypted_key[] = "encrypted_key_data";
char sigkey[] = "authorizer_public_key";
char signature[] = "signature_data";

TEE_Param params[4] = {
    {.memref = {.buffer = &cm_msg, .size = sizeof(cm_msg)}},
    {.memref = {.buffer = encrypted_key, .size = strlen(encrypted_key)}},
    {.memref = {.buffer = sigkey, .size = strlen(sigkey)}},
    {.memref = {.buffer = signature, .size = strlen(signature)}}
};
```

## 总结

这种基于结构体的参数设计既满足了OP-TEE的技术限制，又保持了代码的清晰性和可维护性。通过结构体封装相关参数，使得函数接口更加语义化，同时为未来的扩展留下了空间。 