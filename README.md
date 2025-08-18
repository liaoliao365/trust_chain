
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