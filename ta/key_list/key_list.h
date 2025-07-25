/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#ifndef KEY_LIST_H
#define KEY_LIST_H

#include <tee_api_types.h>
#include <stdbool.h>

/* Key list node structure */
struct key_node {
	char *key;                    /* 动态分配的密钥字符串 */
	struct key_node *next;        /* 指向下一个节点的指针 */
};

/* Key list structure - linked list */
struct key_list {
	struct key_node *head;        /* 链表头指针 */
	uint32_t count;              /* 节点数量 */
};

/* Memory management functions for key_list */
void init_key_list(struct key_list *key_list);
void cleanup_key_list(struct key_list *key_list);
TEE_Result copy_key_string(const char *src, char **dst);
struct key_node *create_key_node(const char *key);
void free_key_node(struct key_node *node);

/* Key list operations */
bool key_exists_in_set(const struct key_list *key_list, const char *key);
TEE_Result add_key_to_set(struct key_list *key_list, const char *key);
TEE_Result remove_key_from_set(struct key_list *key_list, const char *key);

/* 组合操作：查找并删除（如果存在） */
TEE_Result find_and_remove_key(struct key_list *key_list, const char *key, bool *was_found);

#endif /* KEY_LIST_H */ 