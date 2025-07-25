#!/bin/bash

# TEE密钥管理功能测试脚本
# 用于验证TEE密钥生成、存储和签名功能

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查OP-TEE环境
check_optee_env() {
    log_info "检查OP-TEE环境..."
    
    if [ ! -d "/dev/tee0" ]; then
        log_error "OP-TEE设备未找到，请确保OP-TEE已正确安装"
        exit 1
    fi
    
    if ! command -v tee-supplicant &> /dev/null; then
        log_error "tee-supplicant未找到，请确保OP-TEE客户端已安装"
        exit 1
    fi
    
    log_info "OP-TEE环境检查通过"
}

# 编译TA
compile_ta() {
    log_info "编译Trust Chain TA..."
    
    cd optee_examples/trust_chain/ta
    
    if [ ! -f "Makefile" ]; then
        log_error "Makefile未找到"
        exit 1
    fi
    
    make clean
    make
    
    if [ ! -f "8aaaf201-2450-11e4-abe2-0002a5d5c51b.ta" ]; then
        log_error "TA编译失败"
        exit 1
    fi
    
    log_info "TA编译成功"
    cd ../../..
}

# 安装TA
install_ta() {
    log_info "安装Trust Chain TA..."
    
    # 停止tee-supplicant（如果正在运行）
    pkill tee-supplicant || true
    
    # 启动tee-supplicant
    tee-supplicant &
    TEE_SUPPLICANT_PID=$!
    
    # 等待tee-supplicant启动
    sleep 2
    
    # 安装TA
    cp optee_examples/trust_chain/ta/8aaaf201-2450-11e4-abe2-0002a5d5c51b.ta /lib/optee_armtz/
    
    log_info "TA安装完成"
}

# 创建测试程序
create_test_program() {
    log_info "创建TEE密钥管理测试程序..."
    
    cat > test_tee_key_manager.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tee_client_api.h>

#define TA_TRUST_CHAIN_UUID { 0x8aaaf201, 0x2450, 0x11e4, \
    { 0xab, 0xe2, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b} }

#define TA_TRUST_CHAIN_CMD_GET_TEE_PUBKEY 5

