/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#include "utils.h"
#include <stdio.h>
#include <string.h>
#include "key_list/key_list.h"
#include "../tee_key_manager/tee_key_manager.h"
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/base64.h>
#include <mbedtls/platform_util.h>

/* 将 PEM 格式的公钥字符串转换为 TEE 公钥对象 */
static TEE_Result public_key_pem_to_obj(const char *key_str, TEE_ObjectHandle *key_obj);

/* 通用工具函数 */

/* 辅助函数：将字节数组转换为十六进制字符串 */
void bytes_to_hex_string(const uint8_t *bytes, size_t len, char *hex_string) {
    for (size_t i = 0; i < len; i++) {
        snprintf(hex_string + i * 2, 3, "%02x", bytes[i]);
    }
}

/* 辅助函数：将一个十六进制字符转换为整数 */
static int hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1; // 非法字符
}

/* 辅助函数：将十六进制字符串转换为字节数组 */
TEE_Result hex_string_to_bytes(const char *hex_string, size_t hex_len, 
                               uint8_t *bytes, size_t *bytes_len) {
    if (!hex_string || !bytes || !bytes_len)
        return TEE_ERROR_BAD_PARAMETERS;

    if (hex_len % 2 != 0)
        return TEE_ERROR_BAD_PARAMETERS;

    size_t output_len = hex_len / 2;
    for (size_t i = 0; i < output_len; i++) {
        int high = hex_char_to_int(hex_string[2 * i]);
        int low  = hex_char_to_int(hex_string[2 * i + 1]);
        if (high < 0 || low < 0)
            return TEE_ERROR_BAD_PARAMETERS;

        bytes[i] = (uint8_t)((high << 4) | low);
    }

    *bytes_len = output_len;
    return TEE_SUCCESS;
}

/* 内部哈希计算函数：计算SHA256哈希值 */
TEE_Result compute_sha256_hash(const void *data, size_t data_len, 
                               uint8_t *hash_buffer, size_t *hash_len) {
    TEE_OperationHandle hash_op = TEE_HANDLE_NULL;
    TEE_Result res;
    
    res = TEE_AllocateOperation(&hash_op, TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate hash operation: %x", res);
        return res;
    }
    
    res = TEE_DigestDoFinal(hash_op, (uint8_t *)data, data_len, hash_buffer, hash_len);
    TEE_FreeOperation(hash_op);
    
    if (res != TEE_SUCCESS) {
        EMSG("Failed to hash data: %x", res);
    }
    
    return res;
}

/* 时间相关函数 */
TEE_Time get_trust_time(void) {
	TEE_Time time;
	TEE_GetSystemTime(&time);
	return time;
}

/* 哈希计算函数 */
TEE_Result hash_data(const void *data, size_t data_len, char *hash) {
	uint8_t hash_buffer[32]; /* SHA256 hash size */
	size_t hash_len = sizeof(hash_buffer);
	TEE_Result res;
	
	/* 使用内部哈希计算函数 */
	res = compute_sha256_hash(data, data_len, hash_buffer, &hash_len);
	if (res == TEE_SUCCESS) {
		/* 将hash转换为十六进制字符串 */
		bytes_to_hex_string(hash_buffer, hash_len, hash);
	}
	
	return res;
}

