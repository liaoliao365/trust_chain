/*
 * Copyright (c) 2024, Trust Chain Project
 * All rights reserved.
 */

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include <jansson.h>  // 需要安装: sudo apt-get install libjansson-dev


/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* For the UUID (found in the TA's h-file(s)) */
#include <trust_chain_ta.h>

// 定义常量
#define MAX_HASH_LENGTH 64
#define MAX_KEY_LENGTH 256
#define MAX_SIGNATURE_LENGTH 512

// 定义结构体（与TA端保持一致）
struct base_block {
    uint32_t block_height;
    char parent_hash[MAX_HASH_LENGTH];
    uint32_t op;
    char sigkey[MAX_KEY_LENGTH];
    char signature[MAX_SIGNATURE_LENGTH];
    uint64_t trust_timestamp;  // 简化为uint64_t
    char tee_sig[MAX_SIGNATURE_LENGTH];
};

struct access_block {
    struct base_block base;
    uint32_t role;
    char pubkey[MAX_KEY_LENGTH];
};

/* Contribution区块结构体 */
struct contribution_block {
    struct base_block base;          // 继承基础区块
    char commit_hash[MAX_HASH_LENGTH]; // 提交哈希
};

#define PORT 8080
#define BUFFER_SIZE 4096

// 全局TEE会话
static TEEC_Context ctx;
static TEEC_Session sess;
static int tee_initialized = 0;

// 初始化TEE连接
int init_tee_connection() {
    if (tee_initialized) return 0;
    
	TEEC_Result res;
	TEEC_UUID uuid = TA_TRUST_CHAIN_UUID;
	uint32_t err_origin;

	res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS) {
        printf("TEEC_InitializeContext failed with code 0x%x\n", res);
        return -1;
    }

	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS) {
        printf("TEEC_OpenSession failed with code 0x%x origin 0x%x\n", res, err_origin);
        TEEC_FinalizeContext(&ctx);
        return -1;
    }

    tee_initialized = 1;
    printf("TEE connection initialized successfully\n");
    return 0;
}

// 关闭TEE连接
void close_tee_connection() {
    if (tee_initialized) {
        TEEC_CloseSession(&sess);
        TEEC_FinalizeContext(&ctx);
        tee_initialized = 0;
        printf("TEE connection closed\n");
    }
}

// 发送JSON响应
void send_json_response(int client_socket, int status_code, const char *json_response) {
    char response[2048];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %d OK\r\n"
             "Content-Type: application/json\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             status_code, strlen(json_response), json_response);
    
    (void)write(client_socket, response, strlen(response));
}

// 解析JSON请求
json_t* parse_json_request(const char *body) {
    json_error_t error;
    json_t *root = json_loads(body, 0, &error);
    if (!root) {
        printf("JSON parse error: %s\n", error.text);
        return NULL;
    }
    return root;
}

