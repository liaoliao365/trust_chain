
代码文件树  
├── host/  
│   ├── main.c(将ca设计为一个守护进程，监听指定端口，供外部调用)  
│   └── Makefile copy(由于qemu中host使用cmake构建，因此不用这个Makefile)  
│  
├── ta/  
│   ├── user_ta_header_defines.h(定义了ta的一些参数，供外部调用)  
│   ├── include/  
│   │   └── trust_chain_ta.h(定义了ta的一些参数，供内部trust_chain_ta.c调用)  
│   ├── trust_chain_ta.c(ta的主要逻辑)  
│   ├── block/(区块模块，供ta调用)  
│   ├── key_list/(当前将每个仓库的管理员公钥集合和写权限者公钥集合分别用链表管理起来，方便增删，供ta调用，这个设计有点差劲)  
│   ├── tee_key_manager/(tee侧密钥的管理模块，包括签名，验证，解密等函数)  
│   ├── utils/(工具函数模块，包括获取时间，计算哈希，编解码函数)  
│   ├── Makefile  
│   └── sub.mk  
│  
└── CMakeLists.txt  
└── Makefile  
└── pseudocode.md(伪代码描述)  
└── README.md  
└── test_api.sh(AI生成的测试用例)  

哈希算法采用 SHA256  
非对称加密算法采用 RSA 2048  
公钥传入格式为 OpenSSH 格式  
公钥长度最大512  
签名长度最大512  

对外开放接口 
## init-repo

|输入字段|含义|  
|:---:|:--:|
|owner_key |创始人公钥 |  

|输出字段|含义|  
|:---:|:--:|
|repid | 仓库ID  |    
|tee_sig |tee签名，tee对genesis区块的签名|   

## access_control
|输入字段|含义|  
|:---:|:--:|
|repid | 仓库ID |  
|op | 操作|
|op_key | 操作者的公钥|
|authrized_key | 被授权者的公钥|
|role | 被授权的角色|
|signature | op_key对上述5个字段的签名|

|输出字段|含义|  
|:---:|:--:|  
|tee_sig |tee签名，tee对genesis区块的签名|   

## get_latest_hash
|输入字段|含义|  
|:---:|:--:|
|repid | 仓库ID  |
|nonce | 客户端发的随机数|

|输出字段|含义|  
|:---:|:--:|  
|<nonce, latesthash>TEE.pk |tee签名的<nonce, latesthash>字段|  


## commit
|输入字段|含义|  
|:---:|:--:|
|repid | 仓库ID  |
|op | 操作|
|op_key | 操作者的公钥|
|CommitHash | Commit对象的哈希值|
|signature | op_key对上述4个字段的签名|

|输出字段|含义|  
|:---:|:--:|  
|tee_sig |tee签名，tee对genesis区块的签名|  


## commit_enc_code（这个是代码加密版本的commit）
|输入字段|含义|  
|:---:|:--:|
|repid | 仓库ID  |
|op | 操作|
|op_key | 操作者的公钥|
|CommitHash | Commit对象的哈希值|
|Enc(Key)TEE.pk | 用tee公钥对密钥Key的加密值|
|signature | op_key对上述5个字段的签名|

|输出字段|含义|  
|:---:|:--:|  
|Key | 加密代码的对称密钥Key|
|tee_sig |tee签名，tee对genesis区块的签名|  


# 两种区块存储的格式
以下字节表示每个字段二进制格式的大小  

## access_block  
|字段|字节|含义|  
|:---:|:--:|:---:|
|parent_hash| 32 | 父区块的哈希值|
|op  |1  |操作类型，可以为ADD/DELETE|  
|op_key| 256 | 操作者的公钥，表明身份，RSA 2048|
|authrized_key| 256 | 被授权者的公钥|
|role| 1 | 被授权的角色|
|tee_time| 4 | tee的时间戳|
|tee_sig| 256 | tee的签名|

## contri_block  
|字段|字节|含义|  
|:---:|:--:|:---:|
|parent_hash| 32 | 父区块的哈希值|
|op  |1  |操作类型，可以为PUSH/PR|  
|op_key| 256 | 操作者的公钥，表明身份，RSA 2048|
|commit_hash| 32 | 提交贡献的哈希值|
|tee_time| 4 | tee的时间戳|
|tee_sig| 256 | tee的签名|