/* 通用验证函数，接受任何类型的密钥对象 */
TEE_Result verify_signature_common(const void *data, size_t data_len,
                                  TEE_ObjectHandle key_obj, 
                                  const char *signature) {
    TEE_OperationHandle op = TEE_HANDLE_NULL;
    TEE_Result res;
    uint8_t sig_bytes[TEE_SIGNATURE_SIZE_BYTES];
    size_t sig_bytes_len;
    uint8_t data_hash[32]; /* SHA256 hash size */
    size_t hash_len = sizeof(data_hash);
    
    /* 参数检查 */
    if (!data || !signature || key_obj == TEE_HANDLE_NULL) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    
    /* 计算数据的哈希值 */
    res = compute_sha256_hash(data, data_len, data_hash, &hash_len);
    if (res != TEE_SUCCESS) {
        return res;
    }
    
    /* 将十六进制签名转换为字节数组 */
    res = hex_string_to_bytes(signature, strlen(signature), sig_bytes, &sig_bytes_len);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to convert signature from hex: %x", res);
        return res;
    }
    
    /* 创建验证操作 */
    res = TEE_AllocateOperation(&op, TEE_ALG_RSASSA_PKCS1_V1_5_SHA256, 
                               TEE_MODE_VERIFY, TEE_KEY_SIZE_BITS);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate verify operation: %x", res);
        goto cleanup;
    }
    
    /* 设置密钥并验证签名 */
    res = TEE_SetOperationKey(op, key_obj);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to set operation key: %x", res);
        goto cleanup;
    }
    
    res = TEE_AsymmetricVerifyDigest(op, NULL, 0, 
                                     data_hash, hash_len,
                                     sig_bytes, sig_bytes_len);
    if (res != TEE_SUCCESS) {
        EMSG("Signature verification failed: %x", res);
    }
    
cleanup:
    if (op != TEE_HANDLE_NULL)
        TEE_FreeOperation(op);
    return res;
}


/* 签名验证函数 */
TEE_Result verify_signature(const void *data, size_t data_len, 
                           const char *sigkey, const char *signature) {
    TEE_ObjectHandle key_obj = TEE_HANDLE_NULL;
    TEE_Result res;
    
    /* 参数检查 */
    if (!data || !sigkey || !signature) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
    
    /* 尝试从sigkey加载公钥 */
    res = public_key_pem_to_obj(sigkey, &key_obj);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to load public key: %x", res);
        return res;
    }
    
    /* 调用通用验证函数 */
    res = verify_signature_common(data, data_len, key_obj, signature);
    
    /* 清理资源 */
    if (key_obj != TEE_HANDLE_NULL)
        TEE_CloseObject(key_obj);
    
    return res;
}