// 处理初始化仓库请求
void handle_init_repo(int client_socket, const char *body) {
    printf("Handling init-repo request\n");
    
    json_t *root = parse_json_request(body);
    if (!root) {
        send_json_response(client_socket, 400, "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    json_t *admin_key_json = json_object_get(root, "admin_key");
    
    if (!json_is_string(admin_key_json)) {
        json_decref(root);
        send_json_response(client_socket, 400, "{\"error\":\"Missing required field: admin_key\"}");
        return;
    }
    
    const char *admin_key = json_string_value(admin_key_json);
    
    printf("Initializing repository with admin_key: %s\n", admin_key);
    
    // 调用OP-TEE TA
    TEEC_Operation op;
    TEEC_Result res;
    uint32_t err_origin;

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_VALUE_OUTPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE);

    op.params[0].tmpref.buffer = (void *)admin_key;
    op.params[0].tmpref.size = strlen(admin_key) + 1;
    
    // 为输出参数分配内存
    uint32_t repo_id;
    struct access_block genesis_block;
    op.params[1].value.a = 0; // 输出仓库ID
    op.params[2].tmpref.buffer = &genesis_block;
    op.params[2].tmpref.size = sizeof(struct access_block);

    res = TEEC_InvokeCommand(&sess, TA_TRUST_CHAIN_CMD_INIT_REPO, &op, &err_origin);
    
    json_decref(root);

    if (res != TEEC_SUCCESS) {
        printf("Failed to initialize repository: 0x%x origin 0x%x\n", res, err_origin);
        send_json_response(client_socket, 500, "{\"error\":\"Failed to initialize repository\"}");
        return;
    }

    repo_id = op.params[1].value.a;
    printf("Repository initialized successfully with ID: %u\n", repo_id);
    
    // 使用已分配的genesis_block结构体
    
    // 构建包含access_block信息的JSON响应
    char response[2048];
    snprintf(response, sizeof(response), 
            "{\"status\":\"success\","
            "\"repository_id\":%u,"
            "\"genesis_block\":{"
            "\"block_height\":%u,"
            "\"parent_hash\":\"%s\","
            "\"op\":%u,"
            "\"sigkey\":\"%s\","
            "\"signature\":\"%s\","
            "\"trust_timestamp\":%llu,"
            "\"tee_sig\":\"%s\","
            "\"role\":%u,"
            "\"pubkey\":\"%s\""
            "}}", 
            repo_id,
            genesis_block.base.block_height,
            genesis_block.base.parent_hash,
            genesis_block.base.op,
            genesis_block.base.sigkey,
            genesis_block.base.signature,
            (unsigned long long)genesis_block.base.trust_timestamp,
            genesis_block.base.tee_sig,
            genesis_block.role,
            genesis_block.pubkey);
    
    send_json_response(client_socket, 200, response);
}

// 处理提交请求
void handle_commit(int client_socket, const char *body) {
    printf("Handling commit request\n");
    
    json_t *root = parse_json_request(body);
    if (!root) {
        send_json_response(client_socket, 400, "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    json_t *repo_id_json = json_object_get(root, "repo_id");
    json_t *pubkey_json = json_object_get(root, "pubkey");
    json_t *branch_json = json_object_get(root, "branch");
    
    if (!json_is_integer(repo_id_json) || !json_is_string(pubkey_json) || !json_is_string(branch_json)) {
        json_decref(root);
        send_json_response(client_socket, 400, "{\"error\":\"Missing required fields: repo_id, pubkey, branch\"}");
        return;
    }
    
    uint32_t repo_id = json_integer_value(repo_id_json);
    const char *pubkey = json_string_value(pubkey_json);
    const char *branch = json_string_value(branch_json);
    
    printf("Committing to repository %u, pubkey: %s, branch: %s\n", repo_id, pubkey, branch);
    
    // 调用OP-TEE TA
    TEEC_Operation op;
    TEEC_Result res;
    uint32_t err_origin;

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                     TEEC_VALUE_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT,
                                     TEEC_MEMREF_TEMP_INPUT);

    op.params[0].value.a = repo_id;
    op.params[0].value.b = OP_PUSH;
    op.params[2].tmpref.buffer = (void *)pubkey;
    op.params[2].tmpref.size = strlen(pubkey) + 1;
    op.params[3].tmpref.buffer = (void *)branch;
    op.params[3].tmpref.size = strlen(branch) + 1;

    res = TEEC_InvokeCommand(&sess, TA_TRUST_CHAIN_CMD_COMMIT, &op, &err_origin);
    
    json_decref(root);
    
    if (res == TEEC_SUCCESS) {
        printf("Commit successful\n");
        send_json_response(client_socket, 200, "{\"status\":\"success\"}");
    } else {
        printf("Failed to commit: 0x%x origin 0x%x\n", res, err_origin);
        send_json_response(client_socket, 500, "{\"error\":\"Failed to commit\"}");
    }
}

// 处理访问控制请求
void handle_access_control(int client_socket, const char *body) {
    printf("Handling access-control request\n");
    
    json_t *root = parse_json_request(body);
    if (!root) {
        send_json_response(client_socket, 400, "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    json_t *repo_id_json = json_object_get(root, "repo_id");
    json_t *operation_json = json_object_get(root, "operation");
    json_t *role_json = json_object_get(root, "role");
    json_t *public_key_json = json_object_get(root, "public_key");
    json_t *signature_key_json = json_object_get(root, "signature_key");
    json_t *signature_json = json_object_get(root, "signature");
    
    if (!json_is_integer(repo_id_json) || !json_is_string(operation_json) || 
        !json_is_string(role_json) || !json_is_string(public_key_json) ||
        !json_is_string(signature_key_json) || !json_is_string(signature_json)) {
        json_decref(root);
        send_json_response(client_socket, 400, "{\"error\":\"Missing required fields\"}");
        return;
    }
    
    uint32_t repo_id = json_integer_value(repo_id_json);
    const char *operation = json_string_value(operation_json);
    const char *role = json_string_value(role_json);
    const char *public_key = json_string_value(public_key_json);
    // const char *signature_key = json_string_value(signature_key_json);
    // const char *signature = json_string_value(signature_json);
    
    printf("Access control: repo_id=%u, operation=%s, role=%s, public_key=%s\n", 
           repo_id, operation, role, public_key);
    
    // 调用OP-TEE TA
    TEEC_Operation op;
    TEEC_Result res;
    uint32_t err_origin;
	
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_VALUE_INPUT,
					 TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT);

    op.params[0].value.a = repo_id;
    op.params[0].value.b = (strcmp(operation, "ADD") == 0) ? OP_ADD : OP_DELETE;
    op.params[1].value.a = (strcmp(role, "ADMIN") == 0) ? ROLE_ADMIN : ROLE_WRITER;
    op.params[3].tmpref.buffer = (void *)public_key;
    op.params[3].tmpref.size = strlen(public_key) + 1;

    res = TEEC_InvokeCommand(&sess, TA_TRUST_CHAIN_CMD_ACCESS_CONTROL, &op, &err_origin);
    
    json_decref(root);
    
    if (res == TEEC_SUCCESS) {
        printf("Access control successful\n");
        send_json_response(client_socket, 200, "{\"status\":\"success\"}");
    } else {
        printf("Failed to perform access control: 0x%x origin 0x%x\n", res, err_origin);
        send_json_response(client_socket, 500, "{\"error\":\"Failed to perform access control\"}");
    }
}

// 处理获取最新哈希请求
void handle_get_latest_hash(int client_socket, uint32_t repo_id) {
    printf("Getting latest hash for repository %u\n", repo_id);
    
    TEEC_Operation op;
    TEEC_Result res;
    uint32_t err_origin;
	
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE,
					 TEEC_NONE);

    op.params[0].value.a = repo_id;
	
	char hash_output[256];
	op.params[1].tmpref.buffer = hash_output;
	op.params[1].tmpref.size = sizeof(hash_output);

    res = TEEC_InvokeCommand(&sess, TA_TRUST_CHAIN_CMD_GET_LATEST_HASH, &op, &err_origin);
    
    if (res == TEEC_SUCCESS) {
        char response[512];
        snprintf(response, sizeof(response), 
                "{\"status\":\"success\",\"latest_hash\":\"%s\"}", 
                (char *)op.params[1].tmpref.buffer);
        send_json_response(client_socket, 200, response);
    } else {
        printf("Failed to get latest hash: 0x%x origin 0x%x\n", res, err_origin);
        send_json_response(client_socket, 500, "{\"error\":\"Failed to get latest hash\"}");
    }
}

// 处理HTTP请求
void handle_http_request(int client_socket, const char *request) {
    char method[16], path[256];
    sscanf(request, "%s %s", method, path);
    
    printf("Received %s request for %s\n", method, path);
    
    // 处理OPTIONS请求（CORS预检）
    if (strcmp(method, "OPTIONS") == 0) {
        char response[] = "HTTP/1.1 200 OK\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                         "Access-Control-Allow-Headers: Content-Type\r\n"
                         "\r\n";
        (void)write(client_socket, response, strlen(response));
        return;
    }
    
    // 提取请求体
    const char *body_start = strstr(request, "\r\n\r\n");
    const char *body = body_start ? body_start + 4 : "";
    
    if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/init-repo") == 0) {
            handle_init_repo(client_socket, body);
        } else if (strcmp(path, "/commit") == 0) {
            handle_commit(client_socket, body);
        } else if (strcmp(path, "/access-control") == 0) {
            handle_access_control(client_socket, body);
        } else {
            send_json_response(client_socket, 404, "{\"error\":\"Endpoint not found\"}");
        }
    } else if (strcmp(method, "GET") == 0) {
        if (strncmp(path, "/latest-hash/", 13) == 0) {
            uint32_t repo_id = atoi(path + 13);
            handle_get_latest_hash(client_socket, repo_id);
        } else {
            send_json_response(client_socket, 404, "{\"error\":\"Endpoint not found\"}");
        }
    } else {
        send_json_response(client_socket, 405, "{\"error\":\"Method not allowed\"}");
    }
}

