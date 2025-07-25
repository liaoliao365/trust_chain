/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#ifndef TA_TRUST_CHAIN_H
#define TA_TRUST_CHAIN_H

#include <tee_api_types.h>
#include <stdbool.h>
#include "trust_chain_constants.h"

/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_TRUST_CHAIN_UUID \
	{ 0x8aaaf201, 0x2450, 0x11e4, \
		{ 0xab, 0xe2, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b} }

/* Trust Chain TA commands */
#define TA_TRUST_CHAIN_CMD_INIT_REPO             0
#define TA_TRUST_CHAIN_CMD_ACCESS_CONTROL        2
#define TA_TRUST_CHAIN_CMD_GET_LATEST_HASH       3
#define TA_TRUST_CHAIN_CMD_COMMIT                4
#define TA_TRUST_CHAIN_CMD_GET_TEE_PUBKEY        5

//还有PR操作，但是PUSH和PR都交给commit函数处理，只有PUSH需要检查是否有写权限

/* Repository metadata structure */
struct repo_metadata {
	uint32_t block_height;
	char latest_hash[MAX_HASH_LENGTH];
	char founder_key[MAX_KEY_LENGTH];  /* 创始人公钥 */
	struct key_list *admin_keys;
	struct key_list *writer_keys;
};

/* Access control message structure */
struct access_control_message {
	uint32_t rep_id;
	uint32_t op;
	uint32_t role;
	char pubkey[MAX_KEY_LENGTH];
	char sigkey[MAX_KEY_LENGTH];
	char signature[MAX_SIGNATURE_LENGTH];
};

/* Commit message structure */
struct commit_message {
	uint32_t rep_id;
	uint32_t op;
	char commit_hash[MAX_HASH_LENGTH];
	char sigkey[MAX_KEY_LENGTH];
	char signature[MAX_SIGNATURE_LENGTH];
};

/* Latest hash message structure */
struct latesthash_msg {
	uint32_t nonce;
	char latest_hash[MAX_HASH_LENGTH];
};

/* 向后兼容的通用区块结构体 - 用于外部接口 */
struct block {
	uint32_t block_height;
	char parent_hash[MAX_HASH_LENGTH];
	uint32_t op;
	uint32_t role;
	char pubkey[MAX_KEY_LENGTH];
	char sigkey[MAX_KEY_LENGTH];
	char signature[MAX_SIGNATURE_LENGTH];
	char tee_sig[MAX_SIGNATURE_LENGTH];
	char commit_hash[MAX_HASH_LENGTH];
	TEE_Time trust_timestamp;
};

#endif /* TA_TRUST_CHAIN_H */ 