/* 将 PEM 格式的公钥字符串转换为 TEE 公钥对象 */
static TEE_Result public_key_pem_to_obj(const char *key_str, TEE_ObjectHandle *key_obj) {
    TEE_Result res = TEE_SUCCESS;
    int mbedtls_res;

    // unsigned char der_buf[4096];
    // size_t der_len = 0;

    mbedtls_pk_context pk_ctx;
    mbedtls_pk_init(&pk_ctx);

    uint8_t *modulus_buf = NULL;
    uint8_t *exponent_buf = NULL;

    /* 使用 mbedtls_rsa_export 安全地导出 RSA 参数 */
    mbedtls_mpi N, E;
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&E);
    
    /* 使用 mbedtls_pem_read_buffer 解析 PEM -> DER */
    // mbedtls_pem_context pem_ctx;
    // mbedtls_pem_init(&pem_ctx);

    /* 解析 PEM -> DER */
    // mbedtls_res = mbedtls_pem_read_buffer(&pem_ctx, 
    //                                       "-----BEGIN PUBLIC KEY-----", 
    //                                       "-----END PUBLIC KEY-----",
    //                                       (const unsigned char *)key_str, 
    //                                       strlen(key_str), 
    //                                       der_buf, 
    //                                       sizeof(der_buf),
    //                                       &der_len);
    // if (mbedtls_res != 0) {
    //     EMSG("mbedtls_pem_read_buffer failed: -0x%x", -mbedtls_res);
    //     res = TEE_ERROR_BAD_FORMAT;
    //     goto cleanup;
    // }
    
    // /* 解析 DER 数据为公钥结构 */
    // mbedtls_res = mbedtls_pk_parse_public_key(&pk_ctx, der_buf, der_len);
    /* 解析 PEM 数据为公钥结构 */
    mbedtls_res = mbedtls_pk_parse_public_key(&pk_ctx, (const unsigned char *)key_str, strlen(key_str));
    if (mbedtls_res != 0) {
        EMSG("mbedtls_pk_parse_public_key failed: -0x%x", -mbedtls_res);
        res =  TEE_ERROR_BAD_FORMAT;
        goto cleanup;
    }
    
    /* 检查是否为 RSA 公钥 */
    if (mbedtls_pk_get_type(&pk_ctx) != MBEDTLS_PK_RSA) {
        EMSG("Public key is not RSA type");
        res =  TEE_ERROR_BAD_PARAMETERS;
        goto cleanup;
    }
    
    mbedtls_rsa_context *rsa_ctx = mbedtls_pk_rsa(pk_ctx);
    if (rsa_ctx == NULL) {
        EMSG("Failed to extract RSA context from public key");
        res = TEE_ERROR_BAD_PARAMETERS;
        goto cleanup;
    }

    size_t key_bits = mbedtls_pk_get_bitlen(&pk_ctx);

    
    /* 验证模数长度 */
    if (key_bits < 2048 || key_bits > 4096) {
        EMSG("Unsupported RSA key length: %zu, must be between 2048 and 4096", key_bits);
        res = TEE_ERROR_BAD_PARAMETERS;
        goto cleanup;
    }
    
    /* 创建公钥对象 */
    res = TEE_AllocateTransientObject(TEE_TYPE_RSA_PUBLIC_KEY, key_bits, key_obj);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to allocate RSA public key object");
        goto cleanup;
    }
    
    int export_res = mbedtls_rsa_export(rsa_ctx, &N, NULL, NULL, NULL, &E);
    if (export_res != 0) {
        EMSG("mbedtls_rsa_export failed: -0x%x", -export_res);
        res = TEE_ERROR_BAD_FORMAT;
        goto cleanup;
    }
    
    /* 获取模数和指数长度 */
    size_t modulus_len = mbedtls_mpi_size(&N);
    size_t exponent_len = mbedtls_mpi_size(&E);
    
    /* 分配缓冲区并导出模数和指数 */
    modulus_buf = TEE_Malloc(modulus_len, TEE_MALLOC_FILL_ZERO);
    exponent_buf = TEE_Malloc(exponent_len, TEE_MALLOC_FILL_ZERO);
    if (!modulus_buf || !exponent_buf) {
        res = TEE_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    

    if ((mbedtls_res = mbedtls_mpi_write_binary(&N, modulus_buf, modulus_len)) != 0 ||
    (mbedtls_res = mbedtls_mpi_write_binary(&E, exponent_buf, exponent_len)) != 0) {
    EMSG("Failed to write RSA modulus or exponent: -0x%x", -mbedtls_res);
    res = TEE_ERROR_GENERIC;
    goto cleanup;
}
    
    /* 设置公钥属性 */
    TEE_Attribute attrs[2];
    TEE_InitRefAttribute(&attrs[0], TEE_ATTR_RSA_MODULUS, modulus_buf, modulus_len);
    TEE_InitRefAttribute(&attrs[1], TEE_ATTR_RSA_PUBLIC_EXPONENT, exponent_buf, exponent_len);
    
    res = TEE_PopulateTransientObject(*key_obj, attrs, 2);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to populate transient object with RSA public key attributes");
        TEE_FreeTransientObject(*key_obj);
        *key_obj = TEE_HANDLE_NULL;
    }
    
cleanup:
    if (modulus_buf) TEE_Free(modulus_buf);
    if (exponent_buf) TEE_Free(exponent_buf);
    mbedtls_pk_free(&pk_ctx);
    /* 清理 MPI 对象 */
    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&E);
    // mbedtls_pem_free(&pem_ctx);
    return res;
}

