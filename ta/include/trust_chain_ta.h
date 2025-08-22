/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#ifndef TA_TRUST_CHAIN_H
#define TA_TRUST_CHAIN_H

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

/* Operation types */
#define OP_ADD     0
#define OP_DELETE  1
#define OP_PUSH    2
#define OP_PR      3

/* Role types */
#define ROLE_ADMIN  1
#define ROLE_WRITER 2

/* Maximum repository ID */
#define MAX_REPO_ID 1000

/* Maximum key length */
#define MAX_KEY_LENGTH 512

/* Maximum hash length */
#define MAX_HASH_LENGTH 64

/* Maximum signature length */
#define MAX_SIGNATURE_LENGTH 512

/* Maximum branch name length */
#define MAX_BRANCH_LENGTH 128

#endif /* TA_TRUST_CHAIN_H */ 