int main() {
    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Result res;
    TEEC_UUID ta_uuid = TA_TRUST_CHAIN_UUID;
    
    printf("=== TEE密钥管理功能测试 ===\n");
    
    // 初始化TEE上下文
    res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS) {
        printf("TEEC_InitializeContext failed: %x\n", res);
        return 1;
    }
    printf("✓ TEE上下文初始化成功\n");
    
    // 打开会话
    res = TEEC_OpenSession(&ctx, &sess, &ta_uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
    if (res != TEEC_SUCCESS) {
        printf("TEEC_OpenSession failed: %x\n", res);
        TEEC_FinalizeContext(&ctx);
        return 1;
    }
    printf("✓ TEE会话打开成功\n");
    
    // 测试1: 获取TEE公钥
    printf("\n--- 测试1: 获取TEE公钥 ---\n");
    
    char public_key_pem[1024];
    size_t pem_len = sizeof(public_key_pem);
    TEEC_Param params[4];
    
    params[0].tmpref.buffer = public_key_pem;
    params[0].tmpref.size = sizeof(public_key_pem);
    params[1].value.a = pem_len;
    params[2].value.a = 0;
    params[3].value.a = 0;
    
    res = TEEC_InvokeCommand(&sess, TA_TRUST_CHAIN_CMD_GET_TEE_PUBKEY, 
                            TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                            TEEC_VALUE_OUTPUT,
                                            TEEC_NONE,
                                            TEEC_NONE),
                            params, NULL);
    
    if (res != TEEC_SUCCESS) {
        printf("获取TEE公钥失败: %x\n", res);
    } else {
        printf("✓ TEE公钥获取成功\n");
        printf("公钥长度: %zu\n", params[1].value.a);
        printf("公钥内容:\n%s\n", public_key_pem);
    }
    
    // 测试2: 测试仓库初始化（这会触发TEE密钥生成）
    printf("\n--- 测试2: 测试仓库初始化 ---\n");
    
    char admin_key[] = "test_admin_key_12345";
    uint32_t rep_id;
    char genesis_block[1024];
    
    params[0].tmpref.buffer = admin_key;
    params[0].tmpref.size = strlen(admin_key);
    params[1].value.a = 0;
    params[2].tmpref.buffer = genesis_block;
    params[2].tmpref.size = sizeof(genesis_block);
    params[3].value.a = 0;
    
    res = TEEC_InvokeCommand(&sess, 0, /* TA_TRUST_CHAIN_CMD_INIT_REPO */
                            TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                                            TEEC_VALUE_OUTPUT,
                                            TEEC_MEMREF_TEMP_OUTPUT,
                                            TEEC_NONE),
                            params, NULL);
    
    if (res != TEEC_SUCCESS) {
        printf("仓库初始化失败: %x\n", res);
    } else {
        printf("✓ 仓库初始化成功\n");
        printf("仓库ID: %u\n", params[1].value.a);
        rep_id = params[1].value.a;
    }
    
    // 再次获取公钥，确认密钥已生成
    printf("\n--- 测试3: 再次获取TEE公钥（确认密钥已生成） ---\n");
    
    memset(public_key_pem, 0, sizeof(public_key_pem));
    params[0].tmpref.buffer = public_key_pem;
    params[0].tmpref.size = sizeof(public_key_pem);
    params[1].value.a = pem_len;
    
    res = TEEC_InvokeCommand(&sess, TA_TRUST_CHAIN_CMD_GET_TEE_PUBKEY, 
                            TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT,
                                            TEEC_VALUE_OUTPUT,
                                            TEEC_NONE,
                                            TEEC_NONE),
                            params, NULL);
    
    if (res != TEEC_SUCCESS) {
        printf("再次获取TEE公钥失败: %x\n", res);
    } else {
        printf("✓ TEE公钥再次获取成功\n");
        printf("公钥长度: %zu\n", params[1].value.a);
        if (strstr(public_key_pem, "TEE_PUBLIC_KEY_PLACEHOLDER") != NULL) {
            printf("⚠️  注意: 当前返回的是占位符公钥，实际实现中应该返回真实的PEM格式公钥\n");
    printf("简化方案: 使用单一UUID管理整个密钥对，代码更简洁\n");
        }
    }
    
    // 清理
    TEEC_CloseSession(&sess);
    TEEC_FinalizeContext(&ctx);
    
    printf("\n=== 测试完成 ===\n");
    return 0;
}
EOF

    # 编译测试程序
    gcc -o test_tee_key_manager test_tee_key_manager.c -lteec
    
    log_info "测试程序创建完成"
}

# 运行测试
run_test() {
    log_info "运行TEE密钥管理测试..."
    
    if [ ! -f "test_tee_key_manager" ]; then
        log_error "测试程序未找到，请先创建测试程序"
        exit 1
    fi
    
    ./test_tee_key_manager
    
    if [ $? -eq 0 ]; then
        log_info "测试完成"
    else
        log_error "测试失败"
        exit 1
    fi
}

# 清理函数
cleanup() {
    log_info "清理资源..."
    
    # 停止tee-supplicant
    if [ ! -z "$TEE_SUPPLICANT_PID" ]; then
        kill $TEE_SUPPLICANT_PID 2>/dev/null || true
    fi
    
    # 清理测试文件
    rm -f test_tee_key_manager.c test_tee_key_manager
    
    log_info "清理完成"
}

# 主函数
main() {
    log_info "开始TEE密钥管理功能测试"
    
    # 设置清理陷阱
    trap cleanup EXIT
    
    check_optee_env
    compile_ta
    install_ta
    create_test_program
    run_test
    
    log_info "所有测试完成"
}

# 运行主函数
main "$@" 