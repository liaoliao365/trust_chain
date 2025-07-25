/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#include "block.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <string.h>

/* 通用区块初始化函数 */
void init_base_block(struct base_block *base, 
                     uint32_t block_height,
                     const char *parent_hash,
                     uint32_t op,
                     const char *sigkey,
                     const char *signature) {
    if (base == NULL) return;
    
    base->block_height = block_height;
    if (parent_hash) {
        strcpy(base->parent_hash, parent_hash);
    } else {
        strcpy(base->parent_hash, "");
    }
    base->op = op;
    if (sigkey) {
        strcpy(base->sigkey, sigkey);
    } else {
        strcpy(base->sigkey, "");
    }
    if (signature) {
        strcpy(base->signature, signature);
    } else {
        strcpy(base->signature, "");
    }
    base->trust_timestamp = get_trust_time();
    strcpy(base->tee_sig, ""); // 初始化为空，等待TEE签名
}

/* Access区块初始化函数 */
void init_access_block(struct access_block *block,
                      uint32_t block_height,
                      const char *parent_hash,
                      uint32_t op,
                      uint32_t role,
                      const char *pubkey,
                      const char *sigkey,
                      const char *signature) {
    if (block == NULL) return;
    
    /* 初始化基础区块 */
    init_base_block(&block->base, block_height, parent_hash, op, sigkey, signature);
    
    /* 初始化Access区块特定字段 */
    block->role = role;
    if (pubkey) {
        strcpy(block->pubkey, pubkey);
    } else {
        strcpy(block->pubkey, "");
    }
}

/* Contribution区块初始化函数 */
void init_contribution_block(struct contribution_block *block,
                           uint32_t block_height,
                           const char *parent_hash,
                           uint32_t op,
                           const char *commit_hash,
                           const char *sigkey,
                           const char *signature) {
    if (block == NULL) return;
    
    /* 初始化基础区块 */
    init_base_block(&block->base, block_height, parent_hash, op, sigkey, signature);
    
    /* 初始化Contribution区块特定字段 */
    if (commit_hash) {
        strcpy(block->commit_hash, commit_hash);
    } else {
        strcpy(block->commit_hash, "");
    }
}

/* Access区块哈希计算函数 */
TEE_Result calculate_access_block_hash(const struct access_block *block, char *hash) {
    if (block == NULL || hash == NULL) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    
    char data_buffer[1024];
    
    /* 序列化Access区块的所有字段 */
    snprintf(data_buffer, sizeof(data_buffer),
             "%u:%.64s:%u:%.256s:%.256s:%u:%u:%u:%.256s", 
             block->base.block_height, 
             block->base.parent_hash, 
             block->base.op, 
             block->base.sigkey, 
             block->base.signature,
             block->base.trust_timestamp.seconds,
             block->base.trust_timestamp.millis,
             block->role,
             block->pubkey);
    
    /* 计算哈希 */
    return hash_data(data_buffer, strlen(data_buffer), hash);
}

/* Contribution区块哈希计算函数 */
TEE_Result calculate_contribution_block_hash(const struct contribution_block *block, char *hash) {
    if (block == NULL || hash == NULL) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    
    char data_buffer[1024];
    
    /* 序列化Contribution区块的所有字段 */
    snprintf(data_buffer, sizeof(data_buffer),
             "%u:%.64s:%u:%.256s:%.256s:%u:%u:%.64s", 
             block->base.block_height, 
             block->base.parent_hash, 
             block->base.op, 
             block->base.sigkey, 
             block->base.signature,
             block->base.trust_timestamp.seconds,
             block->base.trust_timestamp.millis,
             block->commit_hash);
    
    /* 计算哈希 */
    return hash_data(data_buffer, strlen(data_buffer), hash);
} 