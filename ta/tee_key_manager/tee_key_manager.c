/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#include "tee_key_manager.h"
#include "../utils/utils.h"
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <string.h>

/* 简化的TEE密钥UUID */
const TEE_UUID tee_key_pair_uuid = {
    0x12345678, 0x1234, 0x1234, {0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12}
};

/* 内部辅助函数 */

/**
 * 加载密钥对，如果不存在则生成新的
 */
static TEE_Result load_or_generate_key_pair(TEE_ObjectHandle *key_pair) {
    TEE_Result res = TEE_SUCCESS;
    
    if (!key_pair) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    
    /* 尝试加载现有密钥对 */
    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, &tee_key_pair_uuid, 
                                   sizeof(tee_key_pair_uuid),
                                   TEE_DATA_FLAG_ACCESS_READ, key_pair);
    if (res == TEE_SUCCESS) {
        return TEE_SUCCESS;
    }
    
    /* 密钥不存在，生成新的密钥对 */
    TEE_ObjectHandle rsa_keypair = TEE_HANDLE_NULL;
    TEE_ObjectHandle persistent_key_pair = TEE_HANDLE_NULL;

    // 分配一个临时的 RSA 密钥对对象
    res = TEE_AllocateTransientObject(TEE_TYPE_RSA_KEYPAIR, TEE_KEY_SIZE_BITS, &rsa_keypair);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate transient key object: %x", res);
        return res;
    }
    
    /* 在该对象上生成密钥对 */
    res = TEE_GenerateKey(rsa_keypair, TEE_KEY_SIZE_BITS, NULL, 0);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to generate RSA key pair: %x", res);
        TEE_FreeTransientObject(rsa_keypair);
        return res;
    }
    
    /* 创建一个持久化对象用于存储密钥对 */
    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE, &tee_key_pair_uuid, 
                                     sizeof(tee_key_pair_uuid),
                                     TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE | 
                                     TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_OVERWRITE,
                                     rsa_keypair, 0, 0, &persistent_key_pair);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate persistent object: %x", res);
        TEE_FreeTransientObject(rsa_keypair);
        return res;
    }
    
    IMSG("TEE key pair generated and saved successfully");
    
    TEE_CloseObject(persistent_key_pair);
    TEE_FreeTransientObject(rsa_keypair);
    
    /* 重新加载密钥对 */
    return TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, &tee_key_pair_uuid, 
                                   sizeof(tee_key_pair_uuid),
                                   TEE_DATA_FLAG_ACCESS_READ, key_pair);
}



/* 简化的公共接口实现 */

TEE_Result tee_sign_data(const char *data, char *signature) {
    if (!data || !signature) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    char hash_string[65] = {0}; // 32字节SHA256哈希，十六进制字符串+结尾符
    TEE_Result res = hash_data(data, strlen(data), hash_string);
    if (res != TEE_SUCCESS) {
        return res;
    }
    return tee_sign_hash(hash_string, signature);
}

/* 签名哈希值（不重复计算哈希） */
TEE_Result tee_sign_hash(const char *hash_string, char *signature) {
    TEE_ObjectHandle key_pair = TEE_HANDLE_NULL;
    TEE_OperationHandle op = TEE_HANDLE_NULL;
    TEE_Result res;
    uint8_t sig_buffer[TEE_SIGNATURE_SIZE_BYTES];
    size_t actual_sig_len = sizeof(sig_buffer);
    uint8_t hash_bytes[32]; /* SHA256 hash size */
    size_t hash_len = sizeof(hash_bytes);
    
    if (!hash_string || !signature) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    
    /* 将十六进制哈希字符串转换为字节数组 */
    res = hex_string_to_bytes(hash_string, strlen(hash_string), hash_bytes, &hash_len);
    if (res != TEE_SUCCESS) {
        return res;
    }
    
    /* 加载或生成密钥对 */
    res = load_or_generate_key_pair(&key_pair);
    if (res != TEE_SUCCESS) {
        goto cleanup;
    }
    
    /* 创建签名操作 */
    res = TEE_AllocateOperation(&op, TEE_ALG_RSASSA_PKCS1_V1_5_SHA256, 
                               TEE_MODE_SIGN, TEE_KEY_SIZE_BITS);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate signature operation: %x", res);
        goto cleanup;
    }
    
    /* 设置密钥对并执行签名 */
    res = TEE_SetOperationKey(op, key_pair);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to set operation key: %x", res);
        goto cleanup;
    }
    
    res = TEE_AsymmetricSignDigest(op, NULL, 0, hash_bytes, hash_len,
                                   sig_buffer, &actual_sig_len);
    if (res == TEE_SUCCESS) {
        /* 转换为十六进制字符串 */
        bytes_to_hex_string(sig_buffer, actual_sig_len, signature);
    }
    
