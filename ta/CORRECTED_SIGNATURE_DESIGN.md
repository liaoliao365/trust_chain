# 修正后的签名设计

## 问题分析

之前的实现错误地将 `signature` 从 `sigkey` 字符串中解析出来，这是不正确的。实际上：

- `sigkey` 和 `signature` 是两个完全独立的参数
- `signature` 是 `sigkey` 对消息的签名
- 需要分别传递这两个参数

## 正确的参数设计

### access_control 函数

**需求文档**：
```
func AccessControl (<RepID, Op, Role, Pubkey>, SigKey, Signature)
```

**正确的参数分配**：
```c
static TEE_Result access_control(uint32_t param_types, TEE_Param params[4]) {
    // params[0].memref.buffer: access_control_message 结构体
    // params[1].memref.buffer: sigkey (授权人的公钥)
    // params[2].memref.buffer: signature (sigkey对消息的签名)
    // params[3]: 未使用
}
```

**参数解析**：
```c
ac_msg = (struct access_control_message *)params[0].memref.buffer;
sigkey = (char *)params[1].memref.buffer;
signature = (char *)params[2].memref.buffer;
```

### commit 函数

**需求文档**：
```
Func Commit (Enc(Key)TEE.pk, <RepID, Op, CommitHash>, SigKey, Signature)
```

**正确的参数分配**：
```c
static TEE_Result commit(uint32_t param_types, TEE_Param params[4]) {
    // params[0].memref.buffer: commit_message 结构体
    // params[1].memref.buffer: encrypted_key
    // params[2].memref.buffer: sigkey (授权人的公钥)
    // params[3].memref.buffer: signature (sigkey对消息的签名)
}
```

**参数解析**：
```c
cm_msg = (struct commit_message *)params[0].memref.buffer;
encrypted_key = (char *)params[1].memref.buffer;
sigkey = (char *)params[2].memref.buffer;
signature = (char *)params[3].memref.buffer;
```

## 签名验证流程

### access_control 签名验证

1. **构造验证数据**：
   ```c
   char data_to_verify[1024];
   TEE_Snprintf(data_to_verify, sizeof(data_to_verify), 
                "%u:%u:%u:%s", ac_msg->rep_id, ac_msg->op, ac_msg->role, ac_msg->pubkey);
   ```

2. **验证签名**：
   ```c
   res = verify_signature(data_to_verify, TEE_StrLen(data_to_verify), sigkey, signature);
   ```

### commit 签名验证

1. **构造验证数据**：
   ```c
   char data_to_verify[1024];
   TEE_Snprintf(data_to_verify, sizeof(data_to_verify), 
                "%u:%u:%s", cm_msg->rep_id, cm_msg->op, cm_msg->commit_hash);
   ```

2. **验证签名**：
   ```c
   res = verify_signature(data_to_verify, TEE_StrLen(data_to_verify), sigkey, signature);
   ```

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

## 修正的关键点

1. **独立参数**：`sigkey` 和 `signature` 现在是完全独立的参数
2. **正确验证**：`signature` 是 `sigkey` 对消息的签名，需要正确验证
3. **清晰语义**：参数含义更加明确，符合需求文档
4. **类型安全**：避免了字符串解析的潜在错误

## 总结

修正后的设计：
- ✅ `sigkey` 和 `signature` 是独立参数
- ✅ `signature` 是 `sigkey` 对消息的签名
- ✅ 正确的签名验证流程
- ✅ 符合OP-TEE参数限制
- ✅ 语义清晰，易于理解和维护 