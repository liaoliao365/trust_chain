/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#ifndef TEE_KEY_MANAGER_H
#define TEE_KEY_MANAGER_H

#include <tee_api_types.h>
#include <stdbool.h>

/* TEE密钥管理相关常量 */
#define TEE_KEY_SIZE_BITS 2048
#define TEE_SIGNATURE_SIZE_BYTES 256

/* TEE密钥UUID */
extern const TEE_UUID tee_key_pair_uuid;

/* 简化的TEE密钥管理函数 */

/**
 * 使用TEE私钥对数据进行签名
 * @param data 要签名的数据
 * @param signature 输出参数，签名结果（十六进制字符串）
 * @return TEE_SUCCESS 成功，其他值表示错误
 */
TEE_Result tee_sign_data(const char *data, char *signature);

/**
 * 使用TEE私钥对哈希值进行签名（不重复计算哈希）
 * @param hash_string 十六进制哈希字符串
 * @param signature 输出参数，签名结果（十六进制字符串）
 * @return TEE_SUCCESS 成功，其他值表示错误
 */
TEE_Result tee_sign_hash(const char *hash_string, char *signature);

/**
 * 使用TEE公钥验证签名
 * @param data 原始数据
 * @param data_len 数据长度
 * @param signature 签名（十六进制字符串）
 * @param sig_len 签名长度
 * @return TEE_SUCCESS 验证成功，其他值表示验证失败
 */
TEE_Result tee_verify_signature(const void *data, size_t data_len,
                                const char *signature);

/**
 * 使用TEE私钥解密数据
 * @param encrypted_data 加密的数据
 * @param encrypted_len 加密数据长度
 * @param decrypted_data 输出参数，解密后的数据
 * @param decrypted_len 输出参数，解密后数据长度
 * @return TEE_SUCCESS 成功，其他值表示错误
 */
TEE_Result tee_decrypt_data(const char *encrypted_data, size_t encrypted_len,
                           char *decrypted_data, size_t *decrypted_len);

#endif /* TEE_KEY_MANAGER_H */ 