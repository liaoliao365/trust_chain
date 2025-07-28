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
// #include "trust_chain_constants.h"

/* mbedTLS 头文件 */
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/pem.h>
#include <mbedtls/base64.h>

/* 外部依赖 */
struct block;
struct repo_metadata;



/* 签名验证函数 */
TEE_Result verify_signature(const void *data, size_t data_len, 
                           const char *sigkey, const char *signature);

/* 通用验证函数，接受任何类型的密钥对象 */
TEE_Result verify_signature_common(const void *data, size_t data_len,
                                  TEE_ObjectHandle key_obj, 
                                  const char *signature);

/* 时间相关函数 */
TEE_Time get_trust_time(void);

/* 哈希计算函数 */
TEE_Result hash_data(const void *data, size_t data_len, char *hash);

/* 字节数组与十六进制字符串转换函数 */
void bytes_to_hex_string(const uint8_t *bytes, size_t len, char *hex_string);
TEE_Result hex_string_to_bytes(const char *hex_string, size_t hex_len, 
                               uint8_t *bytes, size_t *bytes_len);

/* 通用哈希计算函数 */
TEE_Result compute_sha256_hash(const void *data, size_t data_len, 
                               uint8_t *hash_buffer, size_t *hash_len);

/* 将 TEE 公钥对象转换为 PEM 格式 */
TEE_Result public_key_obj_to_pem(TEE_ObjectHandle key_obj, char *pem_buffer, size_t *pem_len);



#endif /* UTILS_H */ 