// 处理客户端连接（在新线程中）
void *handle_client(void *socket_desc) {
    int connection_socket = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    bytes_read = recv(connection_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        handle_http_request(connection_socket, buffer);
    }
    
    close(connection_socket);
    free(socket_desc);
    return NULL;
}

int main(void)
{
    printf("=== Trust Chain HTTP Service ===\n");

    // 初始化TEE连接
    if (init_tee_connection() != 0) {
        printf("Failed to initialize TEE connection\n");
        return 1;
    }

    // 创建socket服务器
    int listen_socket, connection_socket;  // listen_socket用于监听，connection_socket用于与客户端通信
    struct sockaddr_in server_addr, client_addr;

    pthread_t thread_id;

    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == -1) {
        printf("Failed to create socket\n");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind failed\n");
        return 1;
    }

    if (listen(listen_socket, 10) < 0) {
        printf("Listen failed\n");
        return 1;
    }

    printf("Trust Chain HTTP Service started on port %d\n", PORT);
    printf("Available endpoints:\n");
    printf("  POST /init-repo - Initialize repository\n");
    printf("  POST /access-control - Access control\n");
    printf("  GET /latest-hash/{repo_id} - Get latest hash\n");
    printf("  POST /commit - Commit operation\n");

    socklen_t client_len = sizeof(client_addr);
    // 主循环：接受客户端连接并为每个连接创建新线程
    while ((connection_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &client_len))>=0) {
        int *new_socket = malloc(sizeof(int));
        if (new_socket == NULL) {
            perror("new_socket malloc failed");
            close(connection_socket);
            continue;
        }

        *new_socket = connection_socket;
        
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_socket) != 0) {
            printf("Could not create thread\n");
            free(new_socket);
            close(connection_socket);
            return 1;
        }
        client_len = sizeof(client_addr);  // 每次调用前都要重新赋值
    }

    // 清理TEE连接
    close_tee_connection();

	return 0;
}
 