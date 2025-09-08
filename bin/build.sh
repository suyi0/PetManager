#!/bin/bash

# 获取当前脚本所在目录（bin目录）
BIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 计算项目根目录（bin目录的父目录）
PROJECT_ROOT="$(dirname "${BIN_DIR}")"

# 设置编译器和选项
COMPILER="/usr/bin/clang++"
CPP_STD="-std=c++20"
DEFINE_FLAGS="-DCROW_ENABLE_SSL"
INCLUDE_DIRS="-I/opt/homebrew/opt/crow/include \
              -I/opt/homebrew/include \
              -I/opt/homebrew/opt/openssl/include \
              -I/opt/homebrew/opt/mysql-connector-c++/include \
              -I/opt/homebrew/opt/curl/include \
              -I/opt/homebrew/opt/nlohmann-json/include"

LIB_DIRS="-L/usr/local/lib \
          -L/opt/homebrew/lib \
          -L/opt/homebrew/opt/openssl/lib \
          -L/opt/homebrew/opt/mysql-connector-c++/lib \
          -L/opt/homebrew/opt/curl/lib"

# 设置库文件路径，添加更多必要的 Boost 库
LIBS="-lboost_date_time -lboost_filesystem -lboost_thread -lssl -lcrypto -lmysqlcppconnx.2 -lcurl"

# 查找所有.cpp文件
CPP_FILES=$(find "${PROJECT_ROOT}/pethospital/src/api/src" -name "*.cpp" -type f)

# 设置输出文件路径
OUTPUT_FILE="${PROJECT_ROOT}/pethospital/src/api/src/RunServe"

# 执行编译
echo "正在编译项目..."
echo "编译器: ${COMPILER}"
echo "源文件: ${CPP_FILES}"
echo "输出文件: ${OUTPUT_FILE}"

${COMPILER} ${CPP_STD} ${DEFINE_FLAGS} ${INCLUDE_DIRS} -Wall -Werror -pedantic -g -fcolor-diagnostics -fansi-escape-codes -pthread ${CPP_FILES} -o ${OUTPUT_FILE} ${LIB_DIRS} ${LIBS} 

# 检查编译结果
if [ $? -eq 0 ]; then
    echo "编译成功！可执行文件位于: ${OUTPUT_FILE}"
else
    echo "编译失败！"
    exit 1
fi