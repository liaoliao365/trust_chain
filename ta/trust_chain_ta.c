/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <string.h>
#include <stdlib.h>
#include "trust_chain_ta.h"
#include "key_list/key_list.h"
#include "utils/utils.h"

/* Global variables */
static uint32_t repo_num = 0;
struct repo_metadata *repositories[MAX_REPO_ID];



/* Main TA functions */

TEE_Result TA_CreateEntryPoint(void) {
	DMSG("TA_CreateEntryPoint has been called");
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
	case TA_TRUST_CHAIN_CMD_DELETE_REPO:
		return delete_repo(param_types, params);
	case TA_TRUST_CHAIN_CMD_ACCESS_CONTROL:
		return access_control(param_types, params);
	case TA_TRUST_CHAIN_CMD_GET_LATEST_HASH:
		return get_latest_hash(param_types, params);
	case TA_TRUST_CHAIN_CMD_COMMIT:
		return commit(param_types, params);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}

/* Command implementations */

/* 仓库存在性检查函数 */
static TEE_Result repo_exists(uint32_t rep_id) {
	if (rep_id >= MAX_REPO_ID || repositories[rep_id] == NULL) {
		return TEE_ERROR_ITEM_NOT_FOUND;
	}
	return TEE_SUCCESS;
}

/* 通用的仓库验证和获取函数 */
static TEE_Result validate_and_get_repo(uint32_t rep_id, struct repo_metadata **repo) {
	TEE_Result res = repo_exists(rep_id);
	if (res != TEE_SUCCESS) {
		return res;
	}
	*repo = repositories[rep_id];
	return TEE_SUCCESS;
}

/* 通用的仓库资源清理函数 */
static void cleanup_repo_resources(uint32_t rep_id) {
	if (rep_id < MAX_REPO_ID && repositories[rep_id] != NULL) {
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
}

static TEE_Result init_repo(uint32_t param_types, TEE_Param params[4]) {
	uint32_t rep_id;
	char *admin_key;
	struct block genesis_block;
	TEE_Result res;
	
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_VALUE_OUTPUT,
	                                   TEE_PARAM_TYPE_MEMREF_OUTPUT,
	                                   TEE_PARAM_TYPE_NONE)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	admin_key = (char *)params[0].memref.buffer;
	
	/* 计算新的仓库ID */
	rep_id = repo_num;
	if (rep_id >= MAX_REPO_ID) {
		//之后实现扩展仓库的逻辑
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	
	/* 分配repository结构 */
	repositories[rep_id] = TEE_Malloc(sizeof(struct repo_metadata), TEE_MALLOC_NO_FLAGS);
	if (repositories[rep_id] == NULL) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	
	/* 初始化repository */
	repositories[rep_id]->block_height = 0;
	TEE_StrCpy(repositories[rep_id]->latest_hash, "0000000000000000000000000000000000000000000000000000000000000000");
	TEE_StrCpy(repositories[rep_id]->founder_key, admin_key);  /* 保存创始人公钥 */
	
	/* 分配并初始化key lists */
	repositories[rep_id]->admin_keys = TEE_Malloc(sizeof(struct key_list), TEE_MALLOC_NO_FLAGS);
	repositories[rep_id]->writer_keys = TEE_Malloc(sizeof(struct key_list), TEE_MALLOC_NO_FLAGS);
	
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
	
	/* 生成Access创世区块 */
	res = get_access_block(repositories[rep_id], OP_ADD, ROLE_ADMIN, admin_key, admin_key, "", &genesis_block);
	if (res != TEE_SUCCESS) {
		/* 清理已分配的资源 */
		cleanup_repo_resources(rep_id);
		return res;
	}
	
	/* 返回计算出的仓库ID和创世区块 */
	params[1].value.a = rep_id;
	memcpy(params[2].memref.buffer, &genesis_block, sizeof(struct block));
	repo_num++;
	return TEE_SUCCESS;
}

static TEE_Result delete_repo(uint32_t param_types, TEE_Param params[4]) {
	uint32_t rep_id, op, role;
	char *pubkey;
	char *sigkey;
	char *signature;
	TEE_Result res;
	
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
	                                   TEE_PARAM_TYPE_VALUE_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_INPUT)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	rep_id = params[0].value.a;
	op = params[0].value.b;
	role = params[1].value.a;
	pubkey = (char *)params[2].memref.buffer;
	sigkey = (char *)params[3].memref.buffer;
	signature = sigkey; /* 简化处理，实际应该从参数中获取 */
	
	/* 获取并验证仓库 */
	struct repo_metadata *repo;
	res = validate_and_get_repo(rep_id, &repo);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	/* 构造验证数据 */
	char data_to_verify[1024];
	TEE_Snprintf(data_to_verify, sizeof(data_to_verify), 
	             "%u:%u:%u:%s", rep_id, op, role, pubkey);
	
	/* 验证签名 */
	res = verify_signature(data_to_verify, TEE_StrLen(data_to_verify), sigkey, signature);
	if (res != TEE_SUCCESS) {
		return TEE_ERROR_SECURITY;
	}
	
	/* 检查管理员权限 */
	
	if (!key_exists_in_set(repo->admin_keys, sigkey)) {
		return TEE_ERROR_ACCESS_DENIED;
	}
	
	/* 删除repository */
	cleanup_repo_resources(rep_id);
	
	repo_num--;
	return TEE_SUCCESS;
}