cleanup:
    if (op != TEE_HANDLE_NULL)
        TEE_FreeOperation(op);
    if (key_pair != TEE_HANDLE_NULL)
        TEE_CloseObject(key_pair);
    return res;
}

TEE_Result tee_verify_signature(const void *data, size_t data_len,
                                const char *signature) {
    TEE_ObjectHandle key_pair = TEE_HANDLE_NULL;
    TEE_ObjectHandle public_key = TEE_HANDLE_NULL;
    TEE_Result res;
    
    if (!data || !signature) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    
    /* 加载TEE密钥对 */
    res = load_or_generate_key_pair(&key_pair);
    if (res != TEE_SUCCESS) {
        return res;
    }
    
    /* 从密钥对中提取公钥 */
    res = TEE_AllocateTransientObject(TEE_TYPE_RSA_PUBLIC_KEY, TEE_KEY_SIZE_BITS, &public_key);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate public key object: %x", res);
        goto cleanup;
    }
    
    /* 复制公钥属性 */
    res = TEE_CopyObjectAttributes1(public_key, key_pair);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to copy public key attributes: %x", res);
        goto cleanup;
    }
    
    /* 调用通用验证函数，使用公钥 */
    res = verify_signature_common(data, data_len, public_key, signature);
    
cleanup:
    /* 清理资源 */
    if (public_key != TEE_HANDLE_NULL)
        TEE_FreeTransientObject(public_key);
    if (key_pair != TEE_HANDLE_NULL)
        TEE_CloseObject(key_pair);
    
    return res;
}

TEE_Result tee_decrypt_data(const char *encrypted_data, size_t encrypted_len,
                           char *decrypted_data, size_t *decrypted_len) {
    TEE_ObjectHandle key_pair = TEE_HANDLE_NULL;
    TEE_OperationHandle op = TEE_HANDLE_NULL;
    TEE_Result res;
    uint8_t encrypted_bytes[TEE_KEY_SIZE_BITS / 8];
    size_t encrypted_bytes_len = sizeof(encrypted_bytes);
    uint8_t decrypted_bytes[TEE_KEY_SIZE_BITS / 8];
    size_t decrypted_bytes_len = sizeof(decrypted_bytes);
    
    if (!encrypted_data || !decrypted_data || !decrypted_len) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    
    /* 将十六进制加密数据转换为字节数组 */
    res = hex_string_to_bytes(encrypted_data, encrypted_len, encrypted_bytes, &encrypted_bytes_len);
    if (res != TEE_SUCCESS) {
        return res;
    }
    
    /* 加载或生成密钥对 */
    res = load_or_generate_key_pair(&key_pair);
    if (res != TEE_SUCCESS) {
        goto cleanup;
    }
    
    /* 创建解密操作 */
    res = TEE_AllocateOperation(&op, TEE_ALG_RSAES_PKCS1_V1_5, 
                               TEE_MODE_DECRYPT, TEE_KEY_SIZE_BITS);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate decrypt operation: %x", res);
        goto cleanup;
    }
    
    /* 设置密钥对并执行解密 */
    res = TEE_SetOperationKey(op, key_pair);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to set operation key: %x", res);
        goto cleanup;
    }
    
    res = TEE_AsymmetricDecrypt(op, NULL, 0, encrypted_bytes, encrypted_bytes_len,
                                decrypted_bytes, &decrypted_bytes_len);
    if (res == TEE_SUCCESS) {
        /* 将解密后的字节转换为十六进制字符串，保持与加密时的一致性 */
        bytes_to_hex_string(decrypted_bytes, decrypted_bytes_len, decrypted_data);
        *decrypted_len = decrypted_bytes_len * 2; /* 十六进制字符串长度是字节数的2倍 */
    }
    
cleanup:
    if (op != TEE_HANDLE_NULL)
        TEE_FreeOperation(op);
    if (key_pair != TEE_HANDLE_NULL)
        TEE_CloseObject(key_pair);
    return res;
} 