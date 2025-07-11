/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#include "key_list.h"

/* Memory management functions for key_list */

void init_key_list(struct key_list *key_list) {
	key_list->head = NULL;
	key_list->count = 0;
}

void cleanup_key_list(struct key_list *key_list) {
	struct key_node *current = key_list->head;
	struct key_node *next;
	
	while (current != NULL) {
		next = current->next;
		free_key_node(current);
		current = next;
	}
	
	key_list->head = NULL;
	key_list->count = 0;
}

TEE_Result copy_key_string(const char *src, char **dst) {
	size_t len = TEE_StrLen(src) + 1;
	*dst = TEE_Malloc(len, TEE_MALLOC_NO_FLAGS);
	if (*dst == NULL) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	TEE_MemMove(*dst, src, len);
	return TEE_SUCCESS;
}

struct key_node *create_key_node(const char *key) {
	struct key_node *node = TEE_Malloc(sizeof(struct key_node), TEE_MALLOC_NO_FLAGS);
	if (node == NULL) {
		return NULL;
	}
	
	if (copy_key_string(key, &node->key) != TEE_SUCCESS) {
		TEE_Free(node);
		return NULL;
	}
	
	node->next = NULL;
	return node;
}

void free_key_node(struct key_node *node) {
	if (node != NULL) {
		if (node->key != NULL) {
			TEE_Free(node->key);
		}
		TEE_Free(node);
	}
}

/* Key list operations */

bool key_exists_in_set(const struct key_list *key_list, const char *key) {
	struct key_node *current = key_list->head;
	
	while (current != NULL) {
		if (TEE_StrCmp(current->key, key) == 0) {
			return true;
		}
		current = current->next;
	}
	return false;
}

TEE_Result add_key_to_set(struct key_list *key_list, const char *key) {
	/* 创建新节点 */
	struct key_node *new_node = create_key_node(key);
	if (new_node == NULL) {
		return TEE_ERROR_OUT_OF_MEMORY;
	}
	
	/* 添加到链表头部 */
	new_node->next = key_list->head;
	key_list->head = new_node;
	key_list->count++;
	
	return TEE_SUCCESS;
}

TEE_Result remove_key_from_set(struct key_list *key_list, const char *key) {
	struct key_node *current = key_list->head;
	struct key_node *prev = NULL;
	
	while (current != NULL) {
		if (TEE_StrCmp(current->key, key) == 0) {
			/* 找到要删除的节点 */
			if (prev == NULL) {
				/* 删除头节点 */
				key_list->head = current->next;
			} else {
				/* 删除中间或尾节点 */
				prev->next = current->next;
			}
			
			free_key_node(current);
			key_list->count--;
			return TEE_SUCCESS;
		}
		prev = current;
		current = current->next;
	}
	
	return TEE_ERROR_ITEM_NOT_FOUND;
} 