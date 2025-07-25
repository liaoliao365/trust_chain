/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "trust_chain_ta.h"
#include "key_list/key_list.h"
#include "utils/utils.h"
#include "block/block.h"
#include "tee_key_manager/tee_key_manager.h"

/* Global variables */
static uint32_t repo_num = 0;
struct repo_metadata *repositories[MAX_REPO_ID];

/* Function declarations */
static TEE_Result init_repo(uint32_t param_types, TEE_Param params[4]);
static TEE_Result access_control(uint32_t param_types, TEE_Param params[4]);
static TEE_Result get_latest_hash(uint32_t param_types, TEE_Param params[4]);
static TEE_Result commit(uint32_t param_types, TEE_Param params[4]);
static TEE_Result get_tee_public_key(uint32_t param_types, TEE_Param params[4]);
static TEE_Result validate_and_get_repo(uint32_t rep_id, struct repo_metadata **repo);
static void cleanup_repo_resources(uint32_t rep_id);

/* Main TA functions */

TEE_Result TA_CreateEntryPoint(void) {
	DMSG("TA_CreateEntryPoint has been called");
	
	/* TEE密钥管理器采用按需初始化，无需显式初始化 */
	IMSG("Trust Chain TA initialized successfully");
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void) {
	DMSG("TA_DestroyEntryPoint has been called");
	
	/* TA销毁时不需要清理仓库信息，这些信息应该持久化保存 */
	/* 仓库信息会在下次TA启动时从持久化存储中恢复 */
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
                                   TEE_Param params[4],
                                   void **sess_ctx) {
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
										TEE_PARAM_TYPE_NONE,
										TEE_PARAM_TYPE_NONE,
										TEE_PARAM_TYPE_NONE);
			 
	DMSG("TA_OpenSessionEntryPoint has been called");
			 
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	(void)params;
	(void)sess_ctx;

	IMSG("Trust Chain TA has been called!\n");
	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *sess_ctx) {
	(void)sess_ctx;
	IMSG("Goodbye!\n");
}

TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx, uint32_t cmd_id,
                                     uint32_t param_types, TEE_Param params[4]) {
	(void)sess_ctx;
	
	switch (cmd_id) {
	case TA_TRUST_CHAIN_CMD_INIT_REPO:
		return init_repo(param_types, params);
	case TA_TRUST_CHAIN_CMD_ACCESS_CONTROL:
		return access_control(param_types, params);
	case TA_TRUST_CHAIN_CMD_GET_LATEST_HASH:
		return get_latest_hash(param_types, params);
	case TA_TRUST_CHAIN_CMD_COMMIT:
		return commit(param_types, params);
	case TA_TRUST_CHAIN_CMD_GET_TEE_PUBKEY:
		return get_tee_public_key(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}

/* Command implementations */

/* 通用的仓库验证和获取函数 */
static TEE_Result validate_and_get_repo(uint32_t rep_id, struct repo_metadata **repo) {
	if (rep_id >= MAX_REPO_ID || repositories[rep_id] == NULL) {
		return TEE_ERROR_ITEM_NOT_FOUND;
	}
	*repo = repositories[rep_id];
	return TEE_SUCCESS;
}

/* 通用的仓库资源清理函数 */
static void cleanup_repo_resources(uint32_t rep_id) {
	if (rep_id >= MAX_REPO_ID || repositories[rep_id] == NULL) {
		DMSG("rep_id out of range or repository is NULL!");
		return;
	}
	struct repo_metadata *repo = repositories[rep_id];
	if (repo->admin_keys) {
		cleanup_key_list(repo->admin_keys);
		TEE_Free(repo->admin_keys);
	}
	if (repo->writer_keys) {
		cleanup_key_list(repo->writer_keys);
		TEE_Free(repo->writer_keys);
	}
	TEE_Free(repo);
	repositories[rep_id] = NULL;
}

static TEE_Result init_repo(uint32_t param_types, TEE_Param params[4]) {
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_VALUE_OUTPUT,
	                                   TEE_PARAM_TYPE_MEMREF_OUTPUT,
	                                   TEE_PARAM_TYPE_NONE)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	char *admin_key = (char *)params[0].memref.buffer;
	uint32_t rep_id = repo_num;
	struct access_block genesis_block;
	TEE_Result res;
	
	if (rep_id >= MAX_REPO_ID) {
		//之后实现扩展仓库的逻辑
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	
	/* 分配repository结构 */
	repositories[rep_id] = TEE_Malloc(sizeof(struct repo_metadata), TEE_MALLOC_FILL_ZERO);
	if (repositories[rep_id] == NULL) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	
	/* 初始化repository */
	repositories[rep_id]->block_height = 0;
	strcpy(repositories[rep_id]->latest_hash, "0000000000000000000000000000000000000000000000000000000000000000");
	strcpy(repositories[rep_id]->founder_key, admin_key);  /* 保存创始人公钥 */
	
	/* 分配并初始化key lists */
	repositories[rep_id]->admin_keys = TEE_Malloc(sizeof(struct key_list), TEE_MALLOC_FILL_ZERO);
	repositories[rep_id]->writer_keys = TEE_Malloc(sizeof(struct key_list), TEE_MALLOC_FILL_ZERO);
	
	if (repositories[rep_id]->admin_keys == NULL || repositories[rep_id]->writer_keys == NULL) {
		cleanup_repo_resources(rep_id);
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	
	init_key_list(repositories[rep_id]->admin_keys);
	init_key_list(repositories[rep_id]->writer_keys);
	
	/* 添加创始人公钥到管理员集合 */
	res = add_key_to_set(repositories[rep_id]->admin_keys, admin_key);
	if (res != TEE_SUCCESS) {
		cleanup_repo_resources(rep_id);      
		return res;
	}
	
	/* 生成Access创世区块 - 使用初始化函数 */
	init_access_block(&genesis_block, 1, repositories[rep_id]->latest_hash, 
	                  OP_ADD, ROLE_ADMIN, admin_key, admin_key, "");
	
	/* 计算创世区块哈希并生成TEE签名 */
	char genesis_hash[MAX_HASH_LENGTH];
	if ((res = calculate_access_block_hash(&genesis_block, genesis_hash)) != TEE_SUCCESS ||
	    (res = tee_sign_hash(genesis_hash, genesis_block.base.tee_sig)) != TEE_SUCCESS) {
		cleanup_repo_resources(rep_id);
		return res;
	}
	
	strcpy(repositories[rep_id]->latest_hash, genesis_hash);
	
	/* 返回计算出的仓库ID和创世区块 */
	params[1].value.a = rep_id;
	memcpy(params[2].memref.buffer, &genesis_block, sizeof(struct access_block));
	repo_num++;
	return TEE_SUCCESS;
}

static TEE_Result access_control(uint32_t param_types, TEE_Param params[4]) {
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_OUTPUT,
	                                   TEE_PARAM_TYPE_NONE,
	                                   TEE_PARAM_TYPE_NONE)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	struct access_control_message *ac_msg = (struct access_control_message *)params[0].memref.buffer;
	struct access_block block;
	struct repo_metadata *repo;
	TEE_Result res;

	/* 对于PUSH和PR操作，检查写权限 */
	if (ac_msg->op != OP_ADD && ac_msg->op != OP_DELETE){
		IMSG("Invalid operation: %u, support only ADD and DELETE", ac_msg->op);
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	res = validate_and_get_repo(ac_msg->rep_id, &repo);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	/* 检查授权者是否有管理员权限 */
	if (!key_exists_in_set(repo->admin_keys, ac_msg->sigkey)) {
		IMSG("Not Admin, not allowed to access, sigkey: %s", ac_msg->sigkey);
		return TEE_ERROR_ACCESS_DENIED;
	}
	
	/* 构造验证数据 */
	char data_to_verify[1024];
	snprintf(data_to_verify, sizeof(data_to_verify), 
	         "%u:%u:%u:%s", ac_msg->rep_id, ac_msg->op, ac_msg->role, ac_msg->pubkey);
	
	/* 验证签名 */
	res = verify_signature(data_to_verify, strlen(data_to_verify), ac_msg->sigkey, ac_msg->signature);
	if (res != TEE_SUCCESS) {
		return TEE_ERROR_SECURITY;
	}
	
	/* 根据操作类型更新密钥集合 */
	if (ac_msg->op == OP_ADD) {
		if (ac_msg->role == ROLE_ADMIN) {
			/* 检查是否已在管理员列表中 */
			if (key_exists_in_set(repo->admin_keys, ac_msg->pubkey)) {
				IMSG("Already in admin list: %s", ac_msg->pubkey);
				return TEE_ERROR_BAD_PARAMETERS; /* 用户已在授权列表中 */
			}
			/* 如果用户是Writer，先删除Writer权限 */
			bool was_writer = false;
			res = find_and_remove_key(repo->writer_keys, ac_msg->pubkey, &was_writer);
			if (res != TEE_SUCCESS) {
				return res;
			}
			if (was_writer) {
				IMSG("From writer to admin: %s", ac_msg->pubkey);
			}
			res = add_key_to_set(repo->admin_keys, ac_msg->pubkey);
			if (res != TEE_SUCCESS) {
				return res;
			}
		} else if (ac_msg->role == ROLE_WRITER) {
			/* 检查是否已在Writer列表中 */
			if (key_exists_in_set(repo->writer_keys, ac_msg->pubkey)) {
				IMSG("Already in writer list: %s", ac_msg->pubkey);
				return TEE_ERROR_BAD_PARAMETERS; /* 用户已在授权列表中 */
			}
			/* 如果用户是Admin，不需要添加Writer权限 */
			if (key_exists_in_set(repo->admin_keys, ac_msg->pubkey)) {
				IMSG("Already in admin list, has writer permission: %s", ac_msg->pubkey);
				return TEE_ERROR_BAD_PARAMETERS; /* 用户是Admin，具有Writer权限 */
			}
			res = add_key_to_set(repo->writer_keys, ac_msg->pubkey);
			if (res != TEE_SUCCESS) {
				return res;
			}
		}
	} else if (ac_msg->op == OP_DELETE) {
		if (ac_msg->role == ROLE_ADMIN) {
			bool was_found = false;
			res = find_and_remove_key(repo->admin_keys, ac_msg->pubkey, &was_found);
			if (res != TEE_SUCCESS) {
				return res;
			}
			if (!was_found) {
				IMSG("Not in Admin list: %s", ac_msg->pubkey);
				return TEE_ERROR_BAD_PARAMETERS; /* 用户不在列表中 */
			}
		} else if (ac_msg->role == ROLE_WRITER) {
			bool was_found = false;
			res = find_and_remove_key(repo->writer_keys, ac_msg->pubkey, &was_found);
			if (res != TEE_SUCCESS) {
				return res;
			}
			if (!was_found) {
				IMSG("Not in Writer list: %s", ac_msg->pubkey);
				return TEE_ERROR_BAD_PARAMETERS; /* 用户不在列表中 */
			}
		}
	}
	
	/* 生成Access区块 - 使用初始化函数 */
	init_access_block(&block, repo->block_height + 1,
	                  repo->latest_hash, ac_msg->op,
	                  ac_msg->role, ac_msg->pubkey, ac_msg->sigkey, ac_msg->signature);
	
	/* 计算区块哈希并生成TEE签名 */
	char block_hash[MAX_HASH_LENGTH];
	if ((res = calculate_access_block_hash(&block, block_hash)) != TEE_SUCCESS ||
	    (res = tee_sign_hash(block_hash, block.base.tee_sig)) != TEE_SUCCESS) {
		return res;
	}

	memcpy(params[1].memref.buffer, &block, sizeof(struct access_block));
	
	strcpy(repo->latest_hash, block_hash);
	repo->block_height++;

	return TEE_SUCCESS;
}

static TEE_Result get_latest_hash(uint32_t param_types, TEE_Param params[4]) {
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
	                                   TEE_PARAM_TYPE_VALUE_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_OUTPUT,
	                                   TEE_PARAM_TYPE_MEMREF_OUTPUT)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	uint32_t rep_id = params[0].value.a;
	uint32_t nonce = params[1].value.a;
	struct latesthash_msg *msg_out = (struct latesthash_msg *)params[2].memref.buffer;
	char *signature_out = (char *)params[3].memref.buffer;
	struct repo_metadata *repo;
	TEE_Result res;
	
	res = validate_and_get_repo(rep_id, &repo);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	/* 构造返回消息 */
	msg_out->nonce = nonce;
	strcpy(msg_out->latest_hash, repo->latest_hash);
	
	/* 生成TEE签名 */
	res = tee_sign_data((char *)msg_out, signature_out);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	return TEE_SUCCESS;
}

static TEE_Result commit(uint32_t param_types, TEE_Param params[4]) {
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_INOUT,
									   TEE_PARAM_TYPE_MEMREF_OUTPUT,
	                                   TEE_PARAM_TYPE_NONE)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	struct commit_message *cm_msg = (struct commit_message *)params[0].memref.buffer;
	char *encrypted_key = (char *)params[1].memref.buffer;
	struct contribution_block block;
	struct repo_metadata *repo;
	TEE_Result res;

	/* 对于PUSH和PR操作，检查写权限 */
	if (cm_msg->op != OP_PUSH && cm_msg->op != OP_PR){
		IMSG("Invalid operation: %u, support only PUSH and PR", cm_msg->op);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	/* 获取并验证仓库 */
	res = validate_and_get_repo(cm_msg->rep_id, &repo);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	if (!key_exists_in_set(repo->admin_keys, cm_msg->sigkey) && !key_exists_in_set(repo->writer_keys, cm_msg->sigkey)) {
		IMSG("Not Admin or Writer, not allowed to commit, sigkey: %s", cm_msg->sigkey);
		return TEE_ERROR_ACCESS_DENIED;	
	}
	
	/* 构造验证数据 */
	char data_to_verify[1024];
	snprintf(data_to_verify, sizeof(data_to_verify), 
	         "%u:%u:%s", cm_msg->rep_id, cm_msg->op, cm_msg->commit_hash);
	
	/* 验证签名 */
	res = verify_signature(data_to_verify, strlen(data_to_verify), cm_msg->sigkey, cm_msg->signature);
	if (res != TEE_SUCCESS) {
		return TEE_ERROR_SECURITY;
	}
	
	/* 生成Contribution区块 - 使用初始化函数 */
	init_contribution_block(&block, repo->block_height + 1,
	                       repo->latest_hash, cm_msg->op, cm_msg->commit_hash,
	                       cm_msg->sigkey, cm_msg->signature);
	
	/* 计算区块哈希并生成TEE签名 */
	char block_hash[MAX_HASH_LENGTH];
	if ((res = calculate_contribution_block_hash(&block, block_hash)) != TEE_SUCCESS ||
	    (res = tee_sign_hash(block_hash, block.base.tee_sig)) != TEE_SUCCESS) {
		return res;
	}
	
	/* 用TEE私钥解密encrypted_key并返回 */
	char decrypted_key[256];
	size_t decrypted_len = sizeof(decrypted_key);
	res = tee_decrypt_data(encrypted_key, strlen(encrypted_key), 
	                      decrypted_key, &decrypted_len);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	/* 将解密后的密钥复制回原缓冲区 */
	memcpy(encrypted_key, decrypted_key, decrypted_len);
	encrypted_key[decrypted_len] = '\0'; /* 确保字符串结尾 */

	/* 将区块复制到输出缓冲区 */
	memcpy(params[2].memref.buffer, &block, sizeof(struct contribution_block));

	strcpy(repo->latest_hash, block_hash);
	repo->block_height++;
	
	return TEE_SUCCESS;
} 

static TEE_Result get_tee_public_key(uint32_t param_types, TEE_Param params[4]) {
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
	                                   TEE_PARAM_TYPE_VALUE_OUTPUT,
	                                   TEE_PARAM_TYPE_NONE,
	                                   TEE_PARAM_TYPE_NONE)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	char *public_key_pem = (char *)params[0].memref.buffer;
	size_t pem_buf_size = params[0].memref.size;
	size_t pem_len = 0;
	
	if (!public_key_pem || pem_buf_size == 0) {
        return TEE_ERROR_BAD_PARAMETERS;
    }
	
	/* 从 TEE 获取真实的公钥 */
	TEE_ObjectHandle key_obj = TEE_HANDLE_NULL;
	TEE_Result res;
	
    // 示例用 UUID 打开持久化密钥，确保该 UUID 存在于 Secure Storage 中
    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
									&tee_key_pair_uuid,
									sizeof(tee_key_pair_uuid),
									TEE_DATA_FLAG_ACCESS_READ,
									&key_obj);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open TEE public key: 0x%x", res);
		return res;
	}
	
	/* 使用工具函数转换为 PEM 格式 */
	res = public_key_obj_to_pem(key_obj, public_key_pem, &pem_len);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to convert public key to PEM format: 0x%x", res);
		TEE_CloseObject(key_obj);
		return res;
	}

	params[1].value.a = (uint32_t)pem_len;
	
	TEE_CloseObject(key_obj);
	return TEE_SUCCESS;
} 