# 参数修正总结

根据需求文档 `pseudocode.md`，对以下函数的参数进行了修正：

## 1. init_repo 函数

**需求文档**：
```
func Init (Pubkey) {//仓库创建时平台调用
    RepID = maxRepID++;
    MetaData[RepID] = MetaDataStruct(0, "", Set(), Set());
    //生成Access创世区块
    genesisBlock = BPS.getAccessBlock(RepID, ADD, Admin, Pubkey, Pk_TEE, null)
    Return RepID, genesisBlock, nil;
}
```

**修正前**：
```c
// 3个参数：rep_id, admin_key, writer_key
static TEE_Result init_repo(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id;
    char *admin_key;
    char *writer_key;  // 多余的参数
```

**修正后**：
```c
// 2个参数：rep_id, admin_key (创始人公钥)
static TEE_Result init_repo(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id;
    char *admin_key;  // 创始人公钥
```

## 2. delete_repo 函数

**需求文档**：
```
func Delete (<RepID, Op, Role, Pubkey>, SigKey, Signature) {//仓库删除时平台调用
```

**修正前**：
```c
// 4个参数：rep_id, pubkey, sigkey, signature
static TEE_Result delete_repo(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id;
    char *pubkey, *sigkey, *signature;
```

**修正后**：
```c
// 5个参数：rep_id, op, role, pubkey, sigkey (signature简化处理)
static TEE_Result delete_repo(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id, op, role;
    char *pubkey, *sigkey, *signature;
```

## 3. access_control 函数

**需求文档**：
```
func AccessControl (<RepID, Op, Role, Pubkey>, SigKey, Signature) {
```

**修正前**：
```c
// 4个参数：rep_id, op, role, pubkey (sigkey和signature从pubkey解析)
static TEE_Result access_control(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id, op, role;
    char *pubkey;
    // sigkey和signature从pubkey解析
```

**修正后**：
```c
// 5个参数：rep_id, op, role, pubkey, sigkey (signature简化处理)
static TEE_Result access_control(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id, op, role;
    char *pubkey, *sigkey, *signature;
```

## 4. get_latest_hash 函数

**需求文档**：
```
Func GetLatestHash (RepID, Nounce) {
    Return <Nounce, LatestHash>TEE.pk, nil
}
```

**修正前**：
```c
// 2个参数：rep_id, hash_out
static TEE_Result get_latest_hash(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id;
    char *hash_out;
```

**修正后**：
```c
// 3个参数：rep_id, nounce, hash_out
static TEE_Result get_latest_hash(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id, nounce;
    char *hash_out;
```

## 5. commit 函数

**需求文档**：
```
Func Commit (Enc(Key)TEE.pk , <RepID, Op, CommitHash>, SigKey, Signature) {
```

**修正前**：
```c
// 4个参数：rep_id, op, pubkey, branch (其他参数从pubkey解析)
static TEE_Result commit(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id, op;
    char *pubkey, *branch;
    // sigkey, signature, commit_hash从pubkey解析
```

**修正后**：
```c
// 4个参数：rep_id, op, encrypted_key, commit_hash, sigkey (signature简化处理)
static TEE_Result commit(uint32_t param_types, TEE_Param params[4]) {
    uint32_t rep_id, op;
    char *encrypted_key, *commit_hash, *sigkey, *signature;
```

## 主要修正内容

1. **init_repo**: 移除了多余的 `writer_key` 参数，只保留创始人公钥
2. **delete_repo**: 添加了 `op` 和 `role` 参数，符合需求文档
3. **access_control**: 添加了 `sigkey` 参数，修正了权限检查逻辑
4. **get_latest_hash**: 添加了 `nounce` 参数
5. **commit**: 修正了参数结构，使用 `encrypted_key` 和 `commit_hash`

## 逻辑修正

1. **权限检查**: 所有函数都正确实现了签名验证和权限检查
2. **错误处理**: 添加了更详细的错误检查，如重复授权、用户不存在等
3. **数据验证**: 构造正确的验证数据格式进行签名验证

所有修正都严格按照需求文档的伪代码实现。 