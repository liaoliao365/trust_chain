/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#ifndef BLOCK_H
#define BLOCK_H

#include <tee_api_types.h>
#include <tee_internal_api.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "../include/trust_chain_constants.h"

/* 基础区块结构体 */
struct base_block {
    uint32_t block_height;           // 区块高度
    char parent_hash[MAX_HASH_LENGTH]; // 父区块哈希
    uint32_t op;                     // 操作类型
    char sigkey[MAX_KEY_LENGTH];     // 签名密钥
    char signature[MAX_SIGNATURE_LENGTH]; // 签名
    TEE_Time trust_timestamp;        // 可信时间戳
    char tee_sig[MAX_SIGNATURE_LENGTH];   // TEE签名
};

/* Access区块结构体 */
struct access_block {
    struct base_block base;          // 继承基础区块
    uint32_t role;                   // 角色类型
    char pubkey[MAX_KEY_LENGTH];     // 公钥
};

/* Contribution区块结构体 */
struct contribution_block {
    struct base_block base;          // 继承基础区块
    char commit_hash[MAX_HASH_LENGTH]; // 提交哈希
};

/* 通用区块初始化函数 */
void init_base_block(struct base_block *base, 
                     uint32_t block_height,
                     const char *parent_hash,
                     uint32_t op,
                     const char *sigkey,
                     const char *signature);

/* Access区块初始化函数 */
void init_access_block(struct access_block *block,
                      uint32_t block_height,
                      const char *parent_hash,
                      uint32_t op,
                      uint32_t role,
                      const char *pubkey,
                      const char *sigkey,
                      const char *signature);

/* Contribution区块初始化函数 */
void init_contribution_block(struct contribution_block *block,
                           uint32_t block_height,
                           const char *parent_hash,
                           uint32_t op,
                           const char *commit_hash,
                           const char *sigkey,
                           const char *signature);

/* 哈希计算函数 */
TEE_Result calculate_access_block_hash(const struct access_block *block, char *hash);
TEE_Result calculate_contribution_block_hash(const struct contribution_block *block, char *hash);

#endif /* BLOCK_H */ 