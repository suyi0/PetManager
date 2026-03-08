
# 设置 PATH 以确保能访问基本命令
export PATH="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:$PATH"
export PATH="$HOME/.npm-global/bin:$PATH"
export PATH="$PATH:/usr/local/mysql/bin"
export PATH="/opt/homebrew/opt/curl/bin:$PATH"
export PATH="$PATH:/Applications/Tailscale.app/Contents/MacOS"
export PATH="/opt/homebrew/opt/mysql-client/bin:$PATH"

# 加载环境变量
ENV_FILE="/Users/yanghang/Code/PetManager/.env"
if [ -f "$ENV_FILE" ]; then
    echo "Loading environment variables from $ENV_FILE"
    set -a
    source "$ENV_FILE"
    set +a
else
    echo "Warning: Environment file $ENV_FILE not found"
fi

# 获取当前脚本所在目录（bin目录）
BIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 计算项目根目录（bin目录的父目录）
PROJECT_ROOT="$(dirname "${BIN_DIR}")"

# 设置编译器和选项
COMPILER="/usr/bin/clang++"
CPP_STD="-std=c++17"
DEFINE_FLAGS="-DCROW_ENABLE_SSL -DCROW_ENABLE_MULTIPART"
INCLUDE_DIRS="-I/opt/homebrew/opt/crow/include \
              -I/opt/homebrew/include \
              -I/opt/homebrew/opt/openssl/include \
              -I/opt/homebrew/opt/mysql-connector-c++/include \
              -I/opt/homebrew/opt/curl/include \
              -I/opt/homebrew/opt/nlohmann-json/include \
              -I${PROJECT_ROOT}/pethospital/backend/controllers \
              -I${PROJECT_ROOT}/pethospital/backend/models \
              -I${PROJECT_ROOT}/pethospital/backend/routes \
              -I${PROJECT_ROOT}/pethospital/backend/services \
              -I${PROJECT_ROOT}/pethospital/backend/middleware \
              -I${PROJECT_ROOT}/pethospital/backend/utils \
              -I${PROJECT_ROOT}/pethospital/backend/database \
              -I${PROJECT_ROOT}/pethospital/backend/websocket"

LIB_DIRS="-L/opt/homebrew/lib \
          -L/opt/homebrew/opt/openssl@3/lib \
          -L/opt/homebrew/opt/mysql-connector-c++/lib \
          -L/opt/homebrew/opt/curl/lib"

# 设置库文件路径，添加更多必要的 Boost 库,确保包含所有必要的库，特别是 curl 和 mysql 相关的库
LIBS="-lboost_date_time -lboost_filesystem -lboost_thread -lssl -lcrypto -lcurl -lmysqlcppconnx -lpthread"

# 查找所有.cpp文件，包括API目录下各个子目录的源文件
CPP_FILES=$(find "${PROJECT_ROOT}/pethospital/backend" -name "*.cpp" -type f ! -path "*/build/*")

# 设置输出文件路径
OUTPUT_FILE="${PROJECT_ROOT}/pethospital/backend/main"

# 执行编译
echo "正在编译项目..."
echo "编译器: ${COMPILER}"
# 计算源文件数量
FILE_COUNT=0
for file in $CPP_FILES; do
    if [ -f "$file" ]; then
        ((FILE_COUNT++))
    fi
done
echo "源文件数量: $FILE_COUNT"
echo "输出文件: ${OUTPUT_FILE}"

${COMPILER} ${CPP_STD} ${DEFINE_FLAGS} ${INCLUDE_DIRS} -Wall -g -pthread $CPP_FILES -o ${OUTPUT_FILE} ${LIB_DIRS} ${LIBS}

# 检查编译结果
if [ $? -eq 0 ]; then
    echo "编译成功！可执行文件位于: ${OUTPUT_FILE}"
else
    echo "编译失败！"
    exit 1
fi