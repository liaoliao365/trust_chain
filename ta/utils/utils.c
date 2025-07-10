/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#include "utils.h"
#include "key_list/key_list.h"



/* 签名验证函数 */
TEE_Result verify_signature(const void *data, size_t data_len, 
                           const char *sigkey, const char *signature) {
	/* 使用OP-TEE的签名验证API */
	TEE_OperationHandle op = TEE_HANDLE_NULL;
	TEE_ObjectHandle key_obj = TEE_HANDLE_NULL;
	TEE_Result res;
	
	/* 这里简化实现，实际应该解析sigkey并验证signature */
	/* 返回TEE_SUCCESS表示验证成功 */
	return TEE_SUCCESS;
}

/* 时间相关函数 */
TEE_Time get_trust_time(void) {
	TEE_Time time;
	TEE_GetSystemTime(&time);
	return time;
}

/* 哈希计算函数 */
TEE_Result hash_data(const void *data, size_t data_len, char *hash) {
	TEE_OperationHandle op = TEE_HANDLE_NULL;
	TEE_Result res;
	uint8_t hash_buffer[32]; /* SHA256 hash size */
	size_t hash_len = sizeof(hash_buffer);
	
	res = TEE_AllocateOperation(TEE_ALG_SHA256, TEE_MODE_DIGEST, 0, &op);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	res = TEE_DigestDoFinal(op, (uint8_t *)data, data_len, hash_buffer, &hash_len);
	TEE_FreeOperation(op);
	
	if (res == TEE_SUCCESS) {
		/* 将hash转换为十六进制字符串 */
		for (size_t i = 0; i < hash_len; i++) {
			TEE_Snprintf(hash + i * 2, 3, "%02x", hash_buffer[i]);
		}
	}
	
	return res;
}



/* TEE签名生成函数 */
void generate_tee_signature(const char *hash, char *tee_sig) {
	/* 生成TEE签名，这里简化实现 */
	TEE_StrCpy(tee_sig, "tee_signature_placeholder");
}

/* 区块生成函数 */
TEE_Result get_access_block(const struct repo_metadata *repo, uint32_t op, uint32_t role,
                           const char *pubkey, const char *sigkey, 
                           const char *signature, struct block *block) {
	TEE_Result res;
	
	/* 验证签名 */
	res = verify_signature(pubkey, TEE_StrLen(pubkey), sigkey, signature);
	if (res != TEE_SUCCESS) {
		return TEE_ERROR_SECURITY;
	}
	
	/* 检查权限 */
	if (role == ROLE_ADMIN) {
		if (!key_exists_in_set(repo->admin_keys, pubkey)) {
			return TEE_ERROR_ACCESS_DENIED;
		}
	} else if (role == ROLE_WRITER) {
		if (!key_exists_in_set(repo->writer_keys, pubkey)) {
			return TEE_ERROR_ACCESS_DENIED;
		}
	}
	
	/* 填充block结构 */
	block->block_height = repo->block_height + 1;
	TEE_StrCpy(block->parent_hash, repo->latest_hash);
	block->op = op;
	block->role = role;
	TEE_StrCpy(block->pubkey, pubkey);
	TEE_StrCpy(block->sigkey, sigkey);
	TEE_StrCpy(block->signature, signature);
	block->trust_timestamp = get_trust_time();
	
	/* 生成TEE签名 */
	generate_tee_signature(block->parent_hash, block->tee_sig);
	
	return TEE_SUCCESS;
}

TEE_Result get_contribution_block(const struct repo_metadata *repo, uint32_t op,
                                 const char *branch, const char *commit_hash,
                                 const char *sigkey, const char *signature,
                                 struct block *block) {
	TEE_Result res;
	char data_to_verify[1024];
	
	/* 构造验证数据 */
	TEE_Snprintf(data_to_verify, sizeof(data_to_verify), 
	             "%s:%s:%s", branch, commit_hash, sigkey);
	
	/* 验证签名 */
	res = verify_signature(data_to_verify, TEE_StrLen(data_to_verify), 
	                      sigkey, signature);
	if (res != TEE_SUCCESS) {
		return TEE_ERROR_SECURITY;
	}
	
	/* 填充block结构 */
	block->block_height = repo->block_height + 1;
	TEE_StrCpy(block->parent_hash, repo->latest_hash);
	block->op = op;
	TEE_StrCpy(block->branch, branch);
	TEE_StrCpy(block->commit_hash, commit_hash);
	TEE_StrCpy(block->sigkey, sigkey);
	TEE_StrCpy(block->signature, signature);
	block->trust_timestamp = get_trust_time();
	
	/* 生成TEE签名 */
	generate_tee_signature(block->parent_hash, block->tee_sig);
	
	return TEE_SUCCESS;
} 