/* 将 TEE 公钥对象转换为标准 PEM 格式（SubjectPublicKeyInfo） */
TEE_Result public_key_obj_to_pem(TEE_ObjectHandle key_obj, char *pem_buffer, size_t *pem_len) {
    TEE_Result res;
    uint8_t modulus[512];
    size_t mod_len = sizeof(modulus);
    uint8_t exponent[8];
    size_t exp_len = sizeof(exponent);

    if (!key_obj || !pem_buffer || !pem_len) {
        return TEE_ERROR_BAD_PARAMETERS;
    }

    /* 导出 modulus 和 exponent */
    res = TEE_GetObjectBufferAttribute(key_obj, TEE_ATTR_RSA_MODULUS, modulus, &mod_len);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to get modulus: 0x%x", res);
        return res;
    }

    res = TEE_GetObjectBufferAttribute(key_obj, TEE_ATTR_RSA_PUBLIC_EXPONENT, exponent, &exp_len);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to get exponent: 0x%x", res);
        return res;
    }

    /* 构建 mbedtls_rsa_context */
    mbedtls_rsa_context rsa;
    mbedtls_rsa_init(&rsa);
    if (mbedtls_rsa_import_raw(&rsa,
                                modulus, mod_len,
                                NULL, 0,
                                NULL, 0,
                                NULL, 0,
                                exponent, exp_len) != 0) {
        mbedtls_rsa_free(&rsa);
        return TEE_ERROR_GENERIC;
    }

    if (mbedtls_rsa_complete(&rsa) != 0) {
        mbedtls_rsa_free(&rsa);
        return TEE_ERROR_GENERIC;
    }

    /* 创建 mbedtls_pk_context 包装 rsa 公钥 */
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    if (mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0) {
        mbedtls_rsa_free(&rsa);
        mbedtls_pk_free(&pk);
        return TEE_ERROR_GENERIC;
    }
    mbedtls_rsa_copy(mbedtls_pk_rsa(pk), &rsa);

    /* DER encode 到临时缓冲区（逆序写入） */
    uint8_t der_buf[1024];
    int der_len = mbedtls_pk_write_pubkey_der(&pk, der_buf, sizeof(der_buf));
    if (der_len <= 0) {
        mbedtls_rsa_free(&rsa);
        mbedtls_pk_free(&pk);
        return TEE_ERROR_GENERIC;
    }

    /* 指向有效数据的起始地址 */
    uint8_t *der_start = der_buf + sizeof(der_buf) - der_len;

    /* Base64 编码 + PEM 封装 */
    const char *pem_header = "-----BEGIN PUBLIC KEY-----\n";
    const char *pem_footer = "-----END PUBLIC KEY-----\n";
    size_t b64_len = 0;
    size_t estimated_len = ((der_len + 2) / 3) * 4 + strlen(pem_header) + strlen(pem_footer) + 32;

    if (*pem_len < estimated_len) {
        *pem_len = estimated_len;
        mbedtls_rsa_free(&rsa);
        mbedtls_pk_free(&pk);
        return TEE_ERROR_SHORT_BUFFER;
    }

    char *out = pem_buffer;
    memcpy(out, pem_header, strlen(pem_header));
    out += strlen(pem_header);

    unsigned char b64_buf[1600];
    if (mbedtls_base64_encode(b64_buf, sizeof(b64_buf), &b64_len, der_start, der_len) != 0) {
        mbedtls_rsa_free(&rsa);
        mbedtls_pk_free(&pk);
        return TEE_ERROR_GENERIC;
    }

    /* 按 64 字符一行写入 */
    for (size_t i = 0; i < b64_len; i += 64) {
        size_t line_len = (i + 64 < b64_len) ? 64 : b64_len - i;
        TEE_MemMove(out, b64_buf + i, line_len);
        out += line_len;
        *out++ = '\n';
    }

    memcpy(out, pem_footer, strlen(pem_footer));
    out += strlen(pem_footer);
    *out = '\0';

    *pem_len = out - pem_buffer;

    mbedtls_rsa_free(&rsa);
    mbedtls_pk_free(&pk);
    return TEE_SUCCESS;
}