static TEE_Result access_control(uint32_t param_types, TEE_Param params[4]) {
	struct access_control_message *ac_msg;
	char *sigkey, *signature;
	struct block block;
	TEE_Result res;
	
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_NONE)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	ac_msg = (struct access_control_message *)params[0].memref.buffer;
	sigkey = (char *)params[1].memref.buffer;
	signature = (char *)params[2].memref.buffer;
	
	/* 获取并验证仓库 */
	struct repo_metadata *repo;
	res = validate_and_get_repo(ac_msg->rep_id, &repo);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	/* 检查授权者是否有管理员权限 */
	
	if (!key_exists_in_set(repo->admin_keys, sigkey)) {
		return TEE_ERROR_ACCESS_DENIED;
	}
	
	/* 构造验证数据 */
	char data_to_verify[1024];
	TEE_Snprintf(data_to_verify, sizeof(data_to_verify), 
	             "%u:%u:%u:%s", ac_msg->rep_id, ac_msg->op, ac_msg->role, ac_msg->pubkey);
	
	/* 验证签名 */
	res = verify_signature(data_to_verify, TEE_StrLen(data_to_verify), sigkey, signature);
	if (res != TEE_SUCCESS) {
		return TEE_ERROR_SECURITY;
	}
	
	/* 根据操作类型更新密钥集合 */
	if (ac_msg->op == OP_ADD) {
		if (ac_msg->role == ROLE_ADMIN) {
			/* 检查是否已在管理员列表中 */
			if (key_exists_in_set(repo->admin_keys, ac_msg->pubkey)) {
				return TEE_ERROR_BAD_PARAMETERS; /* 用户已在授权列表中 */
			}
			/* 如果用户是Writer，先删除Writer权限 */
			if (key_exists_in_set(repo->writer_keys, ac_msg->pubkey)) {
				remove_key_from_set(repo->writer_keys, ac_msg->pubkey);
			}
			add_key_to_set(repo->admin_keys, ac_msg->pubkey);
		} else if (ac_msg->role == ROLE_WRITER) {
			/* 检查是否已在Writer列表中 */
			if (key_exists_in_set(repo->writer_keys, ac_msg->pubkey)) {
				return TEE_ERROR_BAD_PARAMETERS; /* 用户已在授权列表中 */
			}
			/* 如果用户是Admin，不需要添加Writer权限 */
			if (key_exists_in_set(repo->admin_keys, ac_msg->pubkey)) {
				return TEE_ERROR_BAD_PARAMETERS; /* 用户是Admin，具有Writer权限 */
			}
			add_key_to_set(repo->writer_keys, ac_msg->pubkey);
		}
	} else if (ac_msg->op == OP_DELETE) {
		if (ac_msg->role == ROLE_ADMIN) {
			if (!key_exists_in_set(repo->admin_keys, ac_msg->pubkey)) {
				return TEE_ERROR_BAD_PARAMETERS; /* 用户不在列表中 */
			}
			remove_key_from_set(repo->admin_keys, ac_msg->pubkey);
		} else if (ac_msg->role == ROLE_WRITER) {
			if (!key_exists_in_set(repo->writer_keys, ac_msg->pubkey)) {
				return TEE_ERROR_BAD_PARAMETERS; /* 用户不在列表中 */
			}
			remove_key_from_set(repo->writer_keys, ac_msg->pubkey);
		}
	}
	
	/* 生成Access区块 */
	res = get_access_block(repositories[ac_msg->rep_id], ac_msg->op, ac_msg->role, ac_msg->pubkey, sigkey, signature, &block);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	/* 更新repository状态 */
	repositories[ac_msg->rep_id]->block_height = block.block_height;
	TEE_StrCpy(repositories[ac_msg->rep_id]->latest_hash, block.parent_hash);
	
	return TEE_SUCCESS;
}

