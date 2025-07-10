/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#ifndef UTILS_H
#define UTILS_H

#include <tee_api_types.h>
#include <tee_internal_api.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "trust_chain_constants.h"

/* 外部依赖 */
struct block;
struct repo_metadata;



/* 签名验证函数 */
TEE_Result verify_signature(const void *data, size_t data_len, 
                           const char *sigkey, const char *signature);

/* 时间相关函数 */
TEE_Time get_trust_time(void);

/* 哈希计算函数 */
TEE_Result hash_data(const void *data, size_t data_len, char *hash);



/* TEE签名生成函数 */
void generate_tee_signature(const char *hash, char *tee_sig);

/* 区块生成函数 */
TEE_Result get_access_block(const struct repo_metadata *repo, uint32_t op, uint32_t role,
                           const char *pubkey, const char *sigkey, 
                           const char *signature, struct block *block);

TEE_Result get_contribution_block(const struct repo_metadata *repo, uint32_t op,
                                 const char *branch, const char *commit_hash,
                                 const char *sigkey, const char *signature,
                                 struct block *block);

#endif /* UTILS_H */ 