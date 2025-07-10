#!/bin/bash

# Trust Chain API 测试脚本
# 根据需求文档测试所有接口

echo "=== Trust Chain API 测试 ==="

# 1. 测试初始化仓库 (Init) - 只需要创始人的公钥
echo "1. 测试初始化仓库 (Init)"
curl -X POST http://localhost:8080/init-repo \
  -H "Content-Type: application/json" \
  -d '{
    "founder_key": "founder_public_key_123"
  }'

echo -e "\n\n"

# 2. 测试访问控制 - 添加写作者 (AccessControl ADD)
echo "2. 测试访问控制 - 添加写作者 (AccessControl ADD)"
curl -X POST http://localhost:8080/access-control \
  -H "Content-Type: application/json" \
  -d '{
    "repo_id": 0,
    "operation": "ADD",
    "role": "WRITER",
    "public_key": "new_writer_key_123",
    "signature_key": "founder_public_key_123",
    "signature": "signature_for_add_writer"
  }'

echo -e "\n\n"

# 3. 测试提交操作 (Commit)
echo "3. 测试提交操作 (Commit)"
curl -X POST http://localhost:8080/commit \
  -H "Content-Type: application/json" \
  -d '{
    "repo_id": 0,
    "operation": "PUSH",
    "branch": "main",
    "commit_hash": "abc123def456",
    "signature_key": "writer_public_key_456",
    "signature": "signature_for_commit"
  }'

echo -e "\n\n"

# 4. 测试获取最新哈希 (GetLatestHash)
echo "4. 测试获取最新哈希 (GetLatestHash)"
curl -X GET "http://localhost:8080/latest-hash/0?nonce=12345"

echo -e "\n\n"

# 5. 测试访问控制 - 删除写作者 (AccessControl DELETE)
echo "5. 测试访问控制 - 删除写作者 (AccessControl DELETE)"
curl -X POST http://localhost:8080/access-control \
  -H "Content-Type: application/json" \
  -d '{
    "repo_id": 0,
    "operation": "DELETE",
    "role": "WRITER",
    "public_key": "new_writer_key_123",
    "signature_key": "founder_public_key_123",
    "signature": "signature_for_delete_writer"
  }'

echo -e "\n\n"

# 6. 测试删除仓库 (Delete)
echo "6. 测试删除仓库 (Delete)"
curl -X POST http://localhost:8080/delete-repo \
  -H "Content-Type: application/json" \
  -d '{
    "repo_id": 0,
    "admin_key": "founder_public_key_123",
    "signature_key": "founder_public_key_123",
    "signature": "signature_for_delete_repo"
  }'

echo -e "\n\n测试完成！" 