static TEE_Result get_latest_hash(uint32_t param_types, TEE_Param params[4]) {
	uint32_t rep_id, nounce;
	char *hash_out;
	
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
	                                   TEE_PARAM_TYPE_VALUE_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_OUTPUT,
	                                   TEE_PARAM_TYPE_NONE)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	rep_id = params[0].value.a;
	nounce = params[1].value.a;
	hash_out = (char *)params[2].memref.buffer;
	
	/* 获取并验证仓库 */
	struct repo_metadata *repo;
	TEE_Result res = validate_and_get_repo(rep_id, &repo);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	/* 返回最新hash */
	TEE_StrCpy(hash_out, repo->latest_hash);
	
	return TEE_SUCCESS;
}

static TEE_Result commit(uint32_t param_types, TEE_Param params[4]) {
	struct commit_message *cm_msg;
	char *encrypted_key, *sigkey, *signature;
	struct block block;
	TEE_Result res;
	
	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_INPUT,
	                                   TEE_PARAM_TYPE_MEMREF_INPUT)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	
	cm_msg = (struct commit_message *)params[0].memref.buffer;
	encrypted_key = (char *)params[1].memref.buffer;
	sigkey = (char *)params[2].memref.buffer;
	signature = (char *)params[3].memref.buffer;
	
	/* 对于PUSH操作，检查写权限 */
	if (cm_msg->op == OP_PUSH) {
		struct repo_metadata *repo;
		res = validate_and_get_repo(cm_msg->rep_id, &repo);
		if (res != TEE_SUCCESS) {
			return res;
		}
		if (!key_exists_in_set(repo->admin_keys, sigkey) && !key_exists_in_set(repo->writer_keys, sigkey)) {
			return TEE_ERROR_ACCESS_DENIED;
		}
	}
	
	/* 构造验证数据 */
	char data_to_verify[1024];
	TEE_Snprintf(data_to_verify, sizeof(data_to_verify), 
	             "%u:%u:%s", cm_msg->rep_id, cm_msg->op, cm_msg->commit_hash);
	
	/* 验证签名 */
	res = verify_signature(data_to_verify, TEE_StrLen(data_to_verify), sigkey, signature);
	if (res != TEE_SUCCESS) {
		return TEE_ERROR_SECURITY;
	}
	
	/* 获取贡献block */
	res = get_contribution_block(repositories[cm_msg->rep_id], cm_msg->op, "", cm_msg->commit_hash, sigkey, signature, &block);
	if (res != TEE_SUCCESS) {
		return res;
	}
	
	/* 更新repository状态 */
	repositories[cm_msg->rep_id]->block_height = block.block_height;
	TEE_StrCpy(repositories[cm_msg->rep_id]->latest_hash, block.parent_hash);
	
	return TEE_SUCCESS;
} 