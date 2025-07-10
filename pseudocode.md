# 可信链服务
可信链服务中，每个仓库都有一组逻辑上的元数据，包括当前区块高度BlockHeight，可信链最新区块哈希值LatestHash，管理员公钥集合AdminKeySet以及写权限者公钥集合WriterKeySet。假设唯一标识符为ID的仓库在可信链服务中的元数据数据结构可以通过MetaData[ID]定位到，则可信链服务的伪代码如下所示：

Access Control Service
```
Integer maxRepID = 0;//仓库ID的最大值，RepID则代表调用函数的当前仓库ID
Pk_TEE //TEE的公钥
struct MetaDataStruct {
    BlockHeight: Integer
    LatestHash: String
    AdminKeySet: Set of String
    WriterKeySet: Set of String
}
func Init (Pubkey) {//仓库创建时平台调用
    RepID = maxRepID++;
    MetaData[RepID] = MetaDataStruct(0, "", Set(), Set());
    //生成Access创世区块
    genesisBlock = BPS.getAccessBlock(RepID, ADD, Admin, Pubkey, Pk_TEE, null)
    Return RepID, genesisBlock, nil;
}

func Delete (<RepID, Op, Role, Pubkey>, SigKey, Signature) {//仓库删除时平台调用
    _, err = RepExisting(RepID);
    IF err:
        Return nil, err
    //要先检查一下是否有删除的权限！！！
    IF !KeyExisting(AdminKeySet, SigKey):
        Return nil, error("用户无管理员权限")
    MetaData[RepID] = null；
}
func RepExisting(RepID){
    IF RepID < 0 || RepID > maxRepID:
        Return nil, error("无效的仓库ID")； 
    IF !MetaData[RepID]:
        Return nil, error("该仓库已被删除")；
    Return true, nil
}

//RepID被操作仓库ID，Op访问控制操作，Role访问控制的角色，Pubkey被处理的用户的公钥，SigKey授权人的公钥，Signature授权人对<RepID, Op, Role, Pubkey>消息的签名
func AccessControl (<RepID, Op, Role, Pubkey>, SigKey, Signature) {
    _, err = RepExisting(RepID);
    IF err:
        Return nil, err
    //检查授权者是否有管理员权限
    AdminKeySet = MetaData[RepID].AdminKeySet;
    IF !KeyExisting(AdminKeySet, SigKey):
        Return nil, error("用户无管理员权限")；
    IF !VerifySig(<RepID, Op, Role, Pubkey>, SigKey, Signature):
        Return nil, error("签名不合法")；
        //授权
    WriterKeySet = MetaData[RepID].WriterKeySet;
    KeySet = (Role==Admin) ? AdminKeySet : WriterKeySet
    IF (Op=ADD):
        IF Existing(KeySet，Pubkey):
            Return nil, error("用户已在授权列表中，请勿重复授权")；
        ELSE IF (Role==Writer&&Existing(AdminKeySet，Pubkey)):
            Return nil, error("用户是Admin，具有Writer权限")；
        //添加Admin权限时要先删除其writer权限
        ELSE IF (Role==Admin&&Existing( WriterKeySet，Pubkey)):
            DeletefromSet(WriterKeySet，Pubkey)；
        AddtoList(KeySet，Pubkey);
    ELSE IF (Op=DELETE):
        IF Existing(KeySet，Pubkey):
            DeletefromSet(KeySet，Pubkey);
        ELSE 
            Return nil, error("用户不在列表中");
    //生成Access区块
    Block = BPS.getAccessBlock(RepID, Op, Role, Pubkey, SigKey, Signature)
    Return Block, nil;
}
```


Contribution Register Service
```
Func GetLatestHash (RepID, Nounce) {
    _, err = RepExisting(RepID);
    IF err:
        Return nil, err
    LatestHash = MetaData[RepID].LatestHash;
    IF (!LatestHash):
        Return nil, error("仓库不存在");
    Return <Nounce, LatestHash>TEE.pk, nil
}
Func Commit (Enc(Key)TEE.pk , <RepID, Op, CommitHash>, SigKey, Signature) {
    _, err = RepExisting(RepID);
    IF err:
        Return nil, err
    IF (Op=PUSH):
        //检查是否有写权限
        AdminKeySet = MetaData[RepID].AdminKeySet;
        WriterKeySet = MetaData[RepID].WriterKeySet;
        IF !Existing(AdminKeySet, SigKey) && !Existing(WriterKeySet, SigKey):
            Return nil, nil, error("用户无写权限")；
    IF !VerifySig（<RepID, Op, CommitHash>, SigKey, Signature):
        Return nil, nil, error("签名不合法")；
    //生成Contribution区块
    Block = BPS.getContributionBlock(RepID, Op, CommitHash, SigKey, Signature);
    Key = Dec(Enc(Key)TEE.pk)TEE.pk
    Return Block, Key, nil;
}
```

Block Package Service(私有服务，只有TEE能调用)
```
Func getAccessBlock(RepID, Op, Role, Pubkey, SigKey, Signature){
    Block = new PermissionBlock{
        BlockHeight: MetaData[RepID].BlockHeight++,
        Parent: MetaData[RepID].LatestHash,
        Op: Op, 
        Role: Role, 
        PubKey: Pubkey, 
        SigKey: SigKey, 
        Signature: Signature
    }
    MetaData[RepID].LatestHash = BlockHash = Hash(Block);
    Block.TEESig = Enc(BlockHash)Sk_TEE;
    Return Block;
} 
Func getContributionBlock(RepID, Op, Branch, CommitHash, SigKey, Signature){
    Block = new PermissionBlock{
        BlockHeight: MetaData[RepID].BlockHeight++,
        Parent: MetaData[RepID].LatestHash,
        Op: Op, 
        CommitHash: CommitHash, 
        TrustTimestamp: getTrustTime(), 
        SigKey: SigKey, 
        Signature: Signature
    }
    MetaData[RepID].LatestHash = BlockHash = Hash(Block);
    Block.TEESig = Enc(BlockHash)Sk_TEE;
    Return Block;
} 
```


VerifySig(<RepID, Op, Role, Pubkey>, SigKey, Signature)


func VerifySig(data, SigKey, Signature) {
    // 实现签名验证逻辑
    Return verify_signature(data, SigKey, Signature);
}

func getTrustTime() {
    // 获取可信时间戳
    Return current_timestamp();
}

func Hash(data) {
    // 计算哈希值
    Return sha256(data);
